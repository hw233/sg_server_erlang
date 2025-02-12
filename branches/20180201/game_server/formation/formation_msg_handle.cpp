#include "formation_msg_handle.hpp"
#include "formation_manager.hpp"
#include <list>
#include "role/role.hpp"
#include "protos_fwd.hpp"
#include "client.pb.h"
#include "fight/combat.hpp"
#include "fight/fight_manager.hpp"
#include "user_troop/user_troop_mgr.hpp"

USING_NS_NETWORK;
USING_NS_COMMON;

bool formation_msg_handle_t::init_msg_handler()
{
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_update_formation_request, handle_update_formation_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_update_cur_form_id_request, handle_update_cur_form_id_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_fight_update_formation_request, handle_fight_update_formation_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_fight_update_cur_form_id_request, handle_fight_update_cur_form_id_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_get_fight_formation_req, handle_get_fight_formation_req);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_get_fight_special_heroes_request, handle_get_fight_special_heroes_request);
    return true;
}

bool formation_msg_handle_t::handle_update_formation_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_update_formation_request);
	role_ptr p_role = role_manager_t::find_role(uid);

	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}
	const proto::common::formation_single& form_data = msg.form();
	proto::client::gc_update_formation_reply reply;
	reply.set_reply_code(0);

	std::map<uint32_t, uint64_t> hero_pos;

	for (auto it_pair : form_data.formations())
	{
		hero_pos[it_pair.pos()] = (string_util_t::string_to_uint64(it_pair.unique_id()));
	}
	if (!p_role->m_formation_mgr.update_formation(p_role, form_data.formation_index(), proto::common::fight_type_none, hero_pos, form_data.tactic_id()))
	{
		reply.set_reply_code(1);
	}

	p_role->m_formation_mgr.peek_datas(form_data.formation_index(), reply.mutable_formation());

	p_role->send_msg_to_client(op_cmd::gc_update_formation_reply, reply);
	
	return true;
}

bool formation_msg_handle_t::handle_update_cur_form_id_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_update_cur_form_id_request);

	role_ptr p_role = role_manager_t::find_role(uid);

	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	p_role->m_formation_mgr.update_cur_form_id(msg.cur_form_id());
	proto::client::gc_update_cur_form_id_reply reply;
	reply.set_cur_form_id(p_role->m_formation_mgr.get_cur_formation_id());

	p_role->send_msg_to_client(op_cmd::gc_update_cur_form_id_reply, reply);
	return true;
}

bool formation_msg_handle_t::handle_fight_update_formation_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_fight_update_formation_request);
	role_ptr p_role = role_manager_t::find_role(uid);

	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}
	const proto::common::formation_single& form_data = msg.form();
	proto::client::gc_fight_update_formation_reply reply;
	reply.set_reply_code(0);

	uint64_t fight_uid = p_role->get_fight_uid();
	if (fight_uid == 0)
	{
		log_error("cg_fight_update_formation_request error fight_uid = 0 role_uid[%lu]", uid);
		return true;
	}
	combat_ptr p_combat = fight_manager_t::get_combat(fight_uid);
	if (NULL == p_combat)
	{
		log_error("cg_fight_update_formation_request error p_combat NULL role_uid[%lu]", uid);
		return true;
	}

	// 获取个人或是队伍的布阵信息
	formation_manager_t *p_formation_mgr = NULL;
	switch (p_combat->get_combat_type())
	{
	case combat_type_single_role:
		p_formation_mgr = &p_role->m_formation_mgr;
		break;
	case combat_type_troop:
		{
			troop_ptr p_troop = game_troop_mgr_t::get_troop(p_role->get_troop_id());
			if (p_troop != NULL)
			{
				p_formation_mgr = &(p_troop->get_formation_mgr());
			}
		}
		break;
	default:
		break;
	}

	if (NULL == p_formation_mgr)
	{
		log_error("cg_fight_update_formation_request error p_formation_mgr NULL role_uid[%lu]", uid);
		return true;
	}

	std::map<uint32_t, uint64_t> hero_pos;

	for (auto it_pair : form_data.formations())
	{
		hero_pos[it_pair.pos()] = (string_util_t::string_to_uint64(it_pair.unique_id()));
	}
	if (!p_formation_mgr->update_formation(p_role, form_data.formation_index(), p_combat->get_fight_type(), hero_pos, form_data.tactic_id()))
	{
		reply.set_reply_code(1);
	}

	p_formation_mgr->peek_datas(form_data.formation_index(), reply.mutable_formation());
	reply.set_role_uid(string_util_t::uint64_to_string(uid));
	p_combat->send_msg_to_all(op_cmd::gc_fight_update_formation_reply, reply);
	return true;
}

bool formation_msg_handle_t::handle_fight_update_cur_form_id_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_fight_update_cur_form_id_request);

	role_ptr p_role = role_manager_t::find_role(uid);

	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	uint64_t fight_uid = p_role->get_fight_uid();
	if (fight_uid == 0)
	{
		log_error("cg_fight_update_cur_form_id_request error fight_uid = 0 role_uid[%lu]", uid);
		return true;
	}
	combat_ptr p_combat = fight_manager_t::get_combat(fight_uid);
	if (NULL == p_combat)
	{
		log_error("cg_fight_update_cur_form_id_request error p_combat NULL role_uid[%lu]", uid);
		return true;
	}

	// 获取个人或是队伍的布阵信息
	formation_manager_t *p_formation_mgr = NULL;
	switch (p_combat->get_combat_type())
	{
	case combat_type_single_role:
		p_formation_mgr = &p_role->m_formation_mgr;
		break;
	case combat_type_troop:
		{
			// 暂时没有组队换阵的需求 先注掉 需要的时候打开就可以用
// 			troop_ptr p_troop = game_troop_mgr_t::get_troop(p_role->get_troop_id());
// 			if (p_troop != NULL)
// 			{
// 				p_formation_mgr = &(p_troop->get_formation_mgr());
// 			}
		}
		break;
	default:
		break;
	}

	if (NULL == p_formation_mgr)
	{
		log_error("cg_fight_update_cur_form_id_request error p_formation_mgr NULL role_uid[%lu]", uid);
		return true;
	}

	// 通知客户端换默认阵
	p_formation_mgr->update_cur_form_id(msg.cur_form_id());
	proto::client::gc_fight_update_cur_form_id_reply reply;
	reply.set_cur_form_id(p_formation_mgr->get_cur_formation_id());	
	p_role->send_msg_to_client(op_cmd::gc_fight_update_cur_form_id_reply, reply);

	// 广播给其他人当前阵容信息
	proto::client::gc_fight_update_formation_reply up_form_reply;
	up_form_reply.set_reply_code(0);
	p_role->m_formation_mgr.peek_cur_formation_datas(up_form_reply.mutable_formation());
	up_form_reply.set_role_uid(string_util_t::uint64_to_string(uid));
	p_combat->send_msg_to_all(op_cmd::gc_fight_update_formation_reply, up_form_reply);

	return true;
}

bool formation_msg_handle_t::handle_get_fight_formation_req(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_get_fight_formation_req);

	role_ptr p_role = role_manager_t::find_role(uid);

	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	combat_ptr p_combat = fight_manager_t::get_combat(p_role->get_fight_uid());
	if (NULL == p_combat)
	{
		log_error("cg_get_fight_formation_req error p_combat NULL role_uid[%lu]", uid);
		return true;
	}
	formation_ptr p_self_formation = p_combat->get_formation(proto::common::combat_camp_1);
	formation_ptr p_enemy_formation = p_combat->get_formation(proto::common::combat_camp_2);

	proto::client::gc_get_fight_formation_reply reply;
	if (p_self_formation)
	{
		p_self_formation->peek_data(reply.mutable_self_form());
	}
	if (p_enemy_formation)
	{
		p_enemy_formation->peek_data(reply.mutable_enemy_form());
	}

	p_role->send_msg_to_client(op_cmd::gc_get_fight_formation_reply, reply);
	return true;
}

bool formation_msg_handle_t::handle_get_fight_special_heroes_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_get_fight_special_heroes_request);
	role_ptr p_role = role_manager_t::find_role(uid);

	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	proto::client::gc_get_fight_special_heroes_reply reply;
	reply.set_fight_type(msg.fight_type());

	switch (msg.fight_type())
	{
	case proto::common::fight_type_family_war:
	case proto::common::fight_type_family_pk:
		{
			formation_manager_t::peek_family_formation_data(p_role, reply.mutable_hero_list());
		}
		break;
	case proto::common::fight_type_expedition:
		{
			formation_manager_t::peek_expedition_formation_data(p_role, reply.mutable_hero_list());
		}
		break;
	default:
		break;
	}

	p_role->send_msg_to_client(op_cmd::gc_get_fight_special_heroes_reply, reply);

	return true;
}
