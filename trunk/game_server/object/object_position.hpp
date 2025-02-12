#ifndef __GAME_OBJECT_POSITION_H__
#define __GAME_OBJECT_POSITION_H__

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "utility.hpp"
#include "common.pb.h"
#include "server.pb.h"
#include "scene/scene_common.hpp"
#include "network/protobuf.hpp"

#define  DEFAULT_RADIUS 5

// 前置声明
class object_base_t;
class role_t;
class monster_t;
class scene_t;
class collect_point_t;
class partner_t;
class formation_t;
class hero_t;
class item_t;


// 类型定义
typedef std::pair<uint64_t, proto::common::EM_SCENEOBJECT_TYPE> object_id_type;
typedef boost::shared_ptr<object_base_t> object_base_ptr;
typedef boost::shared_ptr<role_t> role_ptr;
typedef boost::shared_ptr<monster_t> monster_ptr;
typedef boost::shared_ptr<scene_t> scene_ptr;
typedef boost::shared_ptr<collect_point_t> collect_ptr;
typedef boost::shared_ptr<partner_t> partner_ptr;
typedef boost::shared_ptr<hero_t> hero_ptr;
typedef boost::shared_ptr<item_t> item_ptr;
typedef std::map<uint64_t, hero_ptr> hero_map;

// 位置类
class object_base_t : public boost::enable_shared_from_this<object_base_t>
{
public:
	object_base_t();
	object_base_t(uint64_t unique_id);
	virtual ~object_base_t();

	// 信息相关
public:
	void init(uint64_t unique_id);
	virtual void del_clear() {};

	// 战斗信息相关
public:
	virtual bool fill_fight_camp(
		uint32_t camp_index,
		uint64_t unique_offset,
		uint32_t team_member_size,
		proto::common::fight_camp* camp,
		proto::common::fight_type fight_type,
		proto::common::hero_state_data* state,
		int32_t team_member_pos = -1 )
	{
		return false;
	}

	// 填充武将列表
	virtual bool fill_hero_data(
		uint32_t camp_index,
		uint64_t unique_offset,
		uint32_t team_member_size,
		proto::common::fight_type fight_type,
		proto::common::hero_data* fhd,
		proto::common::hero_state_data* state = NULL,
		int32_t team_member_pos = -1)
	{
		return false;
	}




	virtual bool get_hero_list(uint32_t fight_type, uint32_t team_member_size, std::vector<uint64_t>& list)
	{
		return false;
	}

	virtual hero_ptr get_hero_by_unique_id( uint64_t unique_id)
	{
		return hero_ptr(NULL);
	}

	virtual void set_fight_uid(uint64_t uid)
	{
		
	}

	virtual bool is_auto_ignore_lineup(uint32_t fight_type)
	{
		return true;
	}

	virtual void check_update_fighting(uint64_t hero_uid)
	{
	
	}

	virtual bool is_ready() 
	{ 
		return true; 
	}

	virtual uint32_t get_camp_unique_offset( uint32_t camp )
	{
		return 0;
	}

	virtual item_ptr get_item_by_uid(uint64_t uid, proto::common::package_type package)
	{
		return NULL;
	}

	virtual hero_map* get_hero_list()
	{
		return NULL;
	}

	// 获取队伍列表
	virtual bool get_team_member_list(std::vector<object_base_ptr> &team_member_list)
	{
		return false;
	}

	// 判断是否队长
	virtual bool is_team_leader()
	{
		return false;
	}

	virtual uint32_t get_level() 
	{
		return 1;
	}

	virtual uint64_t get_main_hero_uid() 
	{ 
		return 0; 
	}

	virtual uint32_t get_main_hero_tid()
	{
		return 0;
	}

public:
	// 生物uid
	inline uint64_t get_object_id() const { return m_object_id; }
	void set_object_id(uint64_t object_id) { m_object_id = object_id; }

	// 得到基类指针
	object_base_ptr get_object_ptr() { return shared_from_this(); }
	// 是否是角色
	virtual bool is_role() { return false; }
	// 是否是怪物
	virtual bool is_monster() { return false; }
	// 是否是伙伴
	virtual bool is_partner() { return false; }
	// 获取对象类型
	virtual proto::common::EM_SCENEOBJECT_TYPE get_object_type() = 0;
	// Role指针
	virtual role_ptr get_role() { return role_ptr(); }
	// Monster指针
	virtual monster_ptr get_monster() { return monster_ptr(); }
	// 伙伴指针
	virtual partner_ptr get_partner() { return partner_ptr(); }
	// 比较函数
	inline bool object_equals(const object_id_type& object) { return m_object_id == object.first && get_object_type() == object.second; }
	// 生物uid，类型
	object_id_type get_object_id_type() { return object_id_type(m_object_id, get_object_type()); }

	virtual std::string get_name() { return ""; }
public:
	// 获取附近的生物信息（可以只获取玩家）
	bool get_nearby_objects(std::vector<object_id_type>& objects_nearby, bool role_only = false, float distance = 0);

	// 场景相关
public:
	// 生物进入视野
	virtual void other_object_into_vision(const object_id_type& object) {};
	// 通知离开视野
	virtual void other_object_out_of_vision(const object_id_type& object) {};
	// 移动函数（）
	virtual void move_to(float x, float y, uint64_t time_stamp, float direction, uint32_t high = 0) {};
	// 扫描附近的敌对生物
	virtual object_id_type scanning_can_attack_object(float radius) { return object_id_type(0, proto::common::SCENEOBJECT_USER); }
	// 是否在地图的某个点附近
	bool is_nearby(uint32_t map_id, float x, float y, float r = DEFAULT_RADIUS);
	// 是在某个点附近
	bool is_nearby_point(std::pair<float, float> point, float r);
	// 是在某个点附近
	bool is_nearby_point(std::pair<uint32_t, uint32_t> point, uint32_t r);
	// 是否是附近的生物
	bool is_nearby_object(object_base_ptr p_target, float radius);

	/*场景处理*/
public:
	// 是否在场景中
	bool is_in_scene() const { return m_is_in_scene; }
	// 离开场景
	void leave_scene();
	// 进入场景（各自去实现）
	virtual void on_enter_scene(scene_ptr p_scene) { m_is_in_scene = true; }
	// 离开场景
	virtual void on_leave_scene(scene_ptr p_scene) { m_is_in_scene = false; }
	// 移动处理
	virtual void on_move_event(scene_ptr p_scene) { };
	// 9屏同步
	virtual void notify_object_common_info_for_nearbies() {};

	// 9屏消息发送
	template<typename T_MSG>
	void send_msg_to_nine(uint16_t cmd, const T_MSG& msg);
	void send_msg_to_nine(uint16_t cmd, const std::string& cmd_data);

	// 场景熟悉相关
public:
	inline uint64_t get_scene_id() const { return m_scene_id; }
	void set_scene_id(uint64_t scene_id) { m_scene_id = scene_id; }

	inline uint32_t get_map_tid() const { return m_map_tid; }
	void set_map_tid(uint32_t map_tid) { m_map_tid = map_tid; }

	proto::server::em_scene_type get_scene_type() const { return m_scene_type; }
	void set_scene_type(proto::server::em_scene_type type) { m_scene_type = type; }
	bool is_in_dungeon() { return m_scene_type == proto::server::em_scene_type_dugeon; }

	void set_current_position(float x, float y, float dir = 0.f, uint64_t time = 0, uint32_t high = 0);
	void set_scene_info(uint64_t scene_id, uint32_t map_tid, proto::server::em_scene_type scene_type);

	void set_direction(float dir) { m_direction = dir; }
	inline float get_direction() const { return m_direction; }

	inline float get_current_x() const { return m_current_x; }
	inline float get_current_y() const { return m_current_y; }

	inline uint64_t get_time_stamp() const { return m_time_stamp; }

	void set_nine_key(uint32_t key) { m_nine_key = key; }
	inline uint32_t get_nine_key() const { return m_nine_key; }

    void set_real_speed(uint32_t speed) { m_real_speed = speed; }
    uint32_t get_real_speed() { return m_real_speed; }

	// 获取对象场景属性
	virtual void get_object_info(proto::common::object_common_info* object_info);
	void get_object_pos_info(proto::client::gc_move_notify* pos_info);
	
	/**
	 * \brief 状态相关
	 */
public:
	virtual void set_object_state(proto::common::object_state_type newstate) { m_object_state = newstate; }
	inline proto::common::object_state_type get_object_state() const { return m_object_state; }

	bool is_die() const { return m_object_state == proto::common::object_state_death; }

	/**
	 * \brief 设置出生回调
	 * \param father:召唤者
	 */
	virtual void set_object_born(uint64_t father = 0);
	virtual void on_object_born();
	/**
	* \brief 设置死亡
	* \param obj_killer:谁杀了你
	*/
	virtual void set_object_die(uint64_t obj_killer = 0);
	virtual void on_object_die();
	/**
	* \brief 被复活回调
	* \param obj_god:谁复活了你
	*/
	virtual void set_object_revive(uint64_t obj_god = 0);
	virtual void on_object_revive();

	virtual uint32_t get_head_id() { return 0;  }
public:
	virtual const proto::common::hero_data &get_heros() { return m_hero_data; }

	proto::common::hero_data m_hero_data;


protected:
	// 生物uid
	uint64_t m_object_id = 0;
	// 位置相关
	float m_direction = 0.f;
	float m_current_x = 0.f;
	float m_current_y = 0.f;
	uint32_t m_current_high = 0; // 高度，只做转发
	uint64_t m_time_stamp = 0;

	uint32_t m_real_speed = 0;

protected:
	// 场景相关
	uint64_t m_scene_id = 0;
	uint32_t m_map_tid = 0;
	bool m_is_in_scene = false;	// 是否在场景
	proto::server::em_scene_type m_scene_type = proto::server::em_scene_type_none;
	uint32_t m_nine_key = 0;	// 当前场景9屏索引
	
protected:
	proto::common::object_state_type m_object_state = proto::common::object_state_none;
};


template<typename T_MSG>
void object_base_t::send_msg_to_nine(uint16_t cmd, const T_MSG& msg)
{
//	size_t msg_sz = msg.ByteSize();
//	if (msg_sz > MAX_TCP_DATA_SIZE)
//	{
//		log_error("cmd[%s:%d] send len[%d] > max_len[%d]", CMD_DESC(cmd), cmd, msg_sz, MAX_TCP_DATA_SIZE);
//		return;
//	}

	std::string cmd_data;
	network::wrap_msg(cmd_data, msg);

	send_msg_to_nine(cmd, cmd_data);
}

inline
void object_base_t::set_current_position(float x, float y, float dir /*= 0.f*/, uint64_t time /*= 0*/, uint32_t high /*= 0*/)
{
	m_current_x = x;
	m_current_y = y;
	m_direction = dir;
	m_time_stamp = time;
	m_current_high = high;
}

inline
void object_base_t::set_scene_info(uint64_t scene_id, uint32_t map_tid, proto::server::em_scene_type scene_type)
{
	m_scene_id = scene_id;
	m_map_tid = map_tid;
	m_scene_type = scene_type;
}

#endif // !__GAME_OBJECT_POSITION_H__
