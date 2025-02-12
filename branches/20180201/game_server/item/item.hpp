#ifndef __GAME_ITEM_H__
#define __GAME_ITEM_H__

#include "protos_fwd.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include "utility.hpp"

USING_NS_COMMON;
// 物品使用地点数组大小
#define item_coordinate_max_num	3 
#define equip_prefix 21

// 使用类型
enum use_item_cond_type
{
    use_item_cond_type_level = 1,
    use_item_cond_type_count
};

// 分类类型（item:big_type）
enum item_big_type
{
	item_big_type_none		= 0, // 无效
	item_big_type_item		= 1, // 道具
	item_big_type_equip		= 2, // 装备
	item_big_type_ordnance	= 3, // 军械
	item_big_type_life		= 4, // 命格
	item_big_type_task		= 5, // 任务
	item_big_type_fashion	= 8, // 时装
	item_big_type_trade		= 9, // 跑商
    item_big_type_money		= 10,// 货币
};

enum bind_type
{
	bind_type_get = 1,
	bind_type_appraise = 2,
	bind_type_equip = 3,
	bind_type_goods = 4,
};

enum item_state
{
	item_state_none = 0,
	item_state_bind = 1,
	item_state_cooldown = 2,
};

class item_t: public boost::enable_shared_from_this<item_t>, private boost::noncopyable
{
public:
    item_t();
    item_t(uint64_t uid, uint32_t tid, int32_t cur_num, const std::string& role_name, uint32_t param);
    ~item_t();

    void load_data(const proto::common::item_single& single);
    void peek_data(proto::common::item_single* p_single);
    void save_self(uint64_t role_uid, bool is_new = false, bool is_right_now = false);

    std::string get_key() const { return m_key; }

    uint64_t get_uid() const { return m_uid; }
    void set_uid(uint64_t val) { m_uid = val; }

    uint32_t get_tid() const { return m_tid; }
    void set_tid(uint32_t val) { m_tid = val; }

    int32_t get_cur_num() const { return m_cur_num; }
    void set_cur_num(int32_t val) { m_cur_num = val; }

	uint32_t get_package() const { return m_package; }
	void set_package(uint32_t package) { m_package = package; }

	uint32_t get_need_grid();

	void set_param(uint32_t param) { m_param = param; }
	uint32_t get_param() const { return m_param; }

	void set_param2(uint32_t param) { m_param2 = param; }
	uint32_t get_param2() const { return m_param2; }

	void set_find_role(std::string role_name) { m_find_role = role_name; }
	std::string get_find_role() { return m_find_role; }

	void set_state(uint32_t val) { m_state = val; }
	uint32_t get_state() const { return m_state; }

	uint32_t get_rarity() { return m_rarity; }
	void set_expired_time(uint32_t val) { m_expired_time = val; }
	uint32_t get_expired_time() const { return m_expired_time; }
	//equip
	bool is_equip();
	void set_hero_uid(uint64_t hero_uid) { m_hero_uid = hero_uid; }
	uint64_t get_hero_uid() { return m_hero_uid; }

	uint32_t get_remake_exattr_bonus() { return m_remake_exattr_bonus; }
	void set_remake_exattr_bonus(uint32_t val) { m_remake_exattr_bonus = val; }

	void set_attr(const std::map<uint32_t, uint32_t>& attr_map);

	void set_exattr(const std::vector<uint32_t>& exattr_vec);
	uint32_t get_exattr(uint32_t index);
	void set_remake_exattr(const std::vector<uint32_t>& exattr_vec);
	void replace_exattr();

	uint32_t get_name() { return m_name; }
	void set_name(uint32_t val) { m_name = val; }

	uint32_t get_special() { return m_special; }
	void set_special(uint32_t val) { m_special = val; }

	uint32_t get_strengthen_level() { return m_strengthen_level; }
	void set_strengthen_level(uint32_t val) { m_strengthen_level = val; }

	uint32_t get_strmaster_level() { return m_strmaster_level; }
	void set_strmaster_level(uint32_t val) { m_strmaster_level = val; }

	uint32_t get_remake_material() { return m_remake_material; }
	void set_remake_material(uint32_t val) { m_remake_material = val; }

	uint32_t get_lock_material() { return m_lock_material; }
	void set_lock_material(uint32_t val) { m_lock_material = val; }

	uint32_t get_strengthen_material() { return m_strengthen_material; }
	void set_strengthen_material(uint32_t val) { m_strengthen_material = val; }

	uint32_t get_strengthen_material_ex() { return m_strengthen_material_ex; }
	void set_strengthen_material_ex(uint32_t val) { m_strengthen_material_ex = val; }

	uint32_t get_strengthen_money() { return m_strengthen_money; }
	void set_strengthen_money(uint32_t val) { m_strengthen_money = val; }

	void set_score(uint32_t score) { m_score = score; }
	void set_remake_score(uint32_t score) { m_remake_score = score; }
	void calc_score();
	void calc_remake_score();
// 	uint32_t calc_special_score();
// 	uint32_t calc_suit_score();
	void calc_attr(std::map<uint32_t, int>& attrs);
	void calc_remake_attr(std::map<uint32_t, int>& attrs);

	void fill_equip_log(proto::log::go_equip_log_notify& msg);

	void peek_goods_item(proto::common::goods_item* p_goods_item);

	void check_rarity();

	void check_state();

	uint32_t get_equip_lucky();

	bool is_clear_equip_lucky();
public:
	//护甲评分/战力
	static uint32_t get_hp_score(std::map<uint32_t, int>& attrs);
	//输出评分/战力
	static uint32_t get_atk_score(std::map<uint32_t, int>& attrs);
	//输出评分/战力（没有武器的情况下）
	static uint32_t get_atk_score_without_weapon(std::map<uint32_t, int>& attrs);
	
	//武将基础格挡率
	static uint32_t get_base_block_rate();
	//武将基础格挡加成
	static uint32_t get_base_block_effect();
	//武将面板破击加成上限值
	static uint32_t get_base_block_effect_reduce_max();
	//效果命中输出能力价值系数
	static float get_effect_hit_number();
	//先手战力价值系数 
	static float get_speed_switch_power();
	//武将基础暴击率  
	static uint32_t get_base_crit_rate();
	//武将基础暴击伤害
	static uint32_t get_base_crit_damage();
	//武将面板抗暴倍数上限值 
	static uint32_t get_critical_fight_max();
	//武将面板格挡率上限值 
	static uint32_t get_parry_base_max();
	//武将面板格挡加成上限值 
	static uint32_t get_parry_add_base_max();
	//武将面板闪避率上限值 
	static uint32_t get_duck_max();
	//武将面板效果闪避率上限值 
	static uint32_t get_effect_duck_max();
	//效果闪避生存能力价值系数 
	static float get_effect_duck_number();
	//防御转生命价值比 
	static float get_defence_switch_hp();
	//有效生命价值系数 
	static float get_hp_effective();
	//有效输出价值系数 
	static float get_ad_effective();
	//装备默认攻击浮动 
	static float get_base_attack_range();
	//破击效果评分参数1
	static uint32_t get_block_param1();
	//破击效果评分参数2
	static uint32_t get_block_param2();
	//武器基础暴击率  
	static uint32_t get_weapon_crit_rate();
	//武器基础暴击伤害
	static uint32_t get_weapon_crit_damage();
	//装备珍品最小等级
	static uint32_t get_min_rarity_level();
	//最小带特技的珍品品质
	static uint32_t get_min_special_quality();
	//最小带前缀的珍品品质
	static uint32_t get_min_name_quality();
	static uint32_t get_equip_make_star_constant();
	static uint32_t get_equip_make_quality_constant();
	static uint32_t get_equip_make_special_constant();
	static uint32_t get_equip_make_lucky_quality();
private:
    std::string m_key = "";
	std::string m_find_role = "";
    uint64_t m_uid = 0;
    uint32_t m_tid = 0;
    int32_t m_cur_num = 0;
	uint32_t m_weared = 0;
	uint32_t m_package = 0;
	uint32_t m_param = 0;
	uint32_t m_param2 = 0;
	uint64_t m_hero_uid = 0;
	std::map<uint32_t, uint32_t> m_attrs;
	std::vector<uint32_t> m_exattrs;
	std::vector<uint32_t> m_remake_exattrs;
	uint32_t m_remake_exattr_bonus = 0;
	uint32_t m_name = 0;
	uint32_t m_special = 0;
	uint32_t m_strengthen_level = 0;
	uint32_t m_strmaster_level = 0;
	uint32_t m_remake_material = 0;
	uint32_t m_lock_material = 0;
	uint32_t m_strengthen_material = 0;
	uint32_t m_strengthen_material_ex = 0;
	uint32_t m_strengthen_money = 0;
	uint32_t m_score = 0;
	uint32_t m_remake_score = 0;
	uint32_t m_rarity = 0;
	uint32_t m_state = 0;
	uint32_t m_expired_time = 0;

};
typedef boost::shared_ptr<item_t> item_ptr;
typedef std::map<uint64_t, item_ptr> item_map;

#endif