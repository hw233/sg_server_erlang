#include "role_msg_handle.hpp"
#include <list>
#include "role_manager.hpp"
#include "role/role_manager.hpp"
#include "gm_cmd/gm_cmd.hpp"
#include "config/config_manager.hpp"
#include "hero/hero_manager.hpp"
#include "item/item_manager.hpp"
#include "monster/monster_manager.hpp"
#include "hero/cultivate_manager.hpp"
#include "fight/fight_manager.hpp"
#include "money_manager.hpp"
#include "tblh/errcode_enum.hpp"
#include "king_war/king_war_manager.hpp"
#include "config_mgr.h"
#include "tblh/Comprehensive.tbls.h"
#include "money_manager.hpp"
#include "achieve/achieve_common.hpp"

USING_NS_NETWORK;
USING_NS_COMMON;

bool role_msg_handle_t::init_msg_handler()
{
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_gm_cmd_request, handle_cg_gm_cmd_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_move_request, handle_cg_role_move_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_hero_cultivate_request, handle_cg_hero_cultivate_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_hero_recruit_request, handle_cg_hero_recruit_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_enter_fight_request, handle_cg_enter_fight_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_change_pk_mode_request, handle_cg_change_pk_mode_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_exchange_money_request, handle_cg_exchange_money_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_change_sys_state_notify, handle_cg_change_sys_state_notify);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_object_revive_request, handle_cg_object_revive_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_world_cup_play_request, handle_cg_world_cup_play_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_world_cup_bet_request, handle_cg_world_cup_bet_request);
    gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_world_cup_bet_reward_request, handle_cg_world_cup_bet_reward_request);
    gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_finish_new_role_guide_notify, handle_cg_finish_new_role_guide_notify);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_change_hero_plugin_request, handle_cg_change_hero_plugin_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_recharge_request,handle_cg_recharge_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_patrol_request, handle_cg_patrol_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_get_level_reward_request, handle_cg_get_level_reward_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_redbag_send_request, handle_cg_redbag_send_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_offline_arena_buy_request, handle_cg_offline_arena_buy_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_recharge_reward_request, handle_cg_recharge_reward_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_role_main_hero_unlock_request, handle_cg_role_main_hero_unlock_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_role_main_hero_change_request, handle_cg_role_main_hero_change_request);
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_buy_vigour_request, handle_cg_buy_vigour_request);

	//给robot使用，用于修改gm权限
	gate_msg_handler_t::m_gate_msg_handle_from_client.register_func(op_cmd::cg_set_gm_level, handle_cg_set_gm_level);

	return true;
}

bool role_msg_handle_t::handle_cg_gm_cmd_request(const network::tcp_socket_ptr& s, const msg_buf_ptr& msg_buf)
{
	return gm_cmd_t::handle_gm_cmd(msg_buf);
}

bool role_msg_handle_t::handle_cg_role_move_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_move_request);
	//ROLE_LOG("role_move_request role [%lu]", uid);

	const role_ptr& p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	p_role->move_to(msg.pos().x(), msg.pos().y(), msg.time_stamp(), msg.direction());

	return true;
}

bool role_msg_handle_t::handle_cg_hero_cultivate_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_hero_cultivate_request);
	//CULTI_LOG("hero_cultivate_request role [%lu]", uid);
	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}
	proto::client::gc_hero_cultivate_reply reply;
	uint32_t out = 0;
	uint32_t ret = cultivate_manager_t::cultivate_hero(p_role, msg, out, reply.mutable_rcd());
	reply.set_reply_code(ret);
	reply.set_type(msg.type());
	if (out != 0)
	{
		reply.set_value(out);
	}
	p_role->send_msg_to_client(op_cmd::gc_hero_cultivate_reply, reply);
	return true;
}

bool role_msg_handle_t::handle_cg_hero_recruit_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_hero_recruit_request);
	//CULTI_LOG("hero_recruit_request role [%lu]", uid);
	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}
	proto::client::gc_hero_recruit_reply reply;
	uint32_t ret = cultivate_manager_t::recruit_hero(p_role, msg.hero_id(), reply.mutable_rcd());
	reply.set_reply_code(ret);
	p_role->send_msg_to_client(op_cmd::gc_hero_recruit_reply, reply);
	return true;
}

// 进入战斗请求
bool role_msg_handle_t::handle_cg_enter_fight_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_enter_fight_request);
	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	fight_manager_t::enter_fight_request(p_role, msg);
	return true;
}

bool role_msg_handle_t::handle_cg_change_pk_mode_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	// 无效
	//PRE_S2S_MSG(proto::client::cg_change_pk_mode_request);
	//ROLE_LOG("role [%lu] change_pk_mode", uid);
	//auto p_role = role_manager_t::find_role(uid);
	//if (nullptr == p_role)
	//{
	//	log_error("invalid role");
	//	return false;
	//}
	//proto::client::gc_change_pk_mode_reply reply;
	//
	//if (msg.mode() > proto::common::PK_SLAUGHTER)
	//{
	//	reply.set_reply_code(1);
	//}
	//else if (p_role->get_pk_punish_time() == 0)
	//{
	//	p_role->change_pk_mode(msg.mode());
	//	reply.set_reply_code(0);
	//}
	//else
	//{
	//	reply.set_reply_code(1);
	//}
	//p_role->send_msg_to_client(op_cmd::gc_change_pk_mode_reply, reply);
	return true;
}

bool role_msg_handle_t::handle_cg_exchange_money_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_exchange_money_request);
	//ROLE_LOG("role [%lu] cg_exchange_money", uid);
	auto p_role = role_manager_t::find_role(uid);
	if (nullptr == p_role)
	{
		log_error("invalid role");
		return false;
	}
	proto::client::gc_exchange_money_reply reply;

	if (money_manager_t::exchange_money(p_role, (proto::common::MONEY_TYPE)msg.source_type(), msg.count(), (proto::common::MONEY_TYPE)msg.dest_type(), reply.mutable_role_data()))
	{
		reply.set_reply_code(0);
	}
	else
	{
		reply.set_reply_code(1);
	}
	//ROLE_LOG("gold[%d] silver[%d] copper[%d]", reply.role_data().per_info().gold(), reply.role_data().per_info().silver(), reply.role_data().per_info().copper());
	p_role->send_msg_to_client(op_cmd::gc_exchange_money_reply, reply);
	return true;
}

bool role_msg_handle_t::handle_cg_change_sys_state_notify(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_change_sys_state_notify);
	role_ptr p_role = role_manager_t::find_role(uid);
	if (nullptr == p_role)
	{
		log_error("invalid role");
		return false;
	}

	uint32_t back_code = common::errcode_enum::error_ok;
	if (msg.has_obj_state())
	{
		back_code = p_role->change_object_state(msg.obj_state());
	}
	else if (msg.has_mode_state())
	{
		back_code = p_role->toggle_ride_mount(msg.mode_state());
	}
	else
	{
		log_error("role[%lu] error change state", uid);
		return false;
	}

	// 更改状态失败通知客户端,告知现在状态
	if (back_code != common::errcode_enum::error_ok)
	{
		proto::client::gc_change_sys_state_notify notify;
		notify.set_reply_code(back_code);
		if (msg.has_obj_state())
		{
			proto::common::object_state_info* object_state = notify.mutable_object_state();
			if (object_state != NULL)
			{
				object_state->set_obj_state(p_role->get_object_state());
				object_state->set_obj_state_param(common::string_util_t::uint64_to_string(p_role->get_object_state_param()));
			}
		}
		else if (msg.has_mode_state())
		{
			proto::common::object_state_info* object_state = notify.mutable_object_state();
			if (object_state != NULL)
			{
				object_state->set_mode_state(p_role->get_mode_state());
			}
		}

		p_role->send_msg_to_client(op_cmd::gc_change_sys_state_notify, notify);
	}
	return true;
}

bool role_msg_handle_t::handle_cg_object_revive_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_object_revive_request);
	role_ptr p_role = role_manager_t::find_role(uid);
	if (NULL == p_role)
	{
		log_error("invalid role");
		return false;
	}

	proto::client::gc_object_revive_reply reply;
	reply.set_reply_code(errcode_enum::error_ok);
	do 
	{
		if (p_role->get_object_state() != proto::common::object_state_death)
		{
			log_error("role[%lu] object state[%u] error", uid, p_role->get_object_state());
			reply.set_reply_code(errcode_enum::sys_notice_object_not_dead);
			break;
		}

		// 不是自动复活
		if (!msg.is_auto())
		{
			switch (msg.revive_type())
			{
			case proto::common::object_revive_king_war:
				{
					if (!p_role->is_in_king_war())
					{
						log_error("role[%lu] not in king war, revive type error", uid);
						reply.set_reply_code(errcode_enum::notice_unknown);
						break;
					}

					king_war_role_ptr p_king_war_role = king_war_manager_t::get_king_war_role(p_role->get_country_id(), p_role->get_object_id());
					if (p_king_war_role == NULL)
					{
						log_error("role[%lu] not find king war info", uid);
						reply.set_reply_code(errcode_enum::notice_unknown);
						break;
					}

					if (p_king_war_role->m_free_revive_count > 0)
					{
						// 自动复活
						king_war_manager_t::role_fast_revive(p_role);

						ROLE_LOG("role[%lu] cost free revive count", p_role->get_object_id());
						break;
					}
					else // cost money
					{
						money_ptr p_money = p_role->get_money();
						if (p_money == NULL)
						{
							log_error("user[%lu] p_money null error", p_role->get_object_id());
							reply.set_reply_code(errcode_enum::notice_unknown);
							break;
						}

						auto p_conf = GET_CONF(Comprehensive, comprehensive_common::king_revive_free_cnt);
						uint32_t free_count = GET_COMPREHENSIVE_VALUE_1(p_conf);

						p_conf = GET_CONF(Comprehensive, comprehensive_common::king_revive_cost_initial);
						uint32_t cost_initial = GET_COMPREHENSIVE_VALUE_1(p_conf);

						p_conf = GET_CONF(Comprehensive, comprehensive_common::king_revive_cost_increase);
						uint32_t cost_increase = GET_COMPREHENSIVE_VALUE_1(p_conf);

						uint32_t cost_gold = cost_initial + (p_king_war_role->m_death_count - free_count) * cost_increase;

						uint32_t money = p_money->get_money(proto::common::MONEY_TYPE_YUANBAO);
						if (money < cost_gold)
						{
							log_error("role[%lu] gold[%u] < cost_gold[%u] ", p_role->get_object_id(), money, cost_gold);
							reply.set_reply_code(errcode_enum::notice_gold_money_not_enough);
							break;
						}
						
						if (!money_manager_t::use_money(p_role, proto::common::MONEY_TYPE_YUANBAO, cost_gold, log_enum::source_type_role_use_gold_to_revive,
							0, false, reply.mutable_change_data()))
						{
							log_error("role[%lu] use_gold money error", p_role->get_object_id());
							reply.set_reply_code(errcode_enum::notice_unknown);
							break;
						}

						ROLE_LOG("role[%lu] cost gold[%u] revive", p_role->get_object_id(), cost_gold);

						// 花钱复活成功
						break;
					}
				}
				break;
			case proto::common::object_revive_dungeon:
				{
					// TODO:复活道具
				}
				break;
			case proto::common::object_revive_family_war:
				{
					// TODO:复活道具
				}
				break;
			default:
				{
					log_warn("role[%lu] revive_type[%u] not do!!!!!!!!!!!!!!!!!!!!!!!!", uid, msg.revive_type());
					reply.set_reply_code(errcode_enum::notice_unknown);
				}
				break;
			}
		}
		else
		{
			// 自动复活检测时间到没到
			uint32_t time_now = common::time_util_t::now_time();
			if (p_role->get_revive_time() > time_now)
			{
				log_error("role[%lu] object revive time not on", uid);
				reply.set_reply_code(errcode_enum::sys_notice_object_revive_time);
				reply.set_left_time(p_role->get_revive_time() - time_now);
				break;
			}
		}

		ROLE_LOG("role[%lu] auto time revive", p_role->get_object_id());

	} while (0);

	// 人物复活
	if (reply.reply_code() == errcode_enum::error_ok)
	{
		if (p_role->is_in_dungeon())
		{
			// 副本在副本里处理，客户端没法跟踪复活状态
			const scene_ptr& p_scene = scene_manager_t::find_scene(p_role->get_scene_id());
			if (NULL != p_scene)
			{
				p_scene->revive_all_role();
			}
		}
		else
		{
			p_role->set_object_revive(0);
			p_role->send_msg_to_client(op_cmd::gc_object_revive_reply, reply);
		}
		achieve_common_t::notify_progress_state(p_role->get_uid(), proto::common::ACHIEVE_KING_WAR_QUICK_REVIVE);
	}
	else
	{
		p_role->send_msg_to_client(op_cmd::gc_object_revive_reply, reply);
	}

	return true;
}

bool role_msg_handle_t::handle_cg_world_cup_play_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_world_cup_play_request);
	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}
	proto::server::gr_world_cup_play_request req;
	req.set_server_id(env::xml->srv_group.group_id);
	p_role->peek_cross_data(req.mutable_rcd(), proto::common::role_cross_type_arena);
	env::server->send_msg_to_area(op_cmd::gr_world_cup_play_request, uid, req);
	return true;
}

bool role_msg_handle_t::handle_cg_world_cup_bet_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_world_cup_bet_request);
	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}
	if (!money_manager_t::use_money(p_role, proto::common::MONEY_TYPE_COPPER, msg.bet(), log_enum::source_type_world_cup_bet, 0))
	{
		log_error("p_role money not enough [%lu]", uid);
		proto::client::gc_world_cup_bet_reply reply;
		reply.set_reply_code(errcode_enum::notice_copper_money_not_enough);
		p_role->send_msg_to_client(op_cmd::gc_world_cup_bet_reply, reply);
		return false;
	}
	proto::server::gr_world_cup_bet_request req;
	req.set_server_id(env::xml->srv_group.group_id);
	req.set_battle_uid(string_util_t::string_to_uint64(msg.battle_uid()));
	req.set_winner(string_util_t::string_to_uint64(msg.winner()));
	req.set_odds(msg.odds());
	req.set_bet(msg.bet());
	env::server->send_msg_to_area(op_cmd::gr_world_cup_bet_request, uid, req);
	return true;
}

bool role_msg_handle_t::handle_cg_world_cup_bet_reward_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_world_cup_bet_reward_request);
	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	proto::server::gr_world_cup_bet_reward_request req;
	req.set_server_id(env::xml->srv_group.group_id);
	req.set_battle_uid(string_util_t::string_to_uint64(msg.battle_uid()));
	env::server->send_msg_to_area(op_cmd::gr_world_cup_bet_reward_request, uid, req);
	return true;
}

bool role_msg_handle_t::handle_cg_finish_new_role_guide_notify(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
    PRE_S2S_MSG(proto::client::cg_finish_new_role_guide_notify);
    role_ptr p_role = role_manager_t::find_role(uid);
    if (p_role == NULL)
    {
        log_error("p_role is null [%lu]", uid);
        return false;
    }
	if (0 != msg.tid())
	{
		p_role->finish_new_role_guide_from_client(msg.tid());
	}
  
    return true;
}

bool role_msg_handle_t::handle_cg_change_hero_plugin_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_change_hero_plugin_request);
	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}
	proto::client::gc_change_hero_plugin_reply reply;
	proto::common::role_change_data* p_data = reply.mutable_rcd();
	if (NULL == p_data)
	{
		log_error("NULL == p_data");
		return false;
	}
	uint32_t reply_code = hero_manager_t::change_hero_plugin(p_role, msg.hero_id(), msg.plugin_id(),p_data);
	reply.set_reply_code(reply_code);
	p_role->send_msg_to_client(op_cmd::gc_change_hero_plugin_reply, reply);
	return true;
}

bool role_msg_handle_t::handle_cg_recharge_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_recharge_request);
	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}
	proto::client::gc_recharge_reply reply;
	proto::common::role_change_data* p_data = reply.mutable_rcd();
	uint32_t reply_code = money_manager_t::game_recharge(uid, msg.tid(), p_data);
	reply.set_reply_code(reply_code);
	p_role->send_msg_to_client(op_cmd::gc_recharge_reply, reply);
	return true;
}

bool role_msg_handle_t::handle_cg_patrol_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_patrol_request);

	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL) {
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	uint32_t reply_code = p_role->patrol_request(msg.map_id(), msg.patrol());
	if (reply_code != common::errcode_enum::error_ok) {
		proto::client::gc_patrol_reply reply;
		reply.set_reply_code(reply_code);
		p_role->send_msg_to_client(op_cmd::gc_patrol_reply, reply);
	}

	return true;
}

bool role_msg_handle_t::handle_cg_get_level_reward_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_get_level_reward_request);

	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL) {
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	proto::client::gc_get_level_reward_reply reply;
	uint32_t reply_code = p_role->get_level_reward(msg.id(), reply.mutable_change_data());
	reply.set_reply_code(reply_code);
	reply.set_id(msg.id());
	p_role->send_msg_to_client(op_cmd::gc_get_level_reward_reply, reply);

	return true;
}

bool role_msg_handle_t::handle_cg_redbag_send_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_redbag_send_request);

	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL) 
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	proto::client::gc_redbag_send_reply reply;
	uint32_t reply_code = p_role->send_redbag(msg.count(), msg.money(), reply.mutable_rcd());
	reply.set_reply_code(reply_code);
	p_role->send_msg_to_client(op_cmd::gc_redbag_send_reply, reply);

	return true;
}

bool role_msg_handle_t::handle_cg_offline_arena_buy_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_offline_arena_buy_request);

	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}
	
	auto p_conf = GET_CONF(Comprehensive, comprehensive_common::offline_arena_cost_number);
	uint32_t money_type = GET_COMPREHENSIVE_VALUE_1(p_conf);
	if (!money_manager_t::use_money(p_role, (proto::common::MONEY_TYPE)money_type, msg.money(), log_enum::source_type_offline_arena_buy, 0))
	{
		proto::client::gc_offline_arena_buy_reply reply;
		reply.set_reply_code(errcode_enum::notice_gold_money_not_enough);
		p_role->send_msg_to_client(op_cmd::gc_offline_arena_buy_reply, reply);
	}
	else
	{
		proto::server::ge_offline_arena_buy_request req;
		req.set_money(msg.money());
		env::server->send_msg_to_center(op_cmd::ge_offline_arena_buy_request, uid, req);
	}
	return true;
}

bool role_msg_handle_t::handle_cg_recharge_reward_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_recharge_reward_request);

	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}
	proto::client::gc_recharge_reward_reply reply;
	uint32_t reply_code = p_role->get_recharge_gift(reply.mutable_rcd());
	reply.set_reply_code(reply_code);
	p_role->send_msg_to_client(op_cmd::gc_recharge_reward_reply, reply);
	return true;
}

bool role_msg_handle_t::handle_cg_role_main_hero_unlock_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_role_main_hero_unlock_request);

	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	proto::client::gc_role_main_hero_unlock_reply reply;
	uint32_t reply_code = hero_manager_t::unlock_main_hero(p_role, msg.hero_tid(), reply.mutable_rcd());
	reply.set_hero_tid(msg.hero_tid());
	reply.set_reply_code(reply_code);
	p_role->send_msg_to_client(op_cmd::gc_role_main_hero_unlock_reply, reply);
	return true;
}


bool role_msg_handle_t::handle_cg_role_main_hero_change_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_role_main_hero_change_request);

	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	hero_manager_t::change_role_main_hero(p_role, msg.hero_tid());
	return true;
}

bool role_msg_handle_t::handle_cg_buy_vigour_request(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_buy_vigour_request);

	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL) {
		log_error("p_role is null [%lu]", uid);
		return false;
	}

	proto::client::gc_buy_vigour_reply reply;
	uint32_t back_code = money_manager_t::role_buy_vigour(p_role);
	reply.set_reply_code(back_code);
	if (reply.mutable_rcd()) p_role->get_personal_info(reply.mutable_rcd()->mutable_per_info());
	p_role->send_msg_to_client(op_cmd::gc_buy_vigour_reply, reply);

	return true;
}

bool role_msg_handle_t::handle_cg_set_gm_level(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& msg_buf)
{
	PRE_S2S_MSG(proto::client::cg_set_gm_level);
	role_ptr p_role = role_manager_t::find_role(uid);
	if (p_role == NULL)
	{
		log_error("p_role is null [%lu]", uid);
		return false;
	}
	
	if ( msg.has_gm_level() )
	{
		p_role->set_gm(msg.gm_level());
		return true;
	}
	p_role->set_gm(0);
	p_role->save_self();
	return true;
}

