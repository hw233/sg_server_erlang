#include "offline_arena.hpp"
#include "log.hpp"
#include "utility.hpp"
#include "main/center_server.hpp"
#include "main/center_server_fwd.hpp"
#include "redis_client.hpp"
#include "offline_arena_manager.hpp"
#include "tblh/OfflineArenaClass.tbls.h"
#include "tblh/MailTable.tbls.h"
#include "tblh/OfflineArenaAward.tbls.h"
#include "tblh/OfflineArenaScore.tbls.h"
#include "config_mgr.h"
#include "mail/mail_manager.hpp"
#include "tblh/mailcode_enum.hpp"
#include "user/global_user_manager.hpp"
USING_NS_COMMON;

void offline_arena_t::save_self()
{
	redis_client_t::set_uint32("offline_arena", "cur_class", m_uid, m_cur_class);
	redis_client_t::set_uint32("offline_arena", "cur_rank", m_uid, m_cur_rank);
	redis_client_t::set_uint32("offline_arena", "old_record_rank", m_uid, m_old_record_rank);
	redis_client_t::set_uint32("offline_arena", "record_rank", m_uid, m_record_rank);
	redis_client_t::set_uint32("offline_arena", "old_class", m_uid, m_old_class);
	redis_client_t::set_uint32("offline_arena", "fight_count", m_uid, m_fight_count);
	redis_client_t::set_uint32("offline_arena", "buy_count", m_uid, m_buy_count);
	redis_client_t::set_uint32("offline_arena", "score", m_uid, m_score);
	std::ostringstream sss;
	for (auto box_id : m_score_award)
	{
		if (!sss.str().empty())
		{
			sss << "$";
		}

		sss << box_id;
	}

	redis_client_t::set_string("offline_arena", "score_award", m_uid, sss.str());
}

void offline_arena_t::one_day()
{
	m_buy_count = 0;
	if (m_fight_count < offline_arena_manager_t::get_fight_count())
	{
		m_fight_count = offline_arena_manager_t::get_fight_count();
	}
	m_score = 0;
	m_score_award.clear();
	save_self();
}

void offline_arena_t::award()
{
	OfflineArenaClass* p_class_conf = GET_CONF(OfflineArenaClass, m_cur_class);
	if (NULL == p_class_conf)
	{
		log_error("NULL == p_class_conf[%d] OfflineArenaClass", m_cur_class);
		return;
	}
	//�������ʼ�
	OfflineArenaAward* p_award_conf = offline_arena_manager_t::get_award_conf_by_rank(m_cur_class, m_cur_rank);
	if (NULL != p_award_conf)
	{
		MailTable* p_mail = GET_CONF(MailTable, common::mailcode_enum::sys_offline_arena_daily);
		if (NULL == p_mail)
		{
			log_error("user[%lu] p_mail[%u] error null", m_uid, common::mailcode_enum::sys_offline_arena_daily);
			return;
		}
		std::string content_text;
		StringTool::format(content_text, common::string_util_t::convert_to_utf8(p_mail->content()).c_str(), common::string_util_t::convert_to_utf8(p_class_conf->name()).c_str(), m_cur_rank);

		mail_manager_t::send_mail(m_uid, proto::common::MAIL_TYPE_SYS, 0, common::string_util_t::convert_to_utf8(p_mail->from_name()), common::string_util_t::convert_to_utf8(p_mail->title()), content_text,
			p_award_conf->day_award(), proto::common::MAIL_GET_TYPE_ARENA);
	}
}

void offline_arena_t::score_award(uint32_t score)
{
	global_user_ptr p_role = global_user_manager_t::get_global_user_by_uid(m_uid);
	if (NULL == p_role)
	{
		log_error("role[%lu] not exist", m_uid);
		return;
	}
	if (m_score < score)
	{
		log_error("role[%lu] score[%d] not enough", m_uid, score);
		return;
	}
	auto p_conf = GET_CONF(OfflineArenaScore, score);
	if (NULL == p_conf)
	{
		log_error("OfflineArenaScore NULL == p_conf[%d]", score);
		return;
	}
	if (std::find(m_score_award.begin(), m_score_award.end(), score) != m_score_award.end())
	{
		log_error("role[%lu] score[%d] already award", m_uid, score);
		return;
	}
	m_score_award.push_back(score);
	save_self();
	proto::server::eg_offline_arena_score_award_notify ntf;
	ntf.set_drop_id(p_conf->drop());
	p_role->send_msg_to_game(op_cmd::eg_offline_arena_score_award_notify, ntf);
}

uint32_t offline_arena_t::get_max_record_num()
{
	return 50;
}

void offline_arena_t::add_record(offline_arena_record_ptr p_record)
{
	uint32_t max_record_num = get_max_record_num();
	if (m_record_data.size() >= max_record_num)
	{
		m_record_data.erase(m_record_data.begin());
	}
	m_record_data.push_back(p_record);
}

void offline_arena_t::peek_record_data(proto::common::offline_arena_record_data* p_data)
{
	if (NULL == p_data)
	{
		log_error("NULL == p_data");
		return;
	}
	for (uint32_t i = m_record_data.size(); i > 0; --i)
	{
		offline_arena_record_ptr p_record = m_record_data[i - 1];
		if (NULL == p_record)
		{
			continue;
		}
		proto::common::offline_arena_record* p_single = p_data->add_record_data();
		p_record->peek_data(p_single);
	}
}

void offline_arena_t::save_record_data()
{
	proto::common::offline_arena_record_data oacd;
	peek_record_data(&oacd);
	redis_client_t::set_protobuf("offline_arena", "record_data", m_uid, oacd);
}

void offline_arena_t::peek_data(proto::common::offline_arena_single* p_single)
{
	if (NULL == p_single)
	{
		log_error("NULL == p_single");
		return;
	}
	p_single->set_role_uid(string_util_t::uint64_to_string(m_uid));
	p_single->set_old_class(m_old_class);
	p_single->set_cur_class(m_cur_class);
	p_single->set_cur_rank(m_cur_rank);
	p_single->set_old_record_rank(m_old_record_rank);
	p_single->set_record_rank(m_record_rank);
	p_single->set_fight_count(m_fight_count);
	p_single->set_buy_count(m_buy_count);
	p_single->mutable_form_data()->mutable_obj()->set_fighting(offline_arena_manager_t::get_offline_arena_fighting(m_uid));
	//redis_client_t::get_protobuf("offline_arena", "fight_data", m_uid, *p_single->mutable_form_data());
	p_single->set_score(m_score);
	
	std::map<uint32_t, OfflineArenaScore*> score_map;
	GET_ALL_CONF(OfflineArenaScore, score_map);
	for (auto it : score_map)
	{
		auto p_conf = it.second;
		if (NULL == p_conf)
		{
			continue;
		}
		proto::common::liveness_box* p_box = p_single->add_score_box();
		p_box->set_liveness(it.first);
		if (m_score < it.first)
		{
			p_box->set_state(0);
		}
		else
		{
			if (std::find(m_score_award.begin(), m_score_award.end(), it.first) == m_score_award.end())
			{
				p_box->set_state(1);
			}
			else
			{
				p_box->set_state(2);
			}
		}
	}
}