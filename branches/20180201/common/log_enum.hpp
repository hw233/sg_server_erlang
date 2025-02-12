#ifndef __COMMON_LOG_ENUM_HPP__
#define __COMMON_LOG_ENUM_HPP__

#include "macros.hpp"

NS_COMMON_BEGIN

namespace log_enum
{
	enum logs_source_type_t : uint32_t
	{
		    source_type_none = 0,

		    //gm
		    source_type_gm_begin = 1,
		    source_type_gm_create_item = source_type_gm_begin,
		    source_type_gm_cost_item,
		    source_type_gm_add_pk,
		    source_type_gm_add_money,
		    source_type_gm_dec_money,
		    source_type_gm_add_hero,
		    source_type_gm_add_exp,
		    source_type_gm_set_level,
		    source_type_gm_add_prestige,
		    source_type_gm_finish_task,
		    source_type_gm_drop,
		    source_type_gm_end,

		    //role
		    source_type_role_begin = 100,
            source_type_role_create = source_type_role_begin,
		    source_type_role_login,
			source_type_role_logout_none,
		    source_type_role_logout_normal,
            source_type_role_logout_pingpong_timeout,
            source_type_role_logout_kick_by_others,
		    source_type_role_logout_ban_account,
		    source_type_role_logout_reconnect_timeout,
            source_type_role_logout_gate_server_close,
            source_type_role_logout_game_server_close,
            source_type_role_logout_msg_bomb_kick,
			source_type_role_logout_center_close,
			source_type_role_logout_cross_server,
			source_type_role_logout_error,
		    source_type_role_offline_reward,
		    source_type_role_use_item_task,
		    source_type_role_luck_gift_return_surplus,
		    source_type_role_luck_gift_send,
		    source_type_role_luck_gift_grab,
		    source_type_role_exchange,
		    source_type_role_every_day_reset,
		    source_type_role_every_week_reset,
		    source_type_role_sell_bag_item,
		    source_type_role_use_item_gift_package,
		    source_type_role_collect,
		    source_type_role_use_treasure_item,
		    source_type_role_use_note_closed,
		    source_type_role_use_note_opened,
		    source_type_role_return_treasure_item,
		    source_type_role_troop_treasure,
		    source_type_role_use_item_mount,
		    source_type_role_auto_reduce_pk_value,
		    source_type_role_challenge,
		    source_type_role_use_item_reduce_pk_value,
		    source_type_role_use_gold_to_revive,
		    source_type_role_use_item_story,
			source_type_role_use_item_plugin,
			source_type_role_use_item_source_package,
			source_type_role_use_item_add_multi_time,
			source_type_role_merage_item,
			source_type_role_decompose_item,
			source_type_role_vigour_pill,
			source_type_role_use_item_add_rolebuff,
			source_type_role_end,

		    //interior
		    source_type_interior_begin = 300,
		    source_type_interior_task_reward = source_type_interior_begin,
		    source_type_interior_task_active_item,
		    source_type_interior_task_finish_pay,
		    source_type_interior_task_finish,
		    source_type_interior_upgrade_build_cost_item,
		    source_type_interior_building_produce,
		    source_type_interior_refresh_task,
		    source_type_interior_upgrade_task_star_level,
		    source_type_interior_complete_task,
		    source_type_interior_battle,
		    source_type_interior_end,

		    //world_cup
		    source_type_world_cup_begin = 400,
		    source_type_world_cup_bet = source_type_world_cup_begin,
		    source_type_world_cup_reward_bet,
		    source_type_world_cup_end,

			// vigour
			source_type_vigour_begin = 450,
			source_type_vigour_buy_add = source_type_vigour_begin,
			source_type_vigour_time_add,
			source_type_vigour_pill_add,
			source_type_vigour_gm_add,
			source_type_vigour_gm_reduce,
			source_type_vigour_dungeon_reduce,
			source_type_vigour_buy_times_clear,
			source_type_vigour_buy_times_add,
			source_type_vigour_end,

		    //hero
		    source_type_hero_begin = 500,
		    source_type_hero_levelup = source_type_hero_begin,
		    source_type_hero_starup,
		    source_type_hero_advanced,
		    source_type_hero_talentadd,
		    source_type_hero_levelup_fate,
		    source_type_hero_levelup_talent,
		    source_type_hero_active_talent,
		    source_type_hero_reset_talent,
		    source_type_hero_arm_strengthen,
		    source_type_hero_recruit,
		    source_type_hero_levelup_skill,
		    source_type_hero_luckydraw,
			source_type_hero_wakeup,
			source_type_hero_story_add,
			source_type_hero_story_remove,
			source_type_hero_unlock,
		    source_type_hero_buy_luckydraw,
		    source_type_hero_open_luckydraw,
			source_type_hero_unlock_skill,
			source_type_hero_replace_skill,
		    source_type_hero_end,

		    //monser
		    source_type_monster_begin = 550,
		    source_type_dark_monster_die = source_type_monster_begin,
		    source_type_light_monster_win,
			source_type_monster_fight_pve,
			source_type_field_boss_pve,
			source_type_against_pve,
		    source_type_monster_end,

		    // mail
		    source_type_mail_begin = 600,
		    source_type_mail_addenda = source_type_mail_begin,
		    source_type_mail_new_mail,
		    source_type_mail_open,
		    source_type_mail_user_get_addenda,
		    source_type_mail_delete,
		    source_type_mail_end,
        
		    //task
		    source_type_task_begin = 700,
		    source_type_task_accept = source_type_task_begin,
		    source_type_task_state_change,
		    source_type_task_finish,
		    source_type_task_help_finish,
		    source_type_bounty_star_box,
		    source_type_bounty_finish,
		    source_type_bounty_accept,
		    source_type_bounty_refresh,
			source_type_task_refresh_shilian,
			source_type_task_fight_win,
			source_type_task_zhuogui_leader,
			source_type_task_quick_finish,
		    source_type_task_end,

		    //family
		    source_type_family_begin = 800,
		    source_type_family_online_prosperity_add = source_type_family_begin,
		    source_type_family_chat_prosperity_add,
		    source_type_family_task_prosperity_add,
		    source_type_family_do_activity,
		    source_type_family_create_cost,
		    source_type_family_impeach_cost,
		    source_type_family_impeach_return,
		    source_type_family_impeach_fail_return,
            source_type_family_create_fail_return,
			source_type_family_create,
			source_type_family_join,
			source_type_family_assign,
			source_type_family_approve_join,
			source_type_family_upgrade_build,
			source_type_family_clear_upgrade_build_cd,
			source_type_family_toggle_auto_newbie,
			source_type_family_modify_declaration,
			source_type_family_modify_notification,
            source_type_family_abandon_impeach,
            source_type_family_approve_impeach,
			source_type_family_kick,
            source_type_family_impeach,      
			source_type_family_delete,
			source_type_family_auto_join,
			source_type_family_join_apply_list,
			source_type_family_decline_join,
			source_type_family_decline_all_join,
			source_type_family_leave,
			source_type_family_prayer_distribute,
			source_type_family_prayer_gift,
			source_type_family_prayer_receive,
			source_type_family_prayer_gift_back,
			source_type_family_prayer_complete,
			source_type_family_prayer_week_gift_rwd,
			source_type_family_prayer_day_time_refresh,
			source_type_family_prayer_week_time_refresh,
			source_type_family_prayer_gift_rwd,
			source_type_family_invite_join,
			source_type_family_money_add,
			source_type_family_money_cost,
			source_type_family_end,


            //country
            source_type_country_begin = 850,
            source_type_country_period_establish = source_type_country_begin,
            source_type_country_hegemony,
			source_type_country_king_family,
            source_type_country_alliance,
            source_type_country_dismiss_alliance,
            source_type_country_modify_year_name,
            source_type_country_modify_notice,
            source_type_country_appoint_officer,
            source_type_country_fire_officer,
            source_type_country_resign_officer,
            source_type_country_country_level,
			source_type_country_money_add,
			source_type_country_money_cost,
			source_type_country_change_name_flag,
			source_type_country_change_name_free,
			source_type_country_change_name,
			source_type_country_change_flag,
			source_type_country_change_year_name_free,
            source_type_country_end,

		    //trade
		    source_type_trade_begin = 900,
		    source_type_trade_buy_item = source_type_trade_begin,
		    source_type_trade_sell_item,
		    source_type_trade_supplement_item,
			source_type_trade_gm_add_trade_point,
			source_type_trade_init_trade_point,
		    source_type_trade_end,

            //friend
            source_type_friend_begin = 950,
            source_type_friend_add_friend = source_type_friend_begin,
            source_type_friend_remove_friend,
            source_type_friend_black_friend,
            source_type_friend_serach_by_id,
            source_type_friend_serach_by_name,
            source_type_friend_cancel_top_friend,
            source_type_friend_top_friend,
            source_type_friend_refresh_recommend_list,
            source_type_friend_get_recent_friends,
            source_type_friend_get_black_friends,
            source_type_friend_cancel_black_friend,
            source_type_friend_get_applier_list,
            source_type_friend_approve_add_friend,
            source_type_friend_clear_friends,
            source_type_friend_clear_recent_friends,
            source_type_friend_clear_black_friends,
            source_type_friend_clear_friend_appliers,
		    source_type_friend_clear_apply_time,
		    source_type_friend_clear_content_time,
		    source_type_friend_update_friend_type,
		    source_type_friend_update_apply_time,
		    source_type_friend_new_relation,
		    source_type_friend_update_content_time,
            source_type_friend_end,

		    //arena
		    source_type_arena_begin = 1000,
		    source_type_arena_buy_ticket = source_type_arena_begin,
		    source_type_arena_battle_win,
		    source_type_arena_daily,
		    source_type_arena_season_finish,
			source_type_arena_join,
			source_type_arena_battle_lose,
		    source_type_arena_end,

		    //pk
		    source_type_pk_begin = 1050,
		    source_type_pk_fight = source_type_pk_begin,
		    source_type_pk_attacker_win,
		    source_type_pk_attacker_lose,
		    source_type_pk_defenser_win,
		    source_type_pk_defenser_lose,
		    source_type_pk_end,

		    // shop
            source_type_shop_begin = 1100,
            source_type_shop_buy = source_type_shop_begin,
		    source_type_shop_buy_center_failed_back,
            source_type_buy_no_limit,
            source_type_buy_limit,
			source_type_random_shop_buy,
			source_type_random_shop_item,
			source_type_random_shop_free_refresh,
			source_type_random_shop_cost_refresh,
			source_type_random_shop_time_refresh,
			source_type_family_shop_buy,
			source_type_family_shop_item,
		    source_type_shop_end,


		    //equip
		    source_type_equip_begin = 1150,
		    source_type_equip_exchange = source_type_equip_begin,
		    source_type_equip_remake_attr,
		    source_type_equip_remake_exattr,
		    source_type_equip_smelt,
		    source_type_equip_on,
		    source_type_equip_off,
		    source_type_equip_replace_exattr,
		    source_type_equip_smelt_box,
			source_type_equip_strengthen,
		    source_type_equip_end,

		    //tower
		    source_type_tower_begin = 1200,
		    source_type_tower_fight = source_type_tower_begin,
		    source_type_tower_auto_fight,
		    source_type_tower_auto_fight_immediately,
		    source_type_tower_auto_fight_reward,
		    source_type_tower_achieve_reward,
			source_type_tower_fight_win,
		    source_type_tower_end,

		    //achieve
		    source_type_achieve_begin = 1250,
		    source_type_achieve_person_reward = source_type_achieve_begin,	///成就奖励
		    source_type_achieve_single_finish,								///完成成就
		    source_type_achieve_add_count,									///增加进度
		    source_type_achieve_main_role_level,							///主角达到等级n	
	        source_type_achieve_main_role_fight,							///主角战斗力达到n
	        source_type_achieve_equip_type_number,							///获得n件指定品质的装备
	        source_type_achieve_equip_type_same,							///集齐n品质套装
	        source_type_achieve_equip_recast,								///重铸n次装备
	        source_type_achieve_equip_exattr_number_star,					///装备同时拥有n个指定星判词
	        source_type_achieve_hero_number,								///拥有n名武将
	        source_type_achieve_hero_number_level,							///拥有n名指定级武将
	        source_type_achieve_hero_number_type,							///拥有指定品质武将n名
	        source_type_achieve_hero_number_star ,							///拥有n名指定星级武将
	        source_type_achieve_hero_number_talent_level,					///将n名武将天赋提升至指定级
	        source_type_achieve_hero_number_skill_level,					///将n名武将技能提升至指定级
	        source_type_achieve_total_copper,								///累计获得n个铜币
	        source_type_achieve_shop_buy_number,							///在n类商店购物n次
	        source_type_achieve_mount_number_type,							///获得指定品质坐骑n匹
	        source_type_achieve_task_finish_number,							///完成n个任务
	        source_type_achieve_task_finish_country_number,					///完成国家任务n次
	        source_type_achieve_task_finish_family_number,					///完成家族任务n次
	        source_type_achieve_trade_number,								///完成跑商n次
	        source_type_achieve_trade_total_gain_number,					///跑商累计利润达到n
	        source_type_achieve_country_bounty_star_number,					///完成n星国家悬赏n次
	        source_type_achieve_treasure_activity_number,					///完成寻宝活动n次
	        source_type_achieve_treasure_activity_special_event_number,		///寻宝活动遭遇n次特殊事件
	        source_type_achieve_channel_chat,								///使用n频道n次
	        source_type_achieve_family,										///加入一个家族:包括担任官职
	        source_type_achieve_country,									///加入一个国家:包括担任官职
	        source_type_achieve_empire_city_challenge_number,				///进行n次皇城约战
	        source_type_achieve_pk_value,									///pk值达到n
	        source_type_achieve_package_copper_number,						///包裹内持有n枚铜币	
			source_type_achieve_task_finish_trial_number,					///完成[]次试炼任务
			source_type_achieve_have_friend_number,							///累计添加[n]好友
			source_type_achieve_trade_single_gain ,							///跑商单次利润达到[]
			source_type_achieve_task_finish_star_trial_number ,				///完成[m]星试炼任务[n]次
			source_type_achieve_total_login ,								///累计登陆
			source_type_achieve_succession_login ,							///连续登陆
			source_type_achieve_task_finish_chase_ghost_number ,			///捉鬼任务完成[]次
			source_type_achieve_chase_ghost_circle_number ,					///捉鬼任务完成环[]数
			source_type_achieve_field_boss_kill_number ,					///野外boss击杀[]数
			source_type_achieve_arena_battle_single_win_number ,			///跨服竞技单次[]胜
			source_type_achieve_arena_battle_total_win_number ,				///跨服竞技总计[]胜
			source_type_achieve_tower_level ,								///千层塔达到[]层
			source_type_achieve_expedition_single_point ,					///远征单次积分达到[]
			source_type_achieve_expedition_store_buy_number ,				///远征在神秘商店购买[]次货物
			source_type_achieve_expedition_store_total_cost_honour ,		///神秘商店累计消耗[]荣耀值
			source_type_achieve_expedition_help_others ,					///远征帮助他人[]次
			source_type_achieve_expedition_challenge_pass_number ,			///远征挑战15次关卡[m]次
			source_type_achieve_family_war_number ,							///参加家族战[]次
			source_type_achieve_family_war_kill_player_number ,				///家族战击杀玩家[]次
			source_type_achieve_family_war_dragon_harm ,					///家族战中对龙柱造成伤害[]
			source_type_achieve_family_war_occupied_sun_or_moon_number ,	///家族战中占领日月曜台[]次
			source_type_achieve_king_war_number ,							///王城战[]次数
			source_type_achieve_king_war_kill_player_number ,				///王城战击杀玩家[]次
			source_type_achieve_king_war_dragon_harm ,						///王城战中对龙脉造成伤害[]
			source_type_achieve_king_war_quick_revive ,						///王城战中立即复活[]次
			source_type_achieve_king_war_start_weapon ,						///王城战中启动神器[]次
			source_type_achieve_grade_pass_dungeon_number ,					///以[]评分通关副本[]次
			source_type_achieve_dungeon_total_pass,							///名将挑战累计通过[]次
			source_type_achieve_rank_type_level ,							///[]排行榜达到[]名次
			source_type_achieve_task_finish_exp_circle_number ,				///经验环任务完成[]次
			source_type_achieve_task_exp_circle_item_quality_number ,		///经验环中提交x品质的物品y次

			source_type_achieve_hero_up_level_number,						///提升武将等级x次
			source_type_achieve_strengthen_equip_number,					///强化x次装备
			source_type_achieve_tower_number,								///爬塔x次层
			source_type_achieve_patrol_kill_monster_group_number,			///巡逻中击杀x组怪

	        source_type_achieve_end,

		    //lifeforce
		    source_type_lifeforce_begin = 1400,
		    source_type_lifeforce_up_lifestar_level = source_type_lifeforce_begin,
		    source_type_lifeforce_up_lifestate_level,
			source_type_lifeforce_up_lifelabel_level,
		    source_type_lifeforce_end,

		    //dungeon
		    source_type_dungeon_begin = 1450,
		    source_type_dungeon_open = source_type_dungeon_begin,
		    source_type_dungeon_enter,
		    source_type_dungeon_leave,
			source_type_dungeon_pass,
			source_type_dungeon_first_pass,
		    source_type_dungeon_end,
		
		    //role_buff
		    source_type_role_buff_begin = 1500,
		    source_type_role_buff_timeout = source_type_role_buff_begin,
		    source_type_role_buff_replace,
		    source_type_role_buff_end,

		    //goods
		    source_type_goods_begin = 1550,
		    source_type_goods_buy = source_type_goods_begin,
		    source_type_goods_sell,
		    source_type_goods_return,
		    source_type_goods_resell,
		    source_type_goods_profit,
			source_type_goods_cancel,
		    source_type_goods_end,

            //account
            source_type_account_begin = 1560,
            source_type_account_registered = source_type_account_begin,
            source_type_account_login,
            source_type_account_create_role,
            source_type_account_end,

			//new_guide
            source_type_new_guide_begin = 1600,
            source_type_finish_new_guide = source_type_new_guide_begin,
            source_type_new_guide_end,
			
			//page
			source_type_page_begin = 1610,
			source_type_page_pass = source_type_page_begin,
			source_type_page_task_finish,
			source_type_page_pass_fight_pve,
			source_type_page_reward_login_auto_send,
			source_type_page_task_quick_finish,

			source_type_page_end,

			//expedition
			source_type_expedition_begin = 1630,
			source_type_expedition_win = source_type_expedition_begin,
			source_type_expedition_shop,
			source_type_expedition_award,
			source_type_expedition_box,
			source_type_expedition_refresh,
			source_type_expedition_help,
			source_type_expedition_call_help,
			source_type_expedition_end,

			///active_reward
			source_type_active_reward_begin = 1640,
			source_type_active_reward_finish = source_type_active_reward_begin,
			source_type_active_reward_end,

			///充值
			source_type_recharge_begin = 1645,
			source_type_recharge_success = source_type_recharge_begin,
			source_type_recharge_gift,
			source_type_recharge_end,

			///活动
			source_type_activity_begin = 1650,
			source_type_activity_done = source_type_activity_begin,
			source_type_activity_new_day,
			source_type_activity_end,

			///场景日志
			source_type_scene_begin = 1700,
			source_type_scene_jump = source_type_scene_begin,
			source_type_scene_end,

			//离线竞技
			source_type_offline_arena_begin = 1750,
			source_type_offline_arena_record_award = source_type_offline_arena_begin,
			source_type_offline_arena_class_award,
			source_type_offline_arena_buy,
			source_type_offline_arena_win,
			source_type_offline_arena_lose,
			source_type_offline_arena_end,

			//运营类活动
			source_type_bussiness_activity_begin = 1850,
			source_type_bussiness_money_tree	  = source_type_bussiness_activity_begin,
			source_type_bussiness_seven_day_login,
			source_type_bussiness_activity_end,

// 			///战斗日志 暂时用不到
// 			source_type_combat_begin = 1800,
// 			source_type_combat_finish = source_type_combat_begin,
// 			source_type_combat_end,
    };

    enum opt_type_t
    {
        opt_type_none = 0,
        opt_type_add = 1,
        opt_type_del = 2,
        opt_type_set = 3,
    };

    enum mobile_type_t : uint32_t
    {
        mobile_type_android      = 0,
        mobile_type_apple        = 1,
    };
};

NS_COMMON_END

#endif
