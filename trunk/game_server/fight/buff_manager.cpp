#include "buff_manager.hpp"
#include "tblh/BuffTable.tbls.h"
#include "tblh/BuffBigType.tbls.h"
#include "tblh/BuffSmallType.tbls.h"
#include "common/config_mgr.h"
#include "fight_hero.hpp"

std::map<uint32_t, elenment_func> buff_manager_t::m_funclist;

buff_manager_t::buff_manager_t()
{
}

buff_manager_t::~buff_manager_t()
{
	close();
}

bool buff_manager_t::init()
{
	return buff_element_t::regist(m_funclist);
}

void buff_manager_t::close()
{
	m_buffs.clear();
	m_special_buffs.clear();
	m_remove_set.clear();
	m_owner.reset();
}

void buff_manager_t::set_owner(fight_hero_ptr owner)
{
	m_owner = owner;
}

void buff_manager_t::change_buff_round(buff_ptr buff, const int change)
{
	if (!buff || change == 0)
		return;

	if (change > 0)
	{
		buff->add_round(change);
		get_owner()->add_combat_buff_act(proto::common::combat_act_type_buff_update, buff->get_id(), buff->get_tid(), buff->get_layer(), buff->get_round(), buff->get_attacker());
	}
	else
	{
		buff->sub_round(-change);
		// 判断回合数是否没有，没有则删除
		if (buff->get_round() == 0)
		{
			remove_buff(buff,buff->get_attacker());
		}
		else
		{
			get_owner()->add_combat_buff_act(proto::common::combat_act_type_buff_update, buff->get_id(), buff->get_tid(), buff->get_layer(), buff->get_round(), buff->get_attacker());
		}
	}
}

bool buff_manager_t::buff_safe_check()
{
	if (get_owner() != NULL && get_owner()->get_combat() != NULL)
	{
		if (get_owner()->get_combat()->get_round_add_buff_count() > 100)
		{
			return false;
		}
	}
	return true;
}

void buff_manager_t::change_buff_round_by_type(const int change, buff_effect_type type)
{
	if (change == 0)
		return;
	buff_vec copy(m_buffs);

	buff_ptr p_buff = buff_ptr();
	buff_vec::iterator it;
	for (it = copy.begin(); it != copy.end(); ++it)
	{
		p_buff = *it;
		if (p_buff == NULL)
			continue;
		if (p_buff->get_effect_type() != type)
			continue;
		change_buff_round(p_buff,change);
	}
}

void buff_manager_t::change_debuff_round(const int change)
{
	change_buff_round_by_type( change, buff_effect_type_debuff );
}

void buff_manager_t::change_profit_buff_round(const int change)
{
	change_buff_round_by_type(change, buff_effect_type_gain);
}

void buff_manager_t::detonate_buff(const int element_type, const fight_hero_ptr p_event_owner,  bool is_remove )
{
	if (NULL == get_owner()) {
		log_error("owner is null");
		return;
	}

	buff_vec copy(m_buffs);

	buff_ptr buff = buff_ptr();
	buff_vec::iterator it;
	for (it = copy.begin(); it != copy.end(); ++it)
	{
		buff = *it;
		if (buff == NULL)
			continue;
		// 判断是否存在该元素
		if(!buff->has_element(element_type))
			continue;
		// 引爆阶段
		//buff->set_step(buff_step_unload);	add by hy 
		buff->set_step(buff_step_detonate);

		get_owner()->add_combat_buff_act(proto::common::combat_act_type_detonate, buff->get_id(), buff->get_tid(), buff->get_layer(), buff->get_round(), buff->get_attacker());

		run_buff(buff, 0 ,  p_event_owner);
		
		// 引导后移除
		if( is_remove )
			remove_buff(buff, buff->get_attacker());
	}
}

void buff_manager_t::detonate_buff_by_smalltype( const uint32_t buff_small_type, const fight_hero_ptr p_event_owner, bool is_remove ) {
	if (NULL == get_owner()) {
		log_error("owner is null");
		return;
	}

	buff_vec copy(m_buffs);

	buff_ptr buff = buff_ptr();
	buff_vec::iterator it;

	for (it = copy.begin(); it != copy.end(); ++it)
	{
		buff = *it;
		if (buff == NULL)
			continue;

		// 判断是否存在该元素
		if ( buff_small_type != buff->get_small_type() )
			continue;

		buff->set_step(buff_step_detonate);

		get_owner()->add_combat_buff_act(proto::common::combat_act_type_detonate, buff->get_id(), buff->get_tid(), buff->get_layer(), buff->get_round(), buff->get_attacker());

		run_buff(buff, 0, p_event_owner);

		// 引导后移除
		if (is_remove)
			remove_buff(buff, buff->get_attacker());
	}
}


void buff_manager_t::change_buff_round(const int buff_tid, const int change)
{
	if (change == 0)
		return;
	buff_vec copy(m_buffs);

	buff_ptr p_buff = buff_ptr();
	buff_vec::iterator it;
	for (it = copy.begin(); it != copy.end(); ++it)
	{
		p_buff = *it;
		if (p_buff == NULL)
			continue;
		if ((int)p_buff->get_tid() != buff_tid)
			continue;
		change_buff_round(p_buff, change);
	}
}

void buff_manager_t::on_dead()
{
	remove_buff_when_dead();
}

void buff_manager_t::on_revive()
{
	remove_buff_when_revive();
}

buff_ptr buff_manager_t::add_buff(const uint32_t id, const uint64_t attacker, const uint32_t skill, const uint32_t layer , const uint32_t round )
{
	if ( NULL == get_owner() || NULL == get_owner()->get_combat() )
	{
		log_error("get_owner is null");
		return NULL;
	}

	if (!buff_safe_check())
	{
		log_error("buff add dead cycle!!! id[%d] attacker[%lu] skill[%d]", id, attacker, skill);
		return NULL;
	}

	BuffTable* conf = GET_CONF(BuffTable, id);
	if (NULL == conf)
	{
		log_error("buff id invalid %u", id);
		return NULL;
	}

	if (!can_add_buff(conf, skill))
	{
		log_error("add_buff can't add buff id[%d] hero[%lu][%d]", id, get_owner()->get_uid(), get_owner()->get_tid());
		return NULL;
	}

	//计算效果命中率
	fight_hero_ptr p_attacker = get_owner()->get_combat()->find_hero( attacker );
	if ( p_attacker != NULL ){
		if (p_attacker->get_camp() != get_owner()->get_camp()) {
			if (conf->buff_formula() > 0 && fight_hero_t::do_effect_hit_result(p_attacker, get_owner() ) == false ) {
				return NULL;
			}
		}
	}

	BuffBigType *p_big_type_conf = GET_CONF(BuffBigType, conf->buff_big_type());
	if (NULL == p_big_type_conf)
	{
		log_error("buff_manager_t p_big_type_conf NULL type[%d] buff_id[%d]", conf->buff_big_type(), id);
		return NULL;
	}

	BuffSmallType *p_small_type_conf = GET_CONF(BuffSmallType, conf->buff_small_type());
	if (NULL == p_small_type_conf)
	{
		log_error("buff_manager_t p_small_type_conf NULL id[%d] type[%d]", id, conf->buff_small_type());
		return NULL;
	}

	buff_ptr p_owner_buff = buff_ptr();
	buff_ptr p_same_owner_buff = buff_ptr();
	buff_ptr p_same_small_type_buff = buff_ptr();
	buff_ptr p_same_big_type_buff = buff_ptr();
	buff_ptr p_replace_buff = buff_ptr();

	for (uint32_t buff_pos = 0; buff_pos < m_buffs.size(); ++buff_pos)
	{
		p_owner_buff = m_buffs[buff_pos];
		if (NULL == p_owner_buff)
		{
			log_error("buff_manager_t p_owner_buff NULL pos[%d]", buff_pos);
			continue;
		}

		// 检查有没有相同大类型的BUFF
		if (p_owner_buff->get_big_type() == conf->buff_big_type())
		{
			p_same_big_type_buff = p_owner_buff;
			// 相同小类型
			if (p_owner_buff->get_small_type() == conf->buff_small_type())
			{
				p_same_small_type_buff = p_owner_buff;
				// 相同来源
				if (p_owner_buff->get_attacker() == attacker)
				{
					p_same_owner_buff = p_owner_buff;
				}
			}
		}
		else
		{
			// 检查有没有互斥 互斥不添加buff
			for (uint32_t i = 0; i < p_big_type_conf->buff_resist_types().size(); ++i)
			{
				if (p_owner_buff->get_big_type() == p_big_type_conf->buff_resist_type(i))
				{
					return buff_ptr();
				}
			}
			// 检查有没有替换
			for (uint32_t j = 0; j < p_big_type_conf->buff_replace_types().size(); ++j)
			{
				if (p_owner_buff->get_big_type() == p_big_type_conf->buff_replace_type(j))
				{
					p_replace_buff = p_owner_buff;
					break;
				}
			}

			if (p_replace_buff != NULL)
				break;
		}
	}

	bool b_can_add_new_buff = false;
	buff_ptr p_cover_buff = buff_ptr();

	// 先检查是否有不同大类型的情况 需要替换的buff
	if (p_replace_buff != NULL)
	{
		remove_buff(p_replace_buff, attacker);
		b_can_add_new_buff = true;
	}
	else
	{
		// 存在相同大类型
		if (p_same_big_type_buff != NULL)
		{
			// 相同小类型
			if (p_same_small_type_buff)
			{
				uint32_t add_type = 0;
				// 相同来源
				if (p_same_owner_buff)
				{
					add_type = p_small_type_conf->same_source();
				}
				// 不同来源
				else
				{
					add_type = p_small_type_conf->diff_source();
				}

				switch (add_type)
				{
				case buff_add_type_add_layer:
					{
						p_cover_buff = p_same_small_type_buff;
					}
					break;
				case buff_add_type_replace:
					{
						buff_ptr p_buff = buff_ptr();
						if (p_same_owner_buff)
							p_buff = p_same_owner_buff;
						else
							p_buff = p_same_small_type_buff;

						remove_buff(p_buff, attacker);
						b_can_add_new_buff = true;
					}
					break;
				case buff_add_type_add:
					{
						b_can_add_new_buff = true;
					}
					break;
				default:
					break;
				}
			}
			else
			{
				b_can_add_new_buff = true;
			}
		}
		// 不同大类型
		else
		{
			b_can_add_new_buff = true;
		}
	}
	
	buff_ptr buff = buff_ptr();
	buff_ptr result = buff_ptr();

	uint32_t buff_id = 0;
	if (p_cover_buff)
	{
		buff_id = p_cover_buff->get_id();
		result = cover_buff(p_cover_buff, conf, layer);
		if (NULL == result)
		{
			m_remove_set.insert(buff);
			return result;
		}
	}

	if (b_can_add_new_buff)
	{
		uint32_t new_count = 0;
		new_count = clac_gain_and_debuff(conf);
		combat_ptr p_combat = get_owner()->get_combat();
		if (NULL == p_combat)
		{
			log_error("buff_manager_t p_combat NULL");
			return result;
		}

		buff_id = p_combat->gen_buff_id();

		fight_hero_ptr attacker_ptr = p_combat->find_hero(attacker);
		if (!attacker_ptr)
		{
			log_error("buff_manager_t attacker NULL");
			return result;
		}
		uint32_t level = attacker_ptr->get_skill_level(skill);

		// 只有自己给自己加buff时需要记录武将当前回合
		uint32_t hero_round = attacker == get_owner()->get_uid() ? get_owner()->get_round() : 0;
		buff.reset(new buff_t(buff_id, conf, attacker, skill, level, layer, hero_round, new_count));
		if (round > 0) {
			buff->set_round(round);
		}
		result = add_new_buff(buff);
	}

	if (NULL == result)
	{
		m_remove_set.insert(buff);
		return result;
	}

	if (NULL != get_owner())
	{
		get_owner()->get_combat()->add_hero_event(get_owner()->get_uid(), result->get_triger_type_list(), result->get_tid() );
		if (result->get_layer_remove_type() > 0) {
			get_owner()->get_combat()->add_hero_event(get_owner()->get_uid(), result->get_layer_remove_type(), result->get_tid());
		}
		get_owner()->on_add_buff(buff_id, id, result->get_layer(), result->get_round(), result->get_trigger_type(), result->get_attacker());

		do_specail_buff_event(result->get_small_type(), p_attacker );
	}
	return result;
}

//为某些特殊buff进行优化
void  buff_manager_t::do_specail_buff_event( uint32_t small_type , fight_hero_ptr p_attacker ) {
	if (NULL == p_attacker) {
		return;
	}
	fight_hero_ptr p_owner = get_owner();
	if (NULL == p_owner) {
		return;
	}

	switch (small_type) {
	//添加中毒时
	case buff_small_type_dot: {
		p_owner->do_team_script_event(event_on_beadd_dot_buff, p_owner);
	}
		break;
	//添加睡眠buff时
	case buff_small_type_sleep: {
		p_owner->do_team_script_event(event_on_beadd_sleep, p_attacker);
	}
		break;
	}
}

uint32_t buff_manager_t::clac_gain_and_debuff(BuffTable* conf)
{
	uint32_t count = 0;
	count = conf->rounds();
	if (get_owner()->get_attr(attrtype_enum::buff_round) != 0 && conf->buff_effect_type() == buff_effect_type::buff_effect_type_gain)
	{
		return count + get_owner()->get_attr(attrtype_enum::buff_round);
	}
	if (get_owner()->get_attr(attrtype_enum::debuff_round) != 0 && conf->buff_effect_type() == buff_effect_type::buff_effect_type_debuff)
	{
		if (conf->buff_big_type() != 4) //只减益非控制类
		{
			std::vector<buff_element_data>::const_iterator it;
			for (it = conf->elements().begin(); it != conf->elements().end(); ++it)
			{
				const buff_element_data& value = *it;
				if (value.id == buff_element_index::buff_element_bomb)
					return count - get_owner()->get_attr(attrtype_enum::debuff_round);
				else
					return count + get_owner()->get_attr(attrtype_enum::debuff_round);
			}
		}
	}
	return count;
}

bool buff_manager_t::can_add_buff(BuffTable* p_buff_conf, uint32_t skill)
{
	if (NULL == p_buff_conf)
	{
		log_error("can_add_buff p_buff_conf NULL");
		return false;
	}

	if (p_buff_conf->rounds() == 0)
	{
		log_error("buff[%d] rounds = 0", p_buff_conf->id());
		return false;
	}

	// 判断是BUFF是否目标指向死亡人员
	SkillEffectTable* skill_conf = GET_CONF(SkillEffectTable, hash_util_t::hash_to_uint32_move_7(skill, 1));
	if (NULL != skill_conf)
	{
		if ( combat_t::include_death_target( skill_conf->target_type() ) == false )
		{
			if (get_owner()->is_dead())
			{
				return false;
			}
		}
	}
	else
	{
		if (get_owner()->is_dead())
		{
			return false;
		}
	}

	if (get_owner()->get_attr(attrtype_enum::immuno) > 0) {
		if (get_owner()->is_immuno_big_buff_type(p_buff_conf->buff_big_type()) == true || get_owner()->is_immuno_small_buff_type(p_buff_conf->buff_small_type()) == true) {
			get_owner()->add_combat_act(proto::common::combat_act_type_buff_immuno, p_buff_conf->get_id(), get_owner(),  get_owner());
			return false;
		}
	}

	// 判断是否禁止强化
	if (get_owner()->get_attr(attrtype_enum::forbid_strengthen) > 0)
	{
		if ((buff_effect_type)p_buff_conf->buff_effect_type() == buff_effect_type_gain)
			return false;
	}

	if (in_remove_set(p_buff_conf->id()))
	{
		log_warn("buff[%d] in_remove_set", p_buff_conf->id());
		return false;
	}
	return true;
}

// buff_ptr buff_manager_t::add_buff(buff_ptr buff)
// {
// 	if (NULL == buff)
// 	{
// 		log_error("buff is null");
// 		return buff_ptr();
// 	}
// 
// 	buff_ptr result = NULL;
// 	buff_ptr os = get_buff_by_id(buff->get_id());
// 	if (NULL == os)
// 	{
// 		result = add_new_buff(buff);
// 	}
// 	else
// 	{
// 		result = update_exist_buff(buff, os);
// 	}
// 
// 	return result;
// }

bool buff_manager_t::remove_buff_by_small_type(uint32_t small_type, uint64_t attacker_id )
{
	uint32_t notify = 0;
	buff_ptr buff = get_buff_by_small_type(small_type, attacker_id);
	if (buff)
	{
		notify |= remove_buff(buff, attacker_id);
		return true;
	}

	return false;
}

bool buff_manager_t::remove_buff_by_tid(uint32_t tid, uint64_t attacker_id)
{
	uint32_t notify = 0;
	buff_ptr buff = get_buff_by_tid(tid, attacker_id);
	if (buff)
	{
		notify |= remove_buff(buff, attacker_id);
		return true;
	}

	return false;
}

bool buff_manager_t::remove_buff(uint32_t id, uint64_t attacker_id, const bool dispel)
{
	uint32_t notify = 0;
	buff_ptr buff = get_buff_by_id(id);
	if (buff)
	{
		notify |= remove_buff(buff, attacker_id, dispel);
		return true;
	}
		
	return false;
}

uint32_t buff_manager_t::remove_buff(buff_ptr buff, uint64_t attacker_id, const bool dispel )
{
	if (NULL == buff)
	{
		log_error("remove_buff buff is null");
		return 0;
	}

	if (in_remove_set(buff))
	{
		log_warn("remove_buff buff[%d:%d] in remove_set", buff->get_id(), buff->get_tid());
		return 0;
	}

	if (m_buffs.size() == 0)
	{
		log_error("remove_buff m_buffs size = 0 but remove buff[%d:%d]", buff->get_id(), buff->get_tid());
		return 0;
	}

	uint32_t buff_layer = buff->get_layer();

	buff->set_step(buff_step_unload);

	buff_vec::iterator iter = std::find(m_buffs.begin(), m_buffs.end(), buff);
	if (iter == m_buffs.end())
	{
		log_error("remove_buff not find buff[%d:%d]", buff->get_id(), buff->get_tid());
		return 0;
	}
	m_buffs.erase(iter);
	FIGHT_LOG("remove buff id[%d:%d]", buff->get_id(), buff->get_tid());

	if (NULL != get_owner())
	{
		get_owner()->get_combat()->del_hero_event(get_owner()->get_uid(), buff->get_triger_type_list(), buff->get_tid());
		if (buff->get_layer_remove_type() > 0) {
			get_owner()->get_combat()->del_hero_event(get_owner()->get_uid(), buff->get_layer_remove_type(), buff->get_tid());
		}
		get_owner()->on_remove_buff(buff->get_id(), buff->get_tid(), 0, 0, buff->get_trigger_type(), attacker_id, dispel);
	}

	//添加指定的buff
	BuffTable *p_conf = buff->get_conf();
	if (p_conf) {
		for( auto iter : p_conf->on_remove_add_buff_map() ){
			add_buff(iter.first, buff->get_attacker(), buff->get_skill(), ceil(iter.second * buff_layer));
		}
	}
	return run_buff(buff);
}

uint32_t buff_manager_t::remove_buff_by_count(uint32_t count, buff_effect_type type)
{
	if (count == 0)
		return 0;
	buff_vec		remove_buffs;
	uint32_t		c = 0;
	for (auto buff : m_buffs)
	{
		if (NULL == buff)
			continue;

		if (buff->get_effect_type() != type)
			continue;
		remove_buffs.push_back(buff);
		++c;
		if (c >= count)
			break;
	}

	uint32_t	remove_count = 0;
	for (auto buff : remove_buffs)
	{
		++remove_count;
		remove_buff(buff);
		if (remove_count >= count)
			break;
	}
	return c > count ? count : c;
}

uint32_t buff_manager_t::remove_debuff_by_count(uint32_t count)
{
	return remove_buff_by_count(count, buff_effect_type_debuff);
}

uint32_t buff_manager_t::remove_profit_buff_by_count(uint32_t count)
{
	return remove_buff_by_count(count, buff_effect_type_gain);
	
}

void buff_manager_t::dispel_buff(const buff_dispel_info_vec& buffs, uint32_t num, fight_hero_ptr p_attacker)
{
	if (NULL == p_attacker)
	{
		log_error("p_attacker NULL");
		return;
	}
	uint64_t attacker_id = p_attacker->get_uid();
	buff_vec temp;
	buff_vec::iterator it;
	buff_ptr p_buff = buff_ptr();
	for (it = m_buffs.begin(); it != m_buffs.end(); ++it)
	{
		p_buff = *it;
		if(NULL == p_buff)
			continue;

		for (uint32_t i = 0; i < buffs.size(); ++i)
		{
			if (p_buff->get_conf() != NULL && p_buff->get_big_type() == buffs[i])
			{
				temp.push_back(p_buff);
			}
		}
	}

	// 全部驱散
	if (num >= temp.size())
	{
		p_attacker->m_buff_mgr.add_dispel_buff_count(temp.size());
		for (it = temp.begin(); it != temp.end(); ++it)
		{
			if (in_remove_set(*it))
				continue;
			remove_buff(*it, attacker_id,true);
		}
	}
	// 随机驱散
	else
	{
		uint32_t count = 0;
		uint32_t limit = 0;
		uint32_t max = 0;
		int32_t rand_pos = 0;
		while (limit++ < 200 && count < num)	// 防死循环
		{
			max = temp.size() - 1;
			rand_pos = random_util_t::randBetween(0, max);
			if (rand_pos == -1) {
				log_error("dispel_buff rand error min[%d] > max[%d]", 0, max);
				continue;
			}

			if (in_remove_set(temp[rand_pos]))
				continue;

			remove_buff(temp[rand_pos], attacker_id,true);
			temp.erase(temp.begin() + rand_pos);
			++count;
		}
		p_attacker->m_buff_mgr.add_dispel_buff_count(count);
	}
}

uint32_t buff_manager_t::dispel_buff(uint32_t type, uint32_t num, uint64_t attacker_id)
{
	buff_vec temp;
	buff_vec::iterator it;
	buff_ptr p_buff = buff_ptr();
	for (it = m_buffs.begin(); it != m_buffs.end(); ++it)
	{
		p_buff = *it;
		if (NULL == p_buff)
			continue;

		if (p_buff->get_conf() != NULL && p_buff->get_big_type() == type)
		{
			temp.push_back(p_buff);
		}
	}

	// 全部驱散
	if (num >= temp.size())
	{
		for (it = temp.begin(); it != temp.end(); ++it)
		{
			remove_buff(*it, attacker_id,true);
		}
	}
	// 随机驱散
	else
	{
		uint32_t count = 0;
		uint32_t limit = 0;
		uint32_t max = temp.size() - 1;
		int32_t rand_pos = 0;
		while (limit++ < 200 && count < num)	// 防死循环
		{
			rand_pos = random_util_t::randBetween(0, max);
			if (rand_pos == -1) {
				log_error("dispel_buff rand error min[%d] > max[%d]", 0, max);
				continue;
			}

			if (in_remove_set(temp[rand_pos]))
				continue;
			remove_buff(temp[rand_pos], attacker_id,true);
			++count;
		}
	}

	return 0;
}

buff_ptr buff_manager_t::get_buff_by_id(const uint32_t id) const
{
	for (auto buff : m_buffs)
	{
		if (NULL != buff)
		{
			if (buff->get_id() == id)
			{
				return buff;
			}
		}
	}

	return buff_ptr();
}

buff_ptr buff_manager_t::get_buff_by_small_type(const uint32_t small_type, uint64_t attacker_id) const
{
	for (auto buff : m_buffs)
	{
		if (NULL == buff)
			continue;

		if (buff->get_small_type() != small_type)
			continue;

		if (attacker_id == 0)
			return buff;

		if (buff->get_attacker() == attacker_id)
			return buff;
	}

	return buff_ptr();
}

buff_ptr buff_manager_t::get_buff_by_tid(const uint32_t tid, uint64_t attacker_id) const
{
	for (auto buff : m_buffs)
	{
		if (NULL == buff)
			continue;
		
		if (buff->get_tid() != tid)
			continue;
		
		if (attacker_id == 0)
			return buff;
		if (buff->get_attacker() == attacker_id)
			return buff;
	}

	return buff_ptr();
}

buff_ptr buff_manager_t::get_random_buff_by_bigtype(const uint32_t type)
{
	std::vector<buff_ptr> find;
	for (auto buff : m_buffs)
	{
		if (NULL != buff)
		{
			if (buff->get_big_type() == type && buff->get_layer() > 0)
			{
				find.push_back(buff);
			}
		}
	}

	if (find.empty())
	{
		return buff_ptr();
	}

	int32_t index = random_util_t::randMin(0, find.size());
	if (index == -1)
	{
		log_error("get_random_buff_by_type index = -1 min[%d] max[%d]", 0, (int32_t)find.size());
		return NULL;
	}
	return find[index];
}

uint32_t buff_manager_t::get_all_count_by_effect_type(const int32_t type)
{
	uint32_t count = 0;
	for (auto buff : m_buffs)
	{
		if (NULL != buff && buff->get_layer() > 0)
		{
			//取增益
			if (type == 0) {
				if (buff->get_effect_type() == buff_effect_type_gain)
					count += buff->get_layer();
			}

			//取减益
			if (type == -1) {
				if (buff->get_effect_type() == buff_effect_type_debuff)
					count += buff->get_layer();
			}
		}
	}
	return count;
}


buff_ptr buff_manager_t::get_random_buff_by_smalltype(const int32_t type) 
{
	std::vector<buff_ptr> find;
	for (auto buff : m_buffs)
	{
		if (NULL != buff && buff->get_layer() > 0 )
		{
			//取增益
			if (type == 0 ) {
				if( buff->get_effect_type() == buff_effect_type_gain )
					find.push_back(buff);
				continue;
			}

			//取减益
			if (type == -1) {
				if( buff->get_effect_type() == buff_effect_type_debuff )
					find.push_back(buff);
				continue;
			}

			if ( buff->get_small_type() == (uint32_t)type ) {
				find.push_back(buff);
			}
		}
	}

	if (find.empty()) {
		return buff_ptr();
	}

	int32_t index = random_util_t::randMin(0, find.size());
	if (index == -1) {
		log_error("get_random_buff_by_type index = -1 min[%d] max[%d]", 0, (int32_t)find.size());
		return NULL;
	}
	return find[index];
}



buff_ptr buff_manager_t::add_new_buff(buff_ptr buff)
{
	if (NULL == buff)
	{
		log_error("add_new_buff buff is null");
		return buff_ptr();
	}

	if (get_owner() != NULL)
		get_owner()->add_combat_buff_act(proto::common::combat_act_type_buff, buff->get_id(), buff->get_tid(), buff->get_layer(), buff->get_round(), buff->get_attacker());

	m_buffs.push_back(buff);

	uint32_t notify = 0;
	notify |= run_buff(buff);

	if (get_owner() && get_owner()->get_combat())
	{
		get_owner()->get_combat()->add_round_buff_add_count();
	}

	return buff;
}

buff_ptr buff_manager_t::cover_buff(buff_ptr os, BuffTable* ns, uint32_t layer)
{
	if (NULL == ns)
	{
		log_error("cover_buff ns is null");
		return buff_ptr();
	}

	if (NULL == os)
	{
		log_error("cover_buff os is null");
		return buff_ptr();
	}

	if (ns->overlay_type() == overlay_type_none)
	{
		log_error("overlay_type is 0");
		return buff_ptr();
	}
	
	if(get_owner() == NULL)
	{
		log_error("get owner null");
		return buff_ptr();
	}

	uint32_t notify = 0;
	int numchange = 0;

	if (get_owner()->get_attr(attrtype_enum::buff_round) != 0 && os->get_effect_type() == buff_effect_type::buff_effect_type_gain)
	{
		layer += get_owner()->get_attr(attrtype_enum::buff_round);
	}

	if (get_owner()->get_attr(attrtype_enum::debuff_round) != 0 && os->get_effect_type() == buff_effect_type::buff_effect_type_debuff)
	{
		layer -= get_owner()->get_attr(attrtype_enum::debuff_round);
	}

	if (ns->overlay_type() == overlay_type_count)
	{
		os->set_round(os->get_round() + ns->rounds());
	}
	else if (ns->overlay_type() == overlay_type_layer)
	{
		uint32_t max_layer = ns->max_layer();
		if (max_layer > os->get_layer() + layer)
		{
			numchange = layer;
			os->add_layer(numchange);
		}
		else
		{
			numchange = max_layer - os->get_layer();
			os->set_layer(max_layer);
		}

		os->set_step(buff_step_layer_change);
	}

	get_owner()->add_combat_buff_act(proto::common::combat_act_type_buff, os->get_id(), os->get_tid(), os->get_layer(), os->get_round(), os->get_attacker());

	notify |= run_buff(os, numchange);
	FIGHT_LOG("cover_buff buff_id[%d:%d] step[%d]", os->get_id(), os->get_tid(), os->get_step());
	return os;
}

uint32_t buff_manager_t::run_buff(buff_ptr buff, const int numchange, const fight_hero_ptr p_event_owner, uint32_t event_num )
{
	if (NULL == buff)
	{
		log_error("run_buff buff is null");
		return 0;
	}

	BuffTable* conf = buff->get_conf();
	if (NULL == conf)
	{
		log_error("run_buff conf is null");
		return 0;
	}

	//用于限制buff执行次数
	if (buff->get_step() == buff_step_trigger) {
		if ( conf->phase_run_count() > 0 && buff->get_phase_run() >= conf->phase_run_count() ){
			return 0;
		}
	
		if (buff->get_phase_run() >= 30) {
			log_error("run_buff[%u] max >= 30", buff->get_tid());
			return 0;
		}

		buff->add_phase_run();
	}

	FIGHT_LOG("run_buff buff id[%d:%d] step[%d]", buff->get_id(), buff->get_tid(), buff->get_step());
	uint32_t notify = 0;
	std::vector<buff_element_data>::const_iterator it;
	for (it = conf->elements().begin(); it != conf->elements().end(); ++it)
	{
		const buff_element_data& value = *it;
		uint32_t element_index = 0;
		if (m_funclist.find(value.id) != m_funclist.end())
		{
			element_index = value.id;
		}
		else
		{
			log_error("buff_manager_t::run_buff [%d:%d] can not find element %u", buff->get_id(), buff->get_tid(), value.id);
			return 0;
		}

		if (element_index > 0)
		{
			notify |= m_funclist[element_index](get_owner(), buff, value, numchange, p_event_owner, event_num);

			FIGHT_LOG("run_buff deal_func buff id[%d:%d] func[%d]", buff->get_id(), buff->get_tid(), element_index);
		}
	}

	if ( notify == buff_notify_valid && conf->passive_show_word() > 0 ) {
		combat_ptr p_combat = get_owner()->get_combat();
		if (NULL == p_combat) {
			return notify;
		}

		fight_hero_ptr p_attack = p_combat->find_hero(buff->get_attacker());
		if (NULL == p_attack) {
			return notify;
		}

		p_attack->send_trigger_passive_to_user(buff->get_skill(), buff->get_id(), buff->get_tid());
	}

	return notify;
}

uint32_t buff_manager_t::get_count_by_small_type(const int type)
{
	uint32_t count = 0;
	buff_ptr p_buff = buff_ptr();
	buff_vec::iterator it;
	for (it = m_buffs.begin(); it != m_buffs.end(); ++it)
	{
		p_buff = *it;
		if (p_buff == NULL)
			continue;
		if ((int)p_buff->get_small_type() == type)
			++count;
	}
	return count;
}

uint32_t buff_manager_t::get_count_by_big_type(const int type)
{
	uint32_t count = 0;
	buff_ptr p_buff = buff_ptr();
	buff_vec::iterator it;
	for (it = m_buffs.begin(); it != m_buffs.end(); ++it)
	{
		p_buff = *it;
		if (p_buff == NULL)
			continue;
		if ((int)p_buff->get_big_type() == type)
			++count;
	}
	return count;
}

uint32_t buff_manager_t::get_buff_count(buff_effect_type type)
{
	uint32_t count = 0;
	buff_ptr p_buff = buff_ptr();
	buff_vec::iterator it;
	for (it = m_buffs.begin(); it != m_buffs.end(); ++it)
	{
		p_buff = *it;
		if (p_buff == NULL)
			continue;
		if (p_buff->get_effect_type() == type)
			++count;
	}
	return count;
}

// 获取减益BUFF数量
uint32_t buff_manager_t::get_debuff_count()
{
	return get_buff_count(buff_effect_type_debuff);
}
// 获取增益BUFF数量
uint32_t buff_manager_t::get_profit_buff_count()
{
	return get_buff_count(buff_effect_type_gain);	
}

void buff_manager_t::add_passive_buff_cd(buff_ptr buff, uint32_t old_turn_level )
{
	SkillEffectTable* skill_conf = GET_CONF(SkillEffectTable, hash_util_t::hash_to_uint32_move_7(buff->get_skill(), 1));
	if (NULL == skill_conf)
		return;

	//判断技能是否被动技能，默认装备技能也是被动的技能
	if (skill_conf->type() != proto::common::skill_type_passive && skill_conf->type() != proto::common::skill_type_equip_skill)
		return;

	get_owner()->add_skill_cd(buff->get_skill());
}

bool buff_manager_t::check_passive_buff_cd(buff_ptr buff)
{
	SkillEffectTable* skill_conf = GET_CONF(SkillEffectTable, hash_util_t::hash_to_uint32_move_7(buff->get_skill(), 1));
	if (NULL == skill_conf)
		return false;

	//判断技能是否被动技能，默认装备技能也是被动的技能
	if (skill_conf->type() != proto::common::skill_type_passive && skill_conf->type() != proto::common::skill_type_equip_skill)
		return false;

	//此处有坑
	if( get_owner()->get_skill_cd( buff->get_skill() ) == 0 )
		return true;

	return false;
}

void buff_manager_t::on_round_start()
{
	clear_trigger_count();
	clear_dispel_buff_count();
}

void buff_manager_t::clear_trigger_count()
{
	buff_vec::iterator it;
	for (it = m_buffs.begin(); it != m_buffs.end(); ++it)
	{
		buff_ptr buff = *it;
		if (NULL == buff)
		{
			continue;
		}
		buff->set_trigger_count(0);
	}
	for (it = m_special_buffs.begin(); it != m_special_buffs.end(); ++it)
	{
		buff_ptr buff = *it;
		if (NULL == buff)
		{
			continue;
		}
		buff->set_trigger_count(0);
	}
}

bool buff_manager_t::update_buff(FIGHT_EVENT check_type, const fight_hero_ptr p_event_owner)
{
	if (NULL == get_owner())
	{
		return false;
	}

    FIGHT_LOG("hero[%lu], update_buff_event[%u]", get_owner()->get_uid(), check_type);
	
	uint32_t notify = 0;
	buff_vec copy(m_buffs);
	buff_vec::iterator it;
	for (it = copy.begin(); it != copy.end(); ++it)
	{
		buff_ptr buff = *it;
		if (NULL == buff)
		{
			continue;
		}
		
		buff->set_step(buff_step_trigger);

		if (check_trigger(check_type, buff, notify, true , p_event_owner))
		{
			buff->add_trigger_count();
		}
		check_remove_layer(check_type, buff, notify);
		check_remove(check_type, buff, notify);
	}

	// 处于忘却状态无法激活被动效果
	if(get_owner()->get_attr( attrtype_enum::forget ) > 0 )
		return false;

	bool trigger = false;
	buff_vec copy_special(m_special_buffs);
	for (it = copy_special.begin(); it != copy_special.end(); ++it)
	{
		buff_ptr buff = *it;
		if (NULL == buff)
			continue;

		// 检测BUFF是否处于CD状态
		if( !check_passive_buff_cd(buff) )
			continue;

		uint32_t old_turn_level = get_owner()->get_turn_level();

		buff->set_step(buff_step_trigger);
		if (check_trigger(check_type, buff, notify, true, p_event_owner) == true)
		{
			// 添加被动BUFFCD
			add_passive_buff_cd(buff, old_turn_level);
			buff->add_trigger_count();
		
			trigger = true;
		}
	}

	return trigger;
}

bool buff_manager_t::check_trigger(FIGHT_EVENT check_type, buff_ptr buff, uint32_t& notify, bool needprobability, const fight_hero_ptr p_event_owner)
{
	if (NULL == buff)
	{
		log_error("buff is null");
		return false;
	}
	
	if (get_owner() == NULL) 
	{
		log_error("owner is null");
		return false;
	}

	//普通buff回合结束后触发
	if (check_type == event_on_round_end && buff->get_round_remove_type() == ROUND_REMOVE_TYPE_NORMAL )
	{
		// 如果buff有记录武将添加回合 则当前武将回合==buff武将回合时不减回合数
		if ( buff->get_hero_round() == 0 )
		{
			buff->sub_round();
		}
		else
		{
			if ( buff->get_hero_round() != get_owner()->get_round() )
				buff->sub_round();
		}

		get_owner()->add_combat_buff_act(proto::common::combat_act_type_buff_update, buff->get_id(), buff->get_tid(), buff->get_layer(), buff->get_round(), buff->get_attacker());
	}

	//炸弹类的buff回合开始前开始减少次数
	if ( check_type == event_on_round_start && buff->get_round_remove_type() == ROUND_REMOVE_TYPE_BOMB )
	{
		buff->sub_round();

		if (NULL != get_owner())
		{
			get_owner()->add_combat_buff_act(proto::common::combat_act_type_buff_update, buff->get_id(), buff->get_tid(), buff->get_layer(), buff->get_round(), buff->get_attacker());
		}
	}


	if ( check_type != event_on_none && !buff->is_trigger_type( check_type) )
	{
		return false;
	}

	if (buff->get_max_trigger_count() != 0)
	{
		if (buff->get_trigger_count() >= buff->get_max_trigger_count())
		{
			return false;
		}
	}

	uint32_t rand_value = random_util_t::randBetween(1, 10000);
	BuffTable *p_conf = buff->get_conf();
	if (needprobability == true)
	{
		if (p_conf != NULL && p_conf->probability() < rand_value)
		{
			return false;
		}
	}

// 	if (check_type == buff_check_type_before_attack || check_type == buff_check_type_after_attack ||
// 		check_type == buff_check_type_before_be_attack || check_type == buff_check_type_after_be_attack)
// 	{
// 		buff->sub_layer();
// 	}

	notify = run_buff(buff, 0, p_event_owner , check_type);
	FIGHT_LOG("check_trigger buff_id[%d:%d] step[%d] type[%d]", buff->get_id(), buff->get_tid(), buff->get_step(), check_type);

	//中毒类 技能触发成功减少次数
	if (notify == buff_notify_valid && buff->get_round_remove_type() == ROUND_REMOVE_TYPE_DOT ) {
		buff->sub_round();

		if (NULL != get_owner()) {
			get_owner()->add_combat_buff_act(proto::common::combat_act_type_buff_update, buff->get_id(), buff->get_tid(), buff->get_layer(), buff->get_round(), buff->get_attacker());
		}
	}

	return notify != (uint32_t)buff_notify_none;
}

bool buff_manager_t::check_remove_layer(FIGHT_EVENT check_type, buff_ptr buff, uint32_t& notify)
{
	if (NULL == buff)
	{
		log_error("buff is null");
		return false;
	}

	if (buff->get_layer_remove_type() != check_type)
	{
		return false;
	}

	sub_buff_layer(buff, 1);
	return true;
}

bool buff_manager_t::check_remove(FIGHT_EVENT check_type, buff_ptr buff, uint32_t& notify)
{
	if (NULL == buff)
	{
		log_error(" buff is null");
		return false;
	}

	if (buff->get_remove_type() == buff_remove_type_cur_round && check_type == event_on_round_end)
	{
		notify |= remove_buff(buff, buff->get_attacker());
		return true;
	}

	if (buff->get_remove_type() == buff_remove_type_after_run) {
		notify |= remove_buff(buff, buff->get_attacker());
		return true;
	}

	if (buff->get_round() == 0)
	{
		notify |= remove_buff(buff, buff->get_attacker());
		return true;
	}

// 	if (buff->get_count() == 0)
// 	{
// 		notify |= remove_buff(buff, get_owner()->get_attacker());
// 		return true;
// 	}

	if (buff->get_layer() == 0)
	{
		notify |= remove_buff(buff, buff->get_attacker());
		return true;
	}

	return false;
}

void buff_manager_t::remove_all_buff()
{
	buff_vec copy(m_buffs);
	buff_vec::iterator it;
	for (it = copy.begin(); it != copy.end(); ++it)
	{
		remove_buff(*it);
	}
}

void buff_manager_t::remove_buff_when_dead()
{
	buff_vec copy(m_buffs);
	buff_vec::iterator it;
	for (it = copy.begin(); it != copy.end(); ++it)
	{
		buff_ptr buff = *it;
		if (NULL == buff)
		{
			continue;
		}

		if (!buff->need_remove_when_dead())
		{
			continue;
		}

		remove_buff(buff);
	}
}

void buff_manager_t::remove_buff_when_revive()
{
	buff_vec copy(m_buffs);
	buff_vec::iterator it;
	for (it = copy.begin(); it != copy.end(); ++it)
	{
		buff_ptr buff = *it;
		if (NULL == buff)
		{
			continue;
		}

		if (buff->need_remove_when_revive())
		{
			remove_buff(buff);
		}
	}
}

bool buff_manager_t::in_remove_set(buff_ptr buff) const
{
	if (NULL == buff)
	{
		log_error("in_remove_set buff is null");
		return false;
	}

	for (auto remove_buff : m_remove_set)
	{
		if (NULL == remove_buff)
		{
			continue;
		}

		if (remove_buff->get_id() == buff->get_id())
		{
			return true;
		}
	}

	return false;
}

bool buff_manager_t::in_remove_set(uint32_t buff_id) const
{
	for (auto remove_buff : m_remove_set)
	{
		if (NULL == remove_buff)
		{
			continue;
		}

		if (remove_buff->get_id() == buff_id)
		{
			return true;
		}
	}

	return false;
}

fight_hero_ptr buff_manager_t::get_owner()
{
	return m_owner.lock();
}

void buff_manager_t::peek_all_buff_data(proto::common::fight_buff_data *buff_data)
{
	if (NULL == buff_data)
	{
		log_error("buff_manager_t peek_all_buff_data buff_data NULL");
		return;
	}
	buff_ptr p_buff = buff_ptr();
	proto::common::fight_buff_single_data *single_data = NULL;
	for (uint32_t i = 0; i < m_buffs.size(); ++i)
	{
		p_buff = m_buffs[i];
		if (NULL == p_buff)
		{
			log_error("buff_manager_t peek_all_buff_data p_buff NULL pos[%d]", i);
			continue;
		}
		single_data = buff_data->add_buff_list();
		p_buff->peek_data(single_data);
	}
}

void buff_manager_t::add_trigger_count(const uint32_t id, uint32_t count /*= 1*/)
{
	buff_ptr buff = get_buff_by_id(id);
	if (NULL == buff)
	{
		log_error("buff is null %u", id);
		return;
	}

	buff->add_count(count);
}

void buff_manager_t::sub_trigger_count(const uint32_t id, uint32_t count /*= 1*/)
{
	buff_ptr buff = get_buff_by_id(id);
	if (NULL == buff)
	{
		log_error("buff is null %u", id);
		return;
	}

	buff->sub_count(count);
}

bool buff_manager_t::add_buff_layer(const uint32_t tid, uint64_t attacker, uint32_t count)
{
	buff_ptr buff = get_buff_by_tid(tid, attacker);
	if (NULL == buff)
		return false;
	
	return add_buff_layer(buff, count);
}

void buff_manager_t::sub_buff_layer(const uint32_t tid, uint64_t attacker, uint32_t count)
{
	buff_ptr buff = get_buff_by_tid(tid, attacker);
	if (NULL != buff)
	{
		sub_buff_layer(buff, count);
	}
}

bool buff_manager_t::add_buff_layer(buff_ptr buff, uint32_t count )
{
	if (NULL == buff)
	{
		log_error("buff is null");
		return false;
	}
	if (count == 0)
		return false;

	uint32_t real_count = count;
	if (buff->get_layer() + count >= buff->get_max_layer())
	{
		real_count = buff->get_max_layer() - buff->get_layer();
	}
	
	/* add by hy ,看不懂的操作
	if (real_count > 0)
		return false;
	*/

	uint32_t notify = 0;
	int numchange = real_count;
	buff->add_layer(real_count);
	buff->set_step(buff_step_layer_change);

	if (NULL != get_owner())
	{
		get_owner()->add_combat_buff_act(proto::common::combat_act_type_buff_update, buff->get_id(), buff->get_tid(), buff->get_layer(), buff->get_round(), buff->get_attacker());
	}

	notify |= run_buff(buff, numchange);
	return real_count > 0;
}

bool buff_manager_t::sub_buff_layer(buff_ptr buff, uint32_t count /*= 1*/)
{
	if (NULL == buff)
	{
		log_error("buff is null");
		return false;
	}

	uint32_t real_count = count;
	if (buff->get_layer() < count)
	{
		real_count = buff->get_layer();
	}

	uint32_t notify = 0;
	int numchange = -real_count;
	buff->sub_layer(real_count);
	buff->set_step(buff_step_layer_change);

	if (NULL != get_owner())
	{
		get_owner()->add_combat_buff_act(proto::common::combat_act_type_buff_update, buff->get_id(), buff->get_tid(), buff->get_layer(), buff->get_round(), buff->get_attacker());
	}

	notify |= run_buff(buff, numchange);
	return buff->get_layer() <= 0 ;
}

void buff_manager_t::sub_buff_layer_or_remove(const uint32_t tid, uint64_t attacker, uint32_t count)
{
	buff_ptr buff = get_buff_by_tid(tid, attacker);
	if (NULL == buff)
		return;

	if (sub_buff_layer(buff, count) == true)
	{
		remove_buff(buff, attacker);
	}

}

uint32_t buff_manager_t::get_buff_layer(const uint32_t id)
{
	buff_ptr buff = get_buff_by_id(id);
	if (NULL == buff)
	{
		return 0;
	}

	return buff->get_layer();
}

uint32_t buff_manager_t::get_buff_layer_by_tid(uint32_t tid, uint64_t attacker)
{
	buff_ptr buff = get_buff_by_tid(tid, attacker);
	if (NULL == buff)
	{
		return 0;
	}

	return buff->get_layer();
}

uint32_t buff_manager_t::get_buff_count_by_tid(uint32_t tid, uint64_t attacker)
{
	uint32_t count = 0;
	for (auto buff : m_buffs)
	{
		if (NULL == buff)
			continue;

		if (buff->get_tid() != tid)
			continue;

		if (attacker == 0)
			count = count + buff->get_round();
		if (buff->get_attacker() == attacker)
			count = count + buff->get_round();
	}

	return count;
}

//根据小类型获取buff层数
uint32_t buff_manager_t::get_buff_layer_by_smalltype(uint32_t type, uint64_t attacker) 
{
	uint32_t count = 0;
	for (auto buff : m_buffs)
	{
		if (NULL == buff)
			continue;

		if (buff->get_small_type() != type)
			continue;

		if (attacker == 0 || buff->get_attacker() == attacker)
		{
			count += buff->get_layer();
		}
	}

	return count;
}

//根据buff增减益类型计算层数
uint32_t buff_manager_t::get_buff_layer_by_effect_type(buff_effect_type type)
{
	uint32_t count = 0;
	buff_ptr p_buff = buff_ptr();
	buff_vec::iterator it;
	for (it = m_buffs.begin(); it != m_buffs.end(); ++it)
	{
		p_buff = *it;
		if (p_buff == NULL)
		{
			continue;
		}
		if (p_buff->get_effect_type() == type)
		{
			count += p_buff->get_layer();
		}
	}
	return count;
}

uint32_t buff_manager_t::get_buff_round(const uint32_t id)
{
	buff_ptr buff = get_buff_by_id(id);
	if (NULL == buff)
	{
		return 0;
	}

	return buff->get_round();
}

uint32_t buff_manager_t::get_buff_round_by_tid(uint32_t tid, uint64_t attacker)
{
	buff_ptr buff = get_buff_by_tid(tid, attacker);
	if (NULL == buff)
	{
		return 0;
	}

	return buff->get_round();
}

uint64_t buff_manager_t::get_buff_attacker(const uint32_t id)
{
	buff_ptr buff = get_buff_by_id(id);
	if (NULL == buff)
	{
		return 0;
	}

	return buff->get_attacker();
}

buff_ptr buff_manager_t::add_special_buff(const uint32_t id, const uint64_t attacker, const uint32_t skill, const uint32_t layer)
{
	buff_ptr buff = buff_ptr();

	combat_ptr p_combat = get_owner()->get_combat();
	if (NULL == p_combat)
	{
		log_error("buff_manager_t p_combat NULL");
		return buff;
	}

	BuffTable* conf = GET_CONF(BuffTable, id);
	if (NULL == conf)
	{
		log_error("buff id invalid %u", id);
		return buff_ptr();
	}

	uint32_t level = get_owner()->get_skill_level(skill);

	uint32_t buff_id = p_combat->gen_buff_id();
	buff.reset(new buff_t(buff_id, conf, attacker, skill, level, layer));

	add_new_special_buff(buff);

	return buff;
}

bool buff_manager_t::remove_special_buff(uint32_t id, uint64_t attacker_id)
{
	buff_ptr p_buff = buff_ptr();
	for (uint32_t i = 0; i < m_special_buffs.size(); ++i)
	{
		p_buff = m_special_buffs[i];
		if (NULL == p_buff)
		{
			log_error("remove_special_buff p_buff NULL pos[%d]", i);
			continue;
		}
		if (p_buff->get_id() == id)
		{
			return remove_special_buff(p_buff, attacker_id);
		}
	}
	return false;
}

bool buff_manager_t::remove_special_buff_by_tid(uint32_t tid, uint64_t attacker_id)
{
	buff_ptr p_buff = buff_ptr();
	for (uint32_t i = 0; i < m_special_buffs.size(); ++i)
	{
		p_buff = m_special_buffs[i];
		if (NULL == p_buff)
		{
			log_error("remove_special_buff p_buff NULL pos[%d]", i);
			continue;
		}
		if (p_buff->get_tid() == tid)
		{
			return remove_special_buff(p_buff, attacker_id);
		}
	}
	return false;
}

void buff_manager_t::remove_all_special_buff()
{
	buff_vec copy(m_special_buffs);
	buff_vec::iterator it;
	for (it = copy.begin(); it != copy.end(); ++it)
	{
		remove_special_buff(*it);
	}
}

void buff_manager_t::add_new_special_buff(buff_ptr buff)
{
	if (NULL == buff)
	{
		log_error("add_un_initiative_buff buff is null");
		return;
	}

	get_owner()->get_combat()->add_hero_event(get_owner()->get_uid(), buff->get_triger_type_list(), buff->get_tid() );

	if ( buff->get_layer_remove_type() > 0 ) {
		get_owner()->get_combat()->add_hero_event(get_owner()->get_uid(), buff->get_layer_remove_type(), buff->get_tid() );
	}
	
	m_special_buffs.push_back(buff);
	run_buff(buff);
}

bool buff_manager_t::remove_special_buff(buff_ptr buff, uint32_t attacker_id /*= 0*/)
{
	if (NULL == buff)
	{
		log_error("remove_special_buff buff is null");
		return false;
	}
	if (m_special_buffs.size() == 0)
	{
		log_error("remove_special_buff m_special_buffs size = 0 but remove buff[%d:%d]", buff->get_id(), buff->get_tid());
		return false;
	}

	buff->set_step(buff_step_unload);

	buff_vec::iterator iter = std::find(m_special_buffs.begin(), m_special_buffs.end(), buff);
	if (iter == m_special_buffs.end())
	{
		log_error("remove_special_buff not find buff[%d:%d]", buff->get_id(), buff->get_tid());
		return 0;
	}
	m_special_buffs.erase(iter);
	FIGHT_LOG("remove_special_buff id[%d:%d]", buff->get_id(), buff->get_tid());
	
	get_owner()->get_combat()->del_hero_event(get_owner()->get_uid(), buff->get_triger_type_list(), buff->get_tid() );
	
	if (buff->get_layer_remove_type() > 0) {
		get_owner()->get_combat()->del_hero_event(get_owner()->get_uid(), buff->get_layer_remove_type(), buff->get_tid());
	}

	return run_buff(buff);
}

//减少玩家身上被动buffcd回合
void buff_manager_t::sub_buff_cd() {
	for (auto p_buff : m_buffs) {
		if (NULL == p_buff)
			continue;
		p_buff->sub_cd();
	}

	for (auto p_buff : m_special_buffs) {
		if (NULL == p_buff)
			continue;
		p_buff->sub_cd();
	}
}

//清理一段攻击后的信息
void buff_manager_t::clear_phase_run_info() {
	for (auto p_buff : m_buffs) {
		if (NULL == p_buff)
			continue;
		p_buff->clear_phase_run();
	}

	for (auto p_buff : m_special_buffs) {
		if (NULL == p_buff)
			continue;
		p_buff->clear_phase_run();
	}
}


buff_ptr buff_manager_t::make_new_buff(combat_ptr p_combat, uint32_t id, uint64_t uid, uint32_t skill_id, uint32_t level, uint32_t layer) {
	if (NULL == p_combat) {
		return buff_ptr();
	}

	BuffTable* conf = GET_CONF(BuffTable, id);
	if (NULL == conf) {
		log_error("buff id invalid %u", id);
		return buff_ptr();
	}

	uint32_t buff_id = p_combat->gen_buff_id();

	buff_ptr buff = buff_ptr();
	buff.reset(new buff_t(buff_id, conf, uid, skill_id, level, layer));

	return buff;
}


uint32_t buff_manager_t::run_public_buff(buff_ptr buff, const fight_hero_ptr p_event_owner, uint32_t event_num) {
	log_error("run public_buff buff_id:[%u], event:%u", buff->get_tid(),  event_num );
	uint32_t notify = 0;
	check_trigger((FIGHT_EVENT)event_num, buff, notify, true, p_event_owner);
	return	0;
}
