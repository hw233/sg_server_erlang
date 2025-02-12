#ifndef __ACTIVITY_MANAGER_H__
#define __ACTIVITY_MANAGER_H__

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "common.pb.h"
#include "client.pb.h"
#include "macros.hpp"
#include "activity.hpp"

enum liveness_box_state
{
	liveness_box_state_none = 0,	//未达到领取条件
	liveness_box_state_finish = 1,	//达到领取条件未领取
	liveness_box_state_over	= 2,	//已领取
};

enum activity_type
{
	activity_country_bounty = 1,	//国家悬赏
	activity_boss			= 2,	//魔王入侵
	activity_arena			= 3,	//竞技场
	activity_trade			= 4,	//丝绸之路
	activity_master_task	= 5,	//师门任务
	activity_treasure		= 6,	//寻宝活动
	activity_tower			= 16,	//千层塔
	activity_expedition		= 21,	//远征
	activity_shilian		= 22,	//试炼任务
	activity_zhuogui		= 23,	//捉鬼任务
	activity_jingyan		= 24,	//经验环任务
};

enum { // 活动大类型
	activity_normal = 0,
	activity_country = 1,
	activity_family = 2,
	activity_max,
};

class activity_manager_t : public boost::enable_shared_from_this<activity_manager_t>
{
public:
	activity_manager_t(uint64_t role_uid);
    ~activity_manager_t() {}

public:
	void load_data(const proto::common::activities_data& activities_data);
	void peek_data(proto::common::activities_data* p_activities_data);
	void peek_family_activity(proto::client::gc_get_family_activities_reply& reply);
	void save_self();
	void one_day();
	void one_week();
public:
	bool liveness_box(uint32_t box_id, proto::common::role_change_data* p_data);
	//活动有没有开启（针对玩家）
	bool check_open(uint32_t activity_id);
	//完成活动
	void activity_done(uint32_t activity_id);
	//活动有没有到开启时间（针对全服）
	static bool check_time_open(uint32_t activity_id);
	//用于红点判断
	bool is_already_open_box(uint32_t liveness);
	//用于GM命令
	bool add_liveness_count(uint32_t val);

	uint32_t get_liveness_count() { return m_liveness_count; }

	uint32_t get_activity_count(uint32_t activity_id);

	bool is_finish_all_circle(uint32_t activity_id);
private:
	std::string m_key = "";
	uint64_t m_role_uid = 0;
	std::map<uint32_t, activity_ptr>	m_activity_map;
	uint32_t m_liveness_count = 0;		//活跃度
	std::vector<uint32_t> m_box_vec;	//已领取的活跃度宝箱
};
typedef boost::shared_ptr<activity_manager_t> activity_mgr_ptr;
#endif // !__ACTIVITY_MANAGER_H__

