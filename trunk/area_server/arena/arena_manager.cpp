#include "arena_manager.hpp"
#include "main/area_server.hpp"
#include "main/area_server_fwd.hpp"
#include "config/config_manager.hpp"
#include "tblh/ArenaClassTable.tbls.h"
#include "tblh/ArenaBattleAwardTable.tbls.h"
#include "tblh/Comprehensive.tbls.h"
#include "tblh/Monster.tbls.h"
#include "tblh/FightAgainst.tbls.h"
#include "tblh/Activities.tbls.h"
#include "role/arena_role_manager.hpp"
#include "tblh/errcode_enum.hpp"
#include "arena_score_rank_mgr.hpp"
#include "world_cup/world_cup_manager.hpp"
#include "time_manager.hpp"
USING_NS_NETWORK;
USING_NS_COMMON;

arena_manager_t::match_pools arena_manager_t::m_match_pools;
arena_manager_t::ai_match_pools arena_manager_t::m_ai_match_pools;
arena_manager_t::arena_ai_map arena_manager_t::m_ai_map;
arena_manager_t::arena_ai_cache_vec arena_manager_t::m_ai_cache_vec;
//uint32_t arena_manager_t::m_dynamic_create_ai_count = 0;
uint32_t arena_manager_t::m_ai_id = 0;

void arena_manager_t::init()
{
	auto conf_match = GET_CONF(Comprehensive, comprehensive_common::arena_match_pool_interval_score);
	uint32_t match_pool_interval_score = GET_COMPREHENSIVE_VALUE_1(conf_match);

	if (NULL == ArenaClassTableManager::getInstancePtr())
	{
		log_error("ArenaClassTableManager::getInstancePtr()");
		return;
	}
	if (match_pool_interval_score == 0)
	{
		log_error("match_pool_interval_score = 0");
		return;
	}
	uint32_t size = ArenaClassTableManager::getInstancePtr()->getSize();
	auto conf_min = GET_CONF(ArenaClassTable, 1);
	auto conf_max = GET_CONF(ArenaClassTable, size);
	uint32_t min_score = conf_min->score();
	uint32_t max_score = conf_max->score();
	uint32_t match_pools_num = (max_score - min_score) / match_pool_interval_score;

	// 有多少个玩家匹配池分段 就有多少个AI匹配池分段(某个匹配池没有AI正常 取决于策划配置)
	arena_match_pool match_pool;
	arena_ai_match_pool ai_match_pool;
	for (uint32_t i = 0; i < match_pools_num; ++i)
	{
		m_match_pools.push_back(match_pool);
		m_ai_match_pools.push_back(ai_match_pool);
	}
	//m_match_pools.resize(match_pools_num);

	init_ai(min_score, match_pool_interval_score);

	//读取活动表
	auto arena_reset_time_id = GET_CONF(Comprehensive, comprehensive_common::arena_reset_time_id);
	uint32_t reset_time_id = GET_COMPREHENSIVE_VALUE_1(arena_reset_time_id);
	if (0 == reset_time_id)
	{
		log_error("arena reset time_id invalid");
		return;
	}
	time_manager_t::register_func(reset_time_id, arena_manager_t::on_season);
	//读取活动表
	auto p_activities_conf = GET_CONF(Activities, activity_arena);
	if (NULL == p_activities_conf)
	{
		log_error("NULL == p_activities_conf[%d]", activity_arena);
		return;
	}

	if (p_activities_conf->time_id().empty())
	{
		log_error("p_activities_conf[%d] time_id null", activity_arena);
		return;
	}
	uint32_t time_id = p_activities_conf->time_id().at(0);
	time_manager_t::register_func(time_id, arena_manager_t::on_close);

	arena_score_rank_mgr_t::init(reset_time_id);
	log_info("load score rank from redis success!");
}

void arena_manager_t::init_ai(uint32_t min_score, uint32_t match_pool_interval_score)
{
	if (match_pool_interval_score == 0)
	{
		log_error("match_pool_interval_score = 0");
		return;
	}
	// 读monster表 获取所有竞技场AI 填充到匹配池
	std::map<uint32_t, Monster*> confs;
	GET_ALL_CONF(Monster, confs);

	int32_t pool_pos = -1;
	int32_t match_pools_size = (int32_t)m_ai_match_pools.size();

	arena_ai_ptr p_ai =	arena_ai_ptr();
	uint32_t ai_id = 0;
	for (auto conf : confs)
	{
		Monster* p_conf = conf.second;
		if (NULL != p_conf && p_conf->type() == proto::common::EM_ARENA_AI)
		{
			pool_pos = (p_conf->arena_score() - (int32_t)min_score) / (int32_t)match_pool_interval_score;
			if (pool_pos < 0 || pool_pos >= match_pools_size)
			{
				log_error("arena ai init fail id[%d] score[%d]", p_conf->id(), p_conf->arena_score());
				continue;
			}
			ai_id = gen_ai_id();
			p_ai.reset(new arena_ai_t(ai_id, p_conf->id(), p_conf->arena_score(), p_conf->arena_level(), p_conf->name(),
									  p_conf->level(), p_conf->get_fight_ids(0)));
			
			m_ai_match_pools[pool_pos].m_arena_ai_match_pool.push_back(p_ai);
			m_ai_map.insert(std::make_pair(ai_id, p_ai));

			p_ai->set_pools_pos(pool_pos);
			p_ai->set_pool_pos(m_ai_match_pools[pool_pos].m_arena_ai_match_pool.size() - 1);
		}
	}
	// 创建动态AI
	for (uint32_t i = 0; i < DYNAMIC_AI_CREATE_MAX_NUM; ++i)
	{
		ai_id = gen_ai_id();
		arena_ai_ptr p_ai(new arena_ai_t(ai_id));
		m_ai_cache_vec.push_back(p_ai);
		m_ai_map.insert(std::make_pair(ai_id, p_ai));
	}
}

uint32_t arena_manager_t::match(uint64_t role_uid, uint32_t score, uint32_t arena_level, const proto::common::role_cross_data &cross_data)
{
	// 查找玩家 没有就创建
	arena_role_ptr p_role = arena_role_manager_t::find_role(role_uid);
	if (NULL == p_role)
	{
		p_role = arena_role_manager_t::add_role(role_uid, score, arena_level, cross_data);
		if (NULL == p_role)
		{
			return errcode_enum::notice_unknown;
		}
	}
	else
	{
		// 更新数据
		p_role->set_score(score);
		p_role->set_arena_level(arena_level);
		p_role->set_cross_data(cross_data);
	}

	// 记录匹配相关数据
	p_role->set_last_play_time(time(0));

	auto conf_min = GET_CONF(ArenaClassTable, 1); 
    if (NULL == conf_min)
    {
        log_error("NULL == conf_min[%d]", 1);
        return errcode_enum::notice_unknown;
    }
	auto conf_match = GET_CONF(Comprehensive, comprehensive_common::arena_match_pool_interval_score);
    if (NULL == conf_match)
    {
        log_error("NULL == conf_match[%d]", comprehensive_common::arena_match_pool_interval_score);
        return errcode_enum::notice_unknown;
    }
	auto conf_match_max_num = GET_CONF(Comprehensive, comprehensive_common::arena_match_maxnum);
    if (NULL == conf_match_max_num)
    {
        log_error("NULL == conf_match_max_num[%d]", comprehensive_common::arena_match_maxnum);
        return errcode_enum::notice_unknown;
    }

	uint32_t match_pool_interval_score = GET_COMPREHENSIVE_VALUE_1(conf_match);
	uint32_t match_max_num = GET_COMPREHENSIVE_VALUE_1(conf_match_max_num);
	uint32_t min_score = conf_min->score();
	// 分段间隔分数不能为0
	if (match_pool_interval_score == 0)
	{
		log_error("arena arena_match_pool_interval_score = 0");
		return errcode_enum::notice_no_match_role;
	}
	// 超过一定匹配次数去匹配AI
	if (p_role->get_match_times() >= match_max_num)
	{
		match_ai(p_role, min_score, score, match_pool_interval_score);
	}
	else
	{
		match_role(p_role, min_score, match_max_num, match_pool_interval_score);
	}


	return errcode_enum::error_ok;
}

// role_level为角色等级 非竞技场段位
void arena_manager_t::match_ai(arena_role_ptr p_role, uint32_t min_score, uint32_t score, uint32_t match_pool_interval_score)
{
	if (NULL == p_role)
	{
		log_error("arena p_role NULL");
		return;
	}
	if (match_pool_interval_score == 0)
	{
		log_error("match_pool_interval_score = 0");
		return;
	}
	// 计算所在匹配池
	int32_t pool_pos = (score - (int32_t)min_score) / (int32_t)match_pool_interval_score;
	int32_t match_pools_size = (int32_t)m_ai_match_pools.size();

	uint32_t reply_code = errcode_enum::error_ok;
	arena_ai_ptr p_match_target = arena_ai_ptr();

	do 
	{
		if (pool_pos < 0 || pool_pos >= match_pools_size)
		{
			break;
		}

		arena_ai_match_pool &find_pool = m_ai_match_pools[pool_pos];
		if (find_pool.m_arena_ai_match_pool.size() == 0)
		{
			break;
		}
		// 随机选择一个
		int32_t find_target_pos = random_util_t::randMin(0, find_pool.m_arena_ai_match_pool.size());
		if (find_target_pos == -1)
		{
			log_error("arena_manager_t find_target_pos error min[%d] max[%d]", 0, (int32_t)find_pool.m_arena_ai_match_pool.size());
			break;
		}
		p_match_target = find_pool.m_arena_ai_match_pool[find_target_pos];
		if (NULL == p_match_target)
		{
			log_error("arena p_match_target NULL pool_pos[%d] ai_pos[%d]", pool_pos, find_target_pos);
			break;
		}
	} while (0);

	// 发送给game_server匹配结果
	proto::server::rg_arena_match_reply msg_game;

	if (NULL == p_match_target)
	{
		reply_code = errcode_enum::notice_no_match_role;

		p_role->add_match_times();
		add_role(pool_pos, p_role);
	}
	else
	{
		// 匹配成功 重置匹配次数 在匹配池中移除双方
		p_role->reset_match_times();
		p_role->set_target_uid(p_match_target->get_id());
		// 移除自己
		if (p_role->get_in_pools_pos() != -1)
		{
			remove_role(p_role->get_in_pools_pos(), p_role->get_role_uid());
		}

		// 发送给cross服务器进行战斗
		proto::server::rs_arena_battle_ai_notify msg_cross;
		//msg_cross.mutable_fight()->set_type(proto::common::fight_type_cross_arena_rank);
		proto::common::fight_param* p_fight_param = msg_cross.mutable_fight();
		if (p_fight_param) {
			p_fight_param->set_type(proto::common::fight_type_cross_arena_rank);

			proto::common::cross_arena_param* p_cross_arena_param = p_fight_param->mutable_cross_arena();
			if (p_cross_arena_param) {
				proto::common::cross_arena_ex* p_cross_arena_ex1 = p_cross_arena_param->mutable_obj1();
				if (p_cross_arena_ex1) {
					p_cross_arena_ex1->set_score(p_role->get_score());
					p_cross_arena_ex1->set_rank_lv(p_role->get_rank_lv());
				}

				proto::common::cross_arena_ex* p_cross_arena_ex2 = p_cross_arena_param->mutable_obj2();
				if (p_cross_arena_ex2) {
					p_cross_arena_ex2->set_score(p_match_target->get_score());
				}

				if (!p_match_target->is_dynamic()) {
					p_cross_arena_param->set_arena_ai_id(p_match_target->get_id());
					p_cross_arena_param->set_monster_id(p_match_target->get_tid());
				}
			}
		}

		p_role->peek_data(msg_cross.mutable_user1());
		if (p_match_target->is_dynamic())
			p_match_target->peek_data(msg_cross.mutable_npc());
		else
			msg_cross.mutable_npc()->set_formation_id(p_match_target->get_formation());
		
		//msg_cross.mutable_fight()->mutable_arena_ai()->set_arena_ai_id(p_match_target->get_id());
		env::server->send_msg_to_cross(op_cmd::rs_arena_battle_ai_notify, p_role->get_role_uid(), msg_cross);

		// 填充发送给自己所在game_server的消息包
		msg_game.set_target_score(p_match_target->get_score());
		msg_game.set_target_arena_level(p_match_target->get_arena_level());
		msg_game.mutable_target_user()->mutable_rd()->set_uid(string_util_t::uint32_to_string(p_match_target->get_id()));
		msg_game.mutable_target_user()->mutable_rd()->set_name(string_util_t::convert_to_utf8(p_match_target->get_name()));
		FightAgainst* p_conf = GET_CONF(FightAgainst, p_match_target->get_formation());
		if (NULL != p_conf)
		{
			msg_game.mutable_target_user()->mutable_rd()->mutable_obj_info()->set_fighting_value(p_conf->fighting());
		}
	}

	msg_game.set_server_id(p_role->get_role_server_id());
	msg_game.set_reply_code(reply_code);
	env::server->send_msg_to_transfer(op_cmd::rg_arena_match_reply, p_role->get_role_uid(), msg_game);
}

void arena_manager_t::match_role(arena_role_ptr p_role, uint32_t min_score, uint32_t match_max_num, uint32_t match_pool_interval_score)
{
	if (NULL == p_role)
	{
		log_error("arena p_role NULL");
		return;
	}
	if (match_pool_interval_score == 0)
	{
		log_error("match_pool_interval_score = 0");
		return;
	}

	// 计算所在匹配池 小于最低分数段的都在同一个匹配池 大于最高分数段的也在同一个匹配池
	int32_t pool_pos = (p_role->get_score() - (int32_t)min_score) / (int32_t)match_pool_interval_score;
	int32_t match_pools_size = (int32_t)m_match_pools.size();

	if (pool_pos < 0)
		pool_pos = 0;
	else if (pool_pos >= match_pools_size)
		pool_pos = match_pools_size - 1;

	int32_t down_pool_pos = 0;
	int32_t up_pool_pos = 0;
	// 扩大匹配池
	if (p_role->get_match_times() > 0)
	{
		down_pool_pos = pool_pos - p_role->get_match_times();
		up_pool_pos = pool_pos + p_role->get_match_times();

		down_pool_pos = down_pool_pos < 0 ? 0 : down_pool_pos;
		up_pool_pos = up_pool_pos >= match_pools_size - 1 ? match_pools_size - 1 : up_pool_pos;
	}

	arena_role_ptr p_match_target = arena_role_ptr();
	int32_t find_target_pos = -1;		// 找到的目标所在的pool的位置
	int32_t find_pool_pos = -1;			// 找到的pool所在的pools的位置
	ARENA_MATCH_POOL::iterator iter;

	for (int32_t i = down_pool_pos; i < up_pool_pos; ++i)
	{
		if (m_match_pools[i].m_arena_match_pool.size() == 0)
			continue;
		// 随机选择一个
		find_target_pos = random_util_t::randMin(0, m_match_pools[i].m_arena_match_pool.size());
		if (find_target_pos == -1)
		{
			log_error("arena_manager_t find_target_pos error min[%d] max[%d]", 0, (int32_t)m_match_pools[i].m_arena_match_pool.size());
			continue;
		}
		iter = std::next(m_match_pools[i].m_arena_match_pool.begin(), find_target_pos);
		if (iter == m_match_pools[i].m_arena_match_pool.end())
		{
			log_error("iter = m_match_pools[i].m_arena_match_pool.end");
			continue;
		}
		p_match_target = iter->second;
		if (NULL == p_match_target)
		{
			log_error("arena p_match_target NULL pool_pos[%d] role_pos[%d]", i, find_target_pos);
			continue;
		}
		// 排除自己
		if(p_role->get_role_uid() == p_match_target->get_role_uid())
			continue;
		// 未达到(包含达到)匹配次数上限时不会匹配到上次战斗过的玩家
		if (p_role->get_match_times() < match_max_num && p_role->get_target_uid() == p_match_target->get_role_uid())
		{
			continue;
		}
		find_pool_pos = i;
		break;
	}

	uint32_t reply_code = errcode_enum::error_ok;
	// 发送给game_server匹配结果
	proto::server::rg_arena_match_reply msg_game;
	proto::server::rg_arena_match_reply msg_game_target;

	if (find_pool_pos == -1)
	{
		// 未匹配到人 增加匹配次数 添加到匹配池
		p_role->add_match_times();
		add_role(pool_pos, p_role);
		reply_code = errcode_enum::notice_no_match_role;
	}
	else
	{
		// 匹配成功 重置匹配次数 在匹配池中移除双方
		p_role->reset_match_times();
		p_role->set_target_uid(p_match_target->get_role_uid());
		// 移除对方
		remove_role(find_pool_pos, p_match_target->get_role_uid());
		// 移除自己
		if (p_role->get_in_pools_pos() != -1)
		{
			remove_role(p_role->get_in_pools_pos(), p_role->get_role_uid());
		}

		// 填充发送给自己所在game_server的消息包
		msg_game.set_target_score(p_match_target->get_score());
		msg_game.set_target_arena_level(p_match_target->get_arena_level());
		p_match_target->peek_data(msg_game.mutable_target_user());

		// 发送给traget_role所在的gamer_server匹配成功消息
		msg_game_target.set_server_id(p_match_target->get_role_server_id());
		msg_game_target.set_target_score(p_role->get_score());
		msg_game_target.set_target_arena_level(p_role->get_arena_level());
		msg_game_target.set_reply_code(reply_code);
		p_role->peek_data(msg_game_target.mutable_target_user());
		env::server->send_msg_to_transfer(op_cmd::rg_arena_match_reply, p_match_target->get_role_uid(), msg_game_target);
	}

	msg_game.set_server_id(p_role->get_role_server_id());
	msg_game.set_reply_code(reply_code);
	env::server->send_msg_to_transfer(op_cmd::rg_arena_match_reply, p_role->get_role_uid(), msg_game);

	// 要先发给客户端匹配消息后再发给cross进入战斗消息 否则在gate会过滤掉非战斗消息
	if (reply_code == errcode_enum::error_ok)
	{
		// 发送给cross服务器进行战斗
		proto::server::rs_arena_battle_notify msg_cross;
		proto::common::fight_param* p_fight_param = msg_cross.mutable_fight();
		if (p_fight_param) {
			p_fight_param->set_type(proto::common::fight_type_cross_arena_rank);

			proto::common::cross_arena_param* p_cross_arena_param = p_fight_param->mutable_cross_arena();
			if (p_cross_arena_param) {
				proto::common::cross_arena_ex* p_cross_arena_ex1 = p_cross_arena_param->mutable_obj1();
				if (p_cross_arena_ex1) {
					p_cross_arena_ex1->set_score(p_role->get_score());
					p_cross_arena_ex1->set_rank_lv(p_role->get_rank_lv());
				}

				proto::common::cross_arena_ex* p_cross_arena_ex2 = p_cross_arena_param->mutable_obj2();
				if (p_cross_arena_ex2) {
					p_cross_arena_ex2->set_score(p_match_target->get_score());
					p_cross_arena_ex2->set_rank_lv(p_match_target->get_rank_lv());
				}
			}
		}
		//msg_cross.mutable_fight()->set_type(proto::common::fight_type_cross_arena_rank);
		p_role->peek_data(msg_cross.mutable_user1());
		p_match_target->peek_data(msg_cross.mutable_user2());
		env::server->send_msg_to_cross(op_cmd::rs_arena_battle_notify, p_role->get_role_uid(), msg_cross);
	}
}

void arena_manager_t::deal_arena_battle_result(proto::server::sr_arena_battle_result_notify &msg)
{
	uint64_t win_obj_uid = string_util_t::string_to_uint64(msg.win_obj().object_uid());
	uint64_t lose_obj_uid = string_util_t::string_to_uint64(msg.lose_obj().object_uid());
	uint32_t ai_arena_score = 0;
	uint32_t fight_type = msg.fight().type();

	// 赢的是AI
	if (msg.win_obj().object_type() == proto::common::SCENEOBJECT_MONSTER)
	{
		win_obj_uid = msg.fight().cross_arena().arena_ai_id();
		arena_role_ptr user = arena_role_manager_t::find_role(lose_obj_uid);
		if (NULL == user)
		{
			log_error("arena lose_user[%lu] NULL", lose_obj_uid);
			return;
		}
		arena_ai_ptr p_ai = get_arena_ai((uint32_t)win_obj_uid);
		if (p_ai != NULL)
		{
			ai_arena_score = p_ai->get_score();
			p_ai->deal_battle_result(true);
			check_need_create_ai(p_ai, user);
		}
		deal_arena_battle_obj_result(user, false, ai_arena_score, user->get_fighting_value(), msg.lose_heros());
	}
	// 输的是AI
	else if (msg.lose_obj().object_type() == proto::common::SCENEOBJECT_MONSTER)
	{
		lose_obj_uid = msg.fight().cross_arena().arena_ai_id();
		arena_role_ptr user = arena_role_manager_t::find_role(win_obj_uid);
		if (NULL == user)
		{
			log_error("arena win_user[%lu] NULL", win_obj_uid);
			return;
		}
		arena_ai_ptr p_ai = get_arena_ai((uint32_t)lose_obj_uid);
		if (p_ai != NULL)
		{
			ai_arena_score = p_ai->get_score();
			p_ai->deal_battle_result(false);
			check_need_create_ai(p_ai, user);
		}
		deal_arena_battle_obj_result(user, true, ai_arena_score, user->get_fighting_value(), msg.win_heros());
	}
	// 两边都是玩家
	else
	{
		if (fight_type == proto::common::fight_type_world_cup)
		{
			world_cup_manager_t::battle_result(win_obj_uid, lose_obj_uid);
		}
		else
		{
			arena_role_ptr win_user = arena_role_manager_t::find_role(win_obj_uid);
			arena_role_ptr lose_user = arena_role_manager_t::find_role(lose_obj_uid);
			if (NULL == win_user)
			{
				log_error("arena win_user[%lu] NULL", win_obj_uid);
				return;
			}
			if (NULL == lose_user)
			{
				log_error("arena lose_user[%lu] NULL", lose_obj_uid);
				return;
			}

			deal_arena_battle_obj_result(win_user, true, lose_user->get_score(), lose_user->get_fighting_value(), msg.lose_heros());
			deal_arena_battle_obj_result(lose_user, false, win_user->get_score(), win_user->get_fighting_value(), msg.win_heros());
		}
	}	
}

void arena_manager_t::deal_role_logout(uint64_t role_uid)
{
	arena_role_ptr p_role = arena_role_manager_t::find_role(role_uid);
	if (NULL == p_role)
	{
		return;
	}
	// 移除自己
	if (p_role->get_in_pools_pos() != -1)
	{
		remove_role(p_role->get_in_pools_pos(), p_role->get_role_uid());
	}
}

void arena_manager_t::cancel_match(uint64_t role_uid)
{
	arena_role_ptr p_role = arena_role_manager_t::find_role(role_uid);
	if (NULL == p_role)
	{
		log_error("arena_manager p_role NULL uid[%lu]", role_uid);
		return;
	}

	// 重置匹配次数 在匹配池中移除自己
	p_role->reset_match_times();
	if (p_role->get_in_pools_pos() != -1)
	{
		remove_role(p_role->get_in_pools_pos(), role_uid);
	}

	proto::server::rg_arena_cancel_match_reply reply;
	reply.set_server_id(p_role->get_role_server_id());
	env::server->send_msg_to_transfer(op_cmd::rg_arena_cancel_match_reply, role_uid, reply);
}

void arena_manager_t::on_season(uint32_t id, bool is_in_time)
{
	if (is_in_time)
	{
		arena_score_rank_mgr_t::reset_stage();
	}
}

void arena_manager_t::on_close(uint32_t id, bool is_in_time)
{
	if (!is_in_time)
	{
		clear_match_pool();
	}
}

void arena_manager_t::deal_arena_battle_obj_result(arena_role_ptr user, bool is_win, uint32_t target_score, uint32_t fighting_value,
												   const proto::common::hero_data &self_heros)
{
	if (NULL == user)
	{
		log_error("arena user NULL");
		return;
	}

	uint32_t user_new_score = user->get_score() + get_battle_reward_score(user->get_score(), target_score, is_win);
	
	uint32_t user_new_level = get_level_by_score(is_win, user_new_score, user->get_arena_level());

	int user_add_rank = 0;
	int user_now_rank = 0;
	int user_old_rank = 0;
	
	user_old_rank = arena_score_rank_mgr_t::get_arena_rank(user->get_role_uid());

	// 填充上阵武将的装备数据
	proto::common::item_data hero_equips;
	proto::common::item_single *p_hero_equip = NULL;
	const proto::common::role_cross_data &cross_data = user->get_cross_data();
	uint64_t hero_equip_uid = 0;
	uint64_t equip_uid = 0;
	for (int32_t i = 0; i < self_heros.hero_list_size(); ++i)
	{
		const proto::common::hero_single &hero_single = self_heros.hero_list(i);
		
		for (int32_t j = 0; j < hero_single.base_attr().equips_size(); ++j)
		{
			hero_equip_uid = string_util_t::string_to_uint64(hero_single.base_attr().equips(j).equip_uid());

			for (int32_t k = 0; k < cross_data.item_data().item_list_size(); ++k)
			{
				equip_uid = string_util_t::string_to_uint64(cross_data.item_data().item_list(k).uid());
				if (hero_equip_uid == equip_uid)
				{
					const proto::common::item_single &equip_single = cross_data.item_data().item_list(k);
					p_hero_equip = hero_equips.add_item_list();
					p_hero_equip->CopyFrom(equip_single);
					break;
				}
			}
		}
	}

	const arena_rank_ptr& p_arena_rank = arena_score_rank_mgr_t::get_arena_rank_ptr();
	if (p_arena_rank) {
		user_now_rank = p_arena_rank->update_arena_score_rank(user->get_role_uid(), user_new_score, user_new_level,
			user->get_role_server_id(), user->get_role_server_name(), user->get_role_name(),
			fighting_value, self_heros, hero_equips);
	}
	
	user_add_rank = user_old_rank - user_now_rank;
	
	// 发送给user战斗结果
	proto::server::rg_arena_battle_result_notify reply;
	reply.set_is_win(is_win);
	reply.set_target_score(target_score);
//	reply.set_target_arena_level(user2->get_arena_level());
	reply.set_own_rank_up(user_add_rank);
	reply.set_own_new_rank(user_now_rank);
//	reply.mutable_target()->mutable_rd()->set_uid(string_util_t::uint64_to_string(target_uid));
	reply.set_server_id(user->get_role_server_id());
	env::server->send_msg_to_transfer(op_cmd::rg_arena_battle_result_notify, user->get_role_uid(), reply);
}

int32_t arena_manager_t::get_battle_reward_score(uint32_t own_score, uint32_t target_score, bool is_win)
{
	if (own_score == 0)
	{
		log_error("arena_manager_t own_score = 0");
		return 0;
	}
	auto score_basic_conf = GET_CONF(Comprehensive, comprehensive_common::arena_basic_score);
	auto score_turn_rank_conf = GET_CONF(Comprehensive, comprehensive_common::arena_score_turn_rank);
	auto score_turn_range_conf = GET_CONF(Comprehensive, comprehensive_common::arena_score_turn_range);
	auto score_turn_max_conf = GET_CONF(Comprehensive, comprehensive_common::arena_score_turn_max);
	auto score_turn_min_conf = GET_CONF(Comprehensive, comprehensive_common::arena_score_turn_min);
	auto conf_min = GET_CONF(ArenaClassTable, 1);

	int32_t basic_score = GET_COMPREHENSIVE_VALUE_1(score_basic_conf);
	int32_t score_turn_rank = GET_COMPREHENSIVE_VALUE_1(score_turn_rank_conf);
	int32_t score_turn_range = GET_COMPREHENSIVE_VALUE_1(score_turn_range_conf);
	int32_t score_turn_max = GET_COMPREHENSIVE_VALUE_1(score_turn_max_conf);
	int32_t score_turn_min = GET_COMPREHENSIVE_VALUE_1(score_turn_min_conf);
	uint32_t min_score = conf_min->score();

	int32_t score_dec = target_score * 100 / own_score;
	int32_t scode_add_rate = 0;

	// 判断是否在下行区间
	if (score_dec < 100 - score_turn_rank)
	{
		for (int32_t i = 100 - score_turn_rank; i > score_turn_min; i -= score_turn_rank)
		{
			if (score_dec < i)
			{
				scode_add_rate -= score_turn_range;
			}
		}

	}
	// 判断是否在上行区间
	else if (score_dec > 100 + score_turn_rank)
	{
		for (int32_t i = 100 + score_turn_rank; i <= score_turn_max; i += score_turn_rank)
		{
			if (score_dec > i)
			{
				scode_add_rate += score_turn_range;
			}
		}
	}

	int32_t	final_award_score = basic_score + basic_score * scode_add_rate / 100;
	final_award_score = is_win ? final_award_score : -final_award_score;

	// 不能低于最低分数
	if (final_award_score < 0 && own_score + final_award_score < min_score)
	{
		final_award_score = -(own_score - min_score);
	}

	return final_award_score;
}


uint32_t arena_manager_t::get_level_by_score(bool is_win, uint32_t score, uint32_t arena_level)
{
	// 如果在1段位并且输了 就不用重算段位了
	if (arena_level == 1 && !is_win)
		return arena_level;

	// 如果在最高段位 并且赢了 也不用重算段位了
	if (NULL == ArenaClassTableManager::getInstancePtr())
	{
		log_error("ArenaClassTableManager::getInstancePtr()");
		return 0;
	}
	uint32_t size = ArenaClassTableManager::getInstancePtr()->getSize();
	if (arena_level == size && is_win)
		return arena_level;

	// 赢了判断向上段位 输了判断向下段位
	if (is_win)
	{
		ArenaClassTable *p_config = GET_CONF(ArenaClassTable, arena_level + 1);
		if (NULL == p_config)
		{
			log_error("arena p_config NULL tid[%d]", arena_level + 1);
			return arena_level;
		}
		if (score >= p_config->score())
		{
			return arena_level + 1;
		}
	}
	else
	{
		ArenaClassTable *p_config = GET_CONF(ArenaClassTable, arena_level - 1);
		if (NULL == p_config)
		{
			log_error("arena p_config NULL tid[%d]", arena_level - 1);
			return arena_level;
		}
		if (score < p_config->score())
		{
			return arena_level - 1;
		}
	}
	return arena_level;
}

uint32_t arena_manager_t::add_role(uint32_t pool_pos, arena_role_ptr p_role)
{
	if (pool_pos >= m_match_pools.size())
	{
		log_error("arena pool_pos[%d] > m_match_pools_size[%d]", pool_pos, (int32_t)m_match_pools.size());
		return errcode_enum::notice_unknown;
	}
	// 安全校验 如已存在于一个pool中 不添加
	if (p_role->get_in_pools_pos() != -1)
	{
		return errcode_enum::error_ok;
	}

	p_role->set_in_pools_pos(pool_pos);

	arena_match_pool &pool = m_match_pools[pool_pos];
	pool.m_arena_match_pool.insert(std::make_pair(p_role->get_role_uid(), p_role));

	return errcode_enum::error_ok;
}

uint32_t arena_manager_t::remove_role(int32_t pool_pos, uint64_t role_uid)
{
	if (pool_pos >= (int32_t)m_match_pools.size() || pool_pos < 0)
	{
		log_error("arena pool_pos[%d] error m_match_pools_size[%d]", pool_pos, (int32_t)m_match_pools.size());
		return errcode_enum::notice_unknown;
	}

	arena_match_pool &pool = m_match_pools[pool_pos];
	ARENA_MATCH_POOL::iterator iter = pool.m_arena_match_pool.find(role_uid);
	if (iter == pool.m_arena_match_pool.end())
	{
		log_error("arena role_uid[%lu] not find", role_uid);
		return errcode_enum::notice_unknown;
	}

	arena_role_ptr p_arena_role = iter->second;
	if (NULL == p_arena_role)
	{
		log_error("arena p_arena_role NULL");
		return errcode_enum::notice_unknown;
	}

	p_arena_role->set_in_pools_pos(-1);
	pool.m_arena_match_pool.erase(iter);

	return errcode_enum::error_ok;
}

arena_ai_ptr arena_manager_t::get_arena_ai(uint32_t id)
{
	arena_ai_map::iterator iter = m_ai_map.find(id);
	if (iter != m_ai_map.end())
		return iter->second;
	return arena_ai_ptr();
}

void arena_manager_t::check_need_create_ai(arena_ai_ptr p_ai, arena_role_ptr target)
{
	if (NULL == p_ai)
	{
		log_error("arena p_ai NULL");
		return;
	}
	if (NULL == target)
	{
		log_error("arena target NULL");
		return;
	}

// 	if (m_dynamic_create_ai_count > DYNAMIC_AI_CREATE_MAX_NUM)
// 	{
// 		WORLDCUP_LOG("arena dynamic ai num[%d] > [%d]", m_dynamic_create_ai_count, DYNAMIC_AI_CREATE_MAX_NUM);
// 		return;
// 	}

	auto arena_ai_max_score_conf = GET_CONF(Comprehensive, comprehensive_common::arena_ai_max_score);
	auto arena_ai_min_score_conf = GET_CONF(Comprehensive, comprehensive_common::arena_ai_min_score);
	auto arena_ai_delete_lose_times_conf = GET_CONF(Comprehensive, comprehensive_common::arena_ai_delete_lose_times);
	auto arena_ai_delete_win_times_conf = GET_CONF(Comprehensive, comprehensive_common::arena_ai_delete_win_times);
	auto arena_ai_born_times_conf = GET_CONF(Comprehensive, comprehensive_common::arena_ai_born_times);

	uint32_t arena_ai_max_score = GET_COMPREHENSIVE_VALUE_1(arena_ai_max_score_conf);
	uint32_t arena_ai_min_score = GET_COMPREHENSIVE_VALUE_1(arena_ai_min_score_conf);
	uint32_t arena_ai_delete_lose_times = GET_COMPREHENSIVE_VALUE_1(arena_ai_delete_lose_times_conf);
	uint32_t arena_ai_delete_win_times = GET_COMPREHENSIVE_VALUE_1(arena_ai_delete_win_times_conf);
	uint32_t arena_ai_born_times = GET_COMPREHENSIVE_VALUE_1(arena_ai_born_times_conf);

	// 动态AI 输赢一定次数从匹配池移除 同时放回动态AI内存池
	if (p_ai->is_dynamic())
	{
		if (p_ai->get_lose_times() >= arena_ai_delete_lose_times || p_ai->get_win_times() >= arena_ai_delete_win_times)
		{
			if (p_ai->get_pools_pos() >= 0 && p_ai->get_pools_pos() < (int32_t)m_ai_match_pools.size())
			{
				arena_ai_match_pool &match_pool = m_ai_match_pools[p_ai->get_pools_pos()];
				if (p_ai->get_pool_pos() >= 0 && p_ai->get_pool_pos() < (int32_t)match_pool.m_arena_ai_match_pool.size())
				{
					push_back_arena_ai_to_cache(p_ai);
					match_pool.m_arena_ai_match_pool.erase(match_pool.m_arena_ai_match_pool.begin() + p_ai->get_pool_pos());
				}
			}
		}
		return;
	}

	// 静态AI 达到条件创建动态AI
	if (p_ai->get_score() < arena_ai_min_score || p_ai->get_score() > arena_ai_max_score)
		return;

	if (p_ai->get_lose_times() < arena_ai_born_times)
		return;
	
	// 重置静态AI的胜负场
	p_ai->reset_battle_data();
	// 创建动态AI
	arena_ai_ptr p_new_ai = pop_arena_ai_from_cache();
	if (NULL == p_new_ai)
	{
		WORLDCUP_LOG("arena pop arena_ai fail");
		return;
	}
	// 设置动态AI的竞技场数据 hero从刚战斗的玩家英雄上copy
	p_new_ai->set_dynamic_ai_data(p_ai->get_score(), p_ai->get_arena_level(), p_ai->get_name(), p_ai->get_level());
	proto::common::mirror_fight_data npc_data;
	npc_data.mutable_obj()->set_object_uid(string_util_t::uint32_to_string(p_new_ai->get_id()));
	npc_data.mutable_obj()->set_object_type(proto::common::SCENEOBJECT_MONSTER);
	npc_data.mutable_obj()->set_name(p_ai->get_name());
	npc_data.mutable_obj()->set_head_id(target->get_cross_data().rd().obj_info().plugin());
	npc_data.mutable_heros()->CopyFrom(target->get_cross_data().hd());
	p_new_ai->set_data(npc_data);

	// 计算应该添加到哪个匹配池
	auto conf_min = GET_CONF(ArenaClassTable, 1);
	auto conf_match = GET_CONF(Comprehensive, comprehensive_common::arena_match_pool_interval_score);

	uint32_t min_score = conf_min->score();
	uint32_t match_pool_interval_score = GET_COMPREHENSIVE_VALUE_1(conf_match);
	if (match_pool_interval_score == 0)
	{
		log_error("match_pool_interval_score = 0");
		return;
	}

	int32_t pool_pos = (p_ai->get_score() - (int32_t)min_score) / (int32_t)match_pool_interval_score;
	if (pool_pos < 0 || pool_pos >= (int32_t)m_ai_match_pools.size())
	{
		log_error("arena create dynamic ai fail id[%d] score[%d]", p_ai->get_tid(), p_ai->get_score());
		push_back_arena_ai_to_cache(p_new_ai);
		return;
	}
	// 添加到匹配池
	m_ai_match_pools[pool_pos].m_arena_ai_match_pool.push_back(p_new_ai);
	m_ai_map.insert(std::make_pair(p_new_ai->get_id(), p_new_ai));
	// 记录匹配池信息
	p_new_ai->set_pools_pos(pool_pos);
	p_new_ai->set_pool_pos(m_ai_match_pools[pool_pos].m_arena_ai_match_pool.size() - 1);
}

arena_ai_ptr arena_manager_t::pop_arena_ai_from_cache()
{
	if (m_ai_cache_vec.size() == 0)
	{
		WORLDCUP_LOG("arena m_ai_cache_vec size = 0");
		return arena_ai_ptr();
	}

	arena_ai_ptr p_ai = m_ai_cache_vec[m_ai_cache_vec.size() - 1];
	m_ai_cache_vec.pop_back();
	return p_ai;
}

void arena_manager_t::push_back_arena_ai_to_cache(arena_ai_ptr p_ai)
{
	if (NULL == p_ai)
		return;
	if (m_ai_cache_vec.size() >= DYNAMIC_AI_CREATE_MAX_NUM)
		return;

	p_ai.reset();
	m_ai_cache_vec.push_back(p_ai);
}

void arena_manager_t::clear_match_pool()
{
	arena_role_ptr p_role = arena_role_ptr();
	ARENA_MATCH_POOL::iterator iter;

	for (uint32_t i = 0; i < m_match_pools.size(); ++i)
	{
		iter = m_match_pools[i].m_arena_match_pool.begin();
		for (; iter != m_match_pools[i].m_arena_match_pool.end(); ++iter)
		{
			p_role = iter->second;
			if (p_role == NULL)
			{
				continue;
			}
			p_role->reset_match_times();
			p_role->set_in_pools_pos(-1);
		}
		m_match_pools[i].m_arena_match_pool.clear();
	}
	m_match_pools.clear();
}
