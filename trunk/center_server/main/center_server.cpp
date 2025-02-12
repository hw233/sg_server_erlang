#include "center_server.hpp"
#include "center_server_fwd.hpp"
#include "log.hpp"
#include "gate/gate_msg_handler.hpp"
#include "gate/gate_manager.hpp"
#include "game/game_msg_handler.hpp"
#include "game/game_manager.hpp"
#include "db/db_msg_handler.hpp"
#include "user/global_user_manager.hpp"
#include "common/global_id.hpp"
#include "redis_client.hpp"
#include "gm/gm_msg_handler.hpp"
#include "config/config_manager.hpp"
#include "city/city_manager.hpp"
#include "chat/chat_msg_handler.hpp"
#include "rank/rank_manager.hpp"
#include "country/country_mgr.hpp"
#include "field_boss/field_boss_manager.hpp"
#include "common/time_manager.hpp"
#include "item/item_manager.hpp"
#include "mail/global_mail_manager.hpp"
#include "transfer/transfer_msg_handler.hpp"
#include "transfer/transfer_manager.hpp"
#include "friend/friend_manager.hpp"
#include "challenge/challenge_manager.hpp"
#include "offline_friend_msg_mgr/offline_friend_msg_mgr.hpp"
#include "king_war/king_war_manager.hpp"
#include "shop/shop_manager.hpp"
#include "scene/global_scene_manager.hpp"
#include "goods/goods_manager.hpp"
#include "redbag/redbag_manager.hpp"
#include "msg_count.hpp"
#include "offline_fight/offline_fight_manager.hpp"
#include "tblh/time_type_enum.hpp"
#include "redis/global_data_mgr.hpp"
#include "offline_arena/offline_arena_manager.hpp"
#include "luckydraw/luckydraw_msg_handle.hpp"
#include "luckydraw/luckydraw_mgr.hpp"
#include "general_event/general_event_manager.hpp"
#include "general_event/general_info_manager.hpp"
#include "sys_notice/sys_notice_manager.hpp"
#include "common/redis_db_table.hpp"

USING_NS_NETWORK;
USING_NS_COMMON;

center_server_t::center_server_t()
{
}

center_server_t::~center_server_t()
{
}

bool center_server_t::on_init_server()
{
    CMD_INIT;

    // xml
    if (!load_xml())
    {
        close_xml();
        COMMON_ASSERT(false);
        return false;
    }

    // logs
    if (!g_log.open(env::xml->log_path, server_name(), env::xml->is_log_trace, env::xml->is_log_debug, env::xml->is_log_info, env::xml->is_new_logfile_per_hour))
    {
        COMMON_ASSERT(false);
        return false;
    }

    // config
    if (!load_config())
    {
        COMMON_ASSERT(false);
        return false;
    }

    // zdb
    if (!init_zdb())
    {
        COMMON_ASSERT(false);
        return false;
    }

    // redis
    if (!init_redis())
    {
        COMMON_ASSERT(false);
        return false;
    }

    // game logic
    if (!init_game_logic())
    {
        COMMON_ASSERT(false);
        return false;
    }

    // network
    if (!init_network())
    {
        COMMON_ASSERT(false);
        return false;
    }
    
    return true;
}

bool center_server_t::on_close_server()
{
	stop_network();

    {
        clear_game_logic();
        release_config();
    }

    close_network();

    close_redis();

    close_zdb();

    close_xml();

    g_log.close();

    CMD_CLOSE;

    return true;
}

void center_server_t::on_run_server()
{
    log_info("center server running...");
}

//----------------------------------------------------------------------------------------------------
bool center_server_t::load_xml()
{
    if (!load_server_xml())
    {
        printf("\033[31mload [%s] failed!\033[0m\n", SERVER_XML_PATH);
        return false;
    }
    printf("load [%s] success!\n", SERVER_XML_PATH);

    //if (!load_channel_xml())
    //{
    //    printf("\033[31mload [%s] failed!\033[0m\n", CHANNEL_XML_PATH);
    //    return false;
    //}
    //printf("load [%s] success!\n", CHANNEL_XML_PATH);

    return true;
}
bool center_server_t::load_server_xml(bool reload)
{
    try
    {
        boost::property_tree::ptree pt;
        boost::property_tree::xml_parser::read_xml(SERVER_XML_PATH, pt);

		if (env::xml == NULL) {
			env::xml = new center_xml_t();
		}

        if(!reload){
            env::xml->srv_group.group_id = pt.get<uint32_t>("server.server_id");
            env::xml->srv_group.start_time = pt.get<std::string>("server.start_time");
    		m_server_id = env::xml->srv_group.group_id;
    		if (m_server_id == 0)
    		{
    			log_error("server_id = 0");
    			return false;
    		}

            /*std::string str_channel_list = pt.get<std::string>("server.channel");
            std::vector<uint32_t> channel_list;
            string_util_t::split(str_channel_list, env::xml->srv_group.channel_list, ";", false);
            if (env::xml->srv_group.channel_list.empty())
            {
                printf("\033[31menv::xml->srv_group.channel_list is empty!\033[0m\n");
                return false;
            }*/

            env::xml->thread_num.network = pt.get<uint32_t>("server.center_server.thread_num.network");

            //listen gate
            env::xml->listen_at_gate.ip = pt.get<std::string>("server.center_server.listen.gate.ip");
            env::xml->listen_at_gate.port = pt.get<uint32_t>("server.center_server.listen.gate.port");
            CHECK_PORT(env::xml->listen_at_gate.port);

            //listen game
            env::xml->listen_at_game.ip = pt.get<std::string>("server.center_server.listen.game.ip");
            env::xml->listen_at_game.port = pt.get<uint32_t>("server.center_server.listen.game.port");
            CHECK_PORT(env::xml->listen_at_game.port);

            env::xml->listen_at_gm.ip = pt.get<std::string>("server.center_server.listen.gm.ip");
            env::xml->listen_at_gm.port = pt.get<uint32_t>("server.center_server.listen.gm.port");
            CHECK_PORT(env::xml->listen_at_gm.port);

    		env::xml->listen_at_chat.ip = pt.get<std::string>("server.center_server.listen.chat.ip");
    		env::xml->listen_at_chat.port = pt.get<uint32_t>("server.center_server.listen.chat.port");
    		CHECK_PORT(env::xml->listen_at_chat.port);

            //listen recharge
            //env::xml->listen_at_recharge.ip   = pt.get<std::string>("server.center_server.listen.recharge.ip");
            //env::xml->listen_at_recharge.port = pt.get<uint32_t>("server.center_server.listen.recharge.port");
            //CHECK_PORT(env::xml->listen_at_recharge.port);

            //connect db
            env::xml->connect_to_db.ip = pt.get<std::string>("server.db_server.listen.center.ip");
            env::xml->connect_to_db.port = pt.get<uint32_t>("server.db_server.listen.center.port");
            CHECK_PORT(env::xml->connect_to_db.port);
    		
            //connect log
            env::xml->connect_to_log.ip = pt.get<std::string>("server.log_server.listen.center.ip");
            env::xml->connect_to_log.port = pt.get<uint32_t>("server.log_server.listen.center.port");
            CHECK_PORT(env::xml->connect_to_log.port);

            //connect transfer
            {
                std::string str_ip_list = pt.get<std::string>("server.center_server.connect.transfer.ip");
                std::vector<std::string> ip_list;
                string_util_t::split(str_ip_list, ip_list, ";");
                if (ip_list.empty())
                {
                    printf("\033[31mconnect to transfer: ip_list is empty!\033[0m\n");
                    return false;
                }

                std::string str_port_list = pt.get<std::string>("server.center_server.connect.transfer.port");
                std::vector<std::string> port_list;
                string_util_t::split(str_port_list, port_list, ";");
                if (port_list.empty())
                {
                    printf("\033[31mconnect to transfer: port_list is empty!\033[0m\n");
                    return false;
                }

                if (ip_list.size() != port_list.size())
                {
                    printf("\033[31mconnect to transfer: ip_size[%d] != port_size[%d]\033[0m\n", uint32_t(ip_list.size()), uint32_t(port_list.size()));
                    return false;
                }

                for (uint32_t i = 0; i < ip_list.size(); ++i)
                {
                    connect_t transfer_connector;
                    transfer_connector.ip = ip_list[i];
                    transfer_connector.port = boost::lexical_cast<uint32_t>(port_list[i]);
                    CHECK_PORT(transfer_connector.port);
                    env::xml->connect_to_transfer_list.push_back(transfer_connector);
                }

                m_transfer_connector_list.resize(env::xml->connect_to_transfer_list.size());
            }

            //mysql
    //         env::xml->mysql.host = pt.get<std::string>("server.center_server.mysql.host");
    //         env::xml->mysql.db = pt.get<std::string>("server.center_server.mysql.db");
    //         env::xml->mysql.user = pt.get<std::string>("server.center_server.mysql.user");
    //         env::xml->mysql.pwd = pt.get<std::string>("server.center_server.mysql.pwd");
    //         env::xml->mysql.init_conn = pt.get<uint32_t>("server.center_server.mysql.init_connections", 0);
    //         env::xml->mysql.max_conn = pt.get<uint32_t>("server.center_server.mysql.max_connections");

            //redis
    		env::xml->redis.host = pt.get<std::string>("server.center_server.redis.host");
    		env::xml->redis.port = pt.get<uint32_t>("server.center_server.redis.port");
    		env::xml->redis.pwd = pt.get<std::string>("server.center_server.redis.pwd");
    		env::xml->redis.db = pt.get<uint32_t>("server.center_server.redis.db");
			env::xml->redis.conn_overtime = pt.get<uint32_t>("server.center_server.redis.conn");
        }
        std::string str_ip_white_list = pt.get<std::string>("server.common.ip_white_list");
        string_util_t::split(str_ip_white_list, env::xml->ip_white_list, ";");
        if (env::xml->ip_white_list.empty())
        {
            printf("\033[31mip_white_list is empty!\033[0m\n");
            return false;
        }

        // print logs
        env::xml->log_path = pt.get<std::string>("server.common.log_path");
        env::xml->is_log_trace = pt.get<bool>("server.common.log_trace");
        env::xml->is_log_debug = pt.get<bool>("server.common.log_debug");
		env::xml->is_log_info = pt.get<bool>("server.common.log_info");
		env::xml->is_log_msg_count = pt.get<bool>("server.common.log_msg_count");
		env::xml->is_new_logfile_per_hour = pt.get<bool>("server.common.new_logfile_per_hour");

        env::xml->reconnect_interval_time = pt.get<uint32_t>("server.common.reconnect_interval_time");
        env::xml->db_log_count_limit = pt.get<uint32_t>("server.common.db_log_count_limit");
        env::xml->config_path = pt.get<std::string>("server.common.path.config");

		uint32_t pk = pt.get<uint32_t>("server.common.log.pk");
		env::xml->log_debug.pk = pk / 10;
		env::xml->log_sql.pk = pk % 10;
		uint32_t rank = pt.get<uint32_t>("server.common.log.rank");
		env::xml->log_debug.rank = rank / 10;
		env::xml->log_sql.rank = rank % 10;
		uint32_t role = pt.get<uint32_t>("server.common.log.role");
		env::xml->log_debug.role = role / 10;
		env::xml->log_sql.role = role % 10;
		uint32_t boss = pt.get<uint32_t>("server.common.log.boss");
		env::xml->log_debug.boss = boss / 10;
		env::xml->log_sql.boss = boss % 10;
		uint32_t friends = pt.get<uint32_t>("server.common.log.friend");
		env::xml->log_debug.friends = friends / 10;
		env::xml->log_sql.friends = friends % 10;
		uint32_t kingwar = pt.get<uint32_t>("server.common.log.kingwar");
		env::xml->log_debug.kingwar = kingwar / 10;
		env::xml->log_sql.kingwar = kingwar % 10;
		uint32_t mail = pt.get<uint32_t>("server.common.log.mail");
		env::xml->log_debug.mail = mail / 10;
		env::xml->log_sql.mail = mail % 10;
		uint32_t scene = pt.get<uint32_t>("server.common.log.scene");
		env::xml->log_debug.scene = scene / 10;
		env::xml->log_sql.scene = scene % 10;
		uint32_t shop = pt.get<uint32_t>("server.common.log.shop");
		env::xml->log_debug.shop = shop / 10;
		env::xml->log_sql.shop = shop % 10;
		uint32_t goods = pt.get<uint32_t>("server.common.log.goods");
		env::xml->log_debug.goods = goods / 10;
		env::xml->log_sql.goods = goods % 10;
		uint32_t family = pt.get<uint32_t>("server.common.log.family");
		env::xml->log_debug.family = family / 10;
		env::xml->log_sql.family = family % 10;
		uint32_t troop = pt.get<uint32_t>("server.common.log.troop");
		env::xml->log_debug.troop = troop / 10;
		env::xml->log_sql.troop = troop % 10;
		uint32_t expedition = pt.get<uint32_t>("server.common.log.expedition");
		env::xml->log_debug.expedition = expedition / 10;
		env::xml->log_sql.expedition = expedition % 10;
		uint32_t redbag = pt.get<uint32_t>("server.common.log.redbag");
		env::xml->log_debug.redbag = redbag / 10;
		env::xml->log_sql.redbag = redbag % 10;
		uint32_t item = pt.get<uint32_t>("server.common.log.item");
		env::xml->log_debug.item = item / 10;
		env::xml->log_sql.item = item % 10;
		uint32_t offline_arena = pt.get<uint32_t>("server.common.log.offline_arena");
		env::xml->log_debug.offline_arena = offline_arena / 10;
		env::xml->log_sql.offline_arena = offline_arena % 10;
		uint32_t country = pt.get<uint32_t>("server.common.log.country");
		env::xml->log_debug.country = country / 10;
		env::xml->log_sql.country = country % 10;
    }
    catch (boost::property_tree::ptree_error& e)
    {
        printf("\033[31mthrow error[%s]\033[0m\n", e.what());
        return false;
    }

    if (NULL == env::xml)
    {
        printf("\033[31menv::xml is NULL!\033[0m\n");
        return false;
    }

    if(reload){
        log_info("reload server_xml success!");
    }
    
    return true;
}
bool center_server_t::load_channel_xml()
{
    //try
    //{
    //    if (env::xml->srv_group.channel_list.empty())
    //    {
    //        log_error("env::xml->srv_group.channel_list is empty!");
    //        return false;
    //    }

    //    boost::property_tree::ptree pt;
    //    boost::property_tree::xml_parser::read_xml(CHANNEL_XML_PATH, pt);

    //    for (uint32_t i = 0; i < env::xml->srv_group.channel_list.size(); ++i)
    //    {
    //        uint32_t channel_id = env::xml->srv_group.channel_list[i];

    //        char buf[512] = { 0 };
    //        snprintf(buf, sizeof(buf), "channel.channel%d", channel_id);
    //        auto item = pt.get_child_optional(buf);
    //        if (!item)
    //        {
    //            continue;
    //        }
    //        std::string path(buf);
    //        channel_t channel;
    //        channel.type = pt.get<uint32_t>(path + ".type");
    //        channel.channel_name = pt.get<std::string>(path + ".channel_name");
    //        channel.game_name = pt.get<std::string>(path + ".game_name");
    //        channel.url = pt.get<std::string>(path + ".url");
    //        channel.login_sk = pt.get<std::string>(path + ".login_sk");
    //        channel.order_sk = pt.get<std::string>(path + ".order_sk");
    //        channel.gift_bag_code_url = pt.get<std::string>(path + ".gift_bag_code_url");
    //        channel.gift_bag_code_sk = pt.get<std::string>(path + ".gift_bag_code_sk");
    //        channel_xml_manager_t::add_channel(channel.type, channel);
    //    }


    //    // check channel xml
    //    if (channel_xml_manager_t::get_channel_size() < 1)
    //    {
    //        printf("\033[31mchannel's size[%d] at channel.xml < 1\033[0m\n", channel_xml_manager_t::get_channel_size());
    //        return false;
    //    }
    //}
    //catch (boost::property_tree::ptree_error& e)
    //{
    //    printf("\033[31mthrow error[%s]\033[0m\n", e.what());
    //    return false;
    //}

    return true;
}
void center_server_t::close_xml()
{
    SAFE_DELETE_P(env::xml);
    log_info("close xml success!");
}

//-------------------------------------------------------------------------------------------------------------
bool center_server_t::init_game_logic()
{
    // global id
    if (!g_id_allocator.init(env::xml->srv_group.group_id, 0, env::xml->srv_group.start_time, false))
    {
        return false;
    }

    // 活动时间管理器初始化
    if (!time_manager_t::init(env::server->get_ios(), env::xml->srv_group.start_time))
    {
        log_error("init timer failed!");
        return false;
    }

    // 跨天的callback
    time_manager_t::register_func(time_type_enum::time_one_day, center_server_t::one_day);
    log_info("init timer success!");

	if (!rank_manager_t::init())
	{
		log_error("init rank failed");
		return false;
	}
	log_info("init rank success!");

	if (!global_scene_manager_t::init())
	{
		log_error("init global_scene_manager_t failed");
		return false;
	}
	log_info("init global_scene_manager_t success!");

    if (!global_mail_manager_t::init())
    {
        log_error("init global mail failed");
        return false;
    }
    log_info("init global mail success!");

	if (!country_mgr_t::init())
	{
		log_error("init country failed");
		return false;
	}
	log_info("init country success!");

    if (!city_manager_t::init())
    {
        log_error("init city failed");
        return false;
    }
    log_info("init city success!");

    if (!item_manager_t::init())
    {
        log_error("init item failed");
        return false;
    }
    log_info("init item success!");

    if (!field_boss_manager_t::init())
    {
        log_error("init field_boss failed");
        return false;
    }
    log_info("init field_boss success!");

    if (!friend_manager_t::init())
    {
        log_error("init friend_manager_t failed");
        return false;
    }
    log_info("init friend_manager_t success!");

    challenge_manager_t::load_data();
    log_info("init challenge success!");

    // king war
    if (!king_war_manager_t::init())
    {
        log_error("init king_war failed");
        return false;
    }
    log_info("init king_war success!");

	// shop
	if (!shop_manager_t::init())
	{
		log_error("init shop failed");
		return false;
	}
	log_info("init shop success!");

	if (!luckydraw_mgr_t::init())
	{
		log_error("init luckydraw_mgr failed");
		return false;
	}
	log_info("init luckydraw_mgr success!");

	if (!general_event_manager_t::init())
	{
		log_error("init general_event failed");
		return false;
	}
	log_info("init general_event success!");

	//goods
	goods_manager_t::load_data();
	log_info("init goods success!");

	//offline_fight
	offline_fight_manager_t::load_data();

	//redbag
	redbag_manager_t::load_data();
	log_info("init redbag success!");

	//offline_arena
	offline_arena_manager_t::load_data();
	log_info("init offline_arena success!");

	//general_info
	general_info_manager_t::load_data();
	log_info("init general_info success!");

	//general_event
	general_event_manager_t::load_data();
	log_info("init general_event success!");

	//初始化公告
	sys_notice_manager_t::init();
	log_info("init sys_notify_manager success!");

	// 这个放在最后，等所有别的模块都初始化完成
	// 检测开服的时候跨天处理
	uint32_t last_update_time = global_data_mgr_t::load_day_update_time();
	if (last_update_time == 0) {
		global_data_mgr_t::save_day_update_time(common::time_util_t::now_time());
	} else if (last_update_time > 0 && !time_manager_t::check_same_period(time_type_enum::time_one_day, last_update_time)) {
		center_server_t::open_update_one_day();
	}

    return true;
}
void center_server_t::clear_game_logic()
{
	time_manager_t::cancel_timer();
	gate_manager_t::clear_gate();
	game_manager_t::clear_game();
	global_user_manager_t::close_clear();
	field_boss_manager_t::close_clear();
	global_mail_manager_t::close_clear();
	offline_friend_msg_mgr_t::close_clear();
	item_manager_t::close();
	country_mgr_t::close();
	shop_manager_t::close();
	global_scene_manager_t::close();
	goods_manager_t::stop_timer();
	redbag_manager_t::stop_timer();
	sys_notice_manager_t::close_clear();
	challenge_manager_t::close_timer();

    log_info("clear game logic success!");
}

//-------------------------------------------------------------------------------------------------------------
bool center_server_t::init_network()
{
    if (!m_network.init(env::xml->thread_num.network, env::xml->ip_white_list))
    {
        return false;
    }

    // listen for gate
    if (!m_network.listen_at(env::xml->listen_at_gate.ip, env::xml->listen_at_gate.port, &m_gate_listener))
    {
        return false;
    }
    else
    {
        if (!gate_msg_handler_t::init_msg_handler())
        {
            log_error("init gate msg handler failed!");
            return false;
        }
    }

    // listen for game
    if (!m_network.listen_at(env::xml->listen_at_game.ip, env::xml->listen_at_game.port, &m_game_listener))
    {
        return false;
    }
    else
    {
        if (!game_msg_handler_t::init_msg_handler())
        {
            log_error("init game msg handler failed!");
            return false;
        }

    }

    // listen for gm
    if (!m_network.listen_at(env::xml->listen_at_gm.ip, env::xml->listen_at_gm.port, &m_gm_listener))
    {
        return false;
    }
    else
    {
        if (!gm_msg_handler_t::init_msg_handler())
        {
            log_error("init gm msg handler failed!");
            return false;
        }
    }

    // listen for chat
    if (!m_network.listen_at(env::xml->listen_at_chat.ip, env::xml->listen_at_chat.port, &m_chat_listener))
    {
        return false;
    }
    else
    {
        if (!chat_msg_handler_t::init_msg_handler())
        {
            log_error("init chat msg handler failed!");
            return false;
        }
    }

    // connect to db
	if (!m_network.connect_to(env::xml->connect_to_db.ip, env::xml->connect_to_db.port, &m_db_connector, env::xml->reconnect_interval_time))
	{
		return false;
	}
	else
	{
		if (!db_msg_handler_t::init_msg_handler())
		{
			log_error("init db msg handler failed!");
			return false;
		}
	}

    // connect to log
    if (!m_network.connect_to(env::xml->connect_to_log.ip, env::xml->connect_to_log.port, &m_log_connector, env::xml->reconnect_interval_time))
    {
        return false;
    }

    // connect to transfer
    for (size_t idx = 0; idx < env::xml->connect_to_transfer_list.size(); ++idx)
    {
        connect_t connector = env::xml->connect_to_transfer_list.at(idx);
        if (!m_network.connect_to(connector.ip, connector.port, &m_transfer_connector_list[idx], env::xml->reconnect_interval_time))
        {
            return false;
        }
    }
    if (!transfer_msg_handler_t::init_msg_handler())
    {
        log_error("init transfer msg handler failed!");
        return false;
    }

    return true;
}
void center_server_t::close_network()
{
	msg_count_t::write_msg_recv_count_data_file(env::xml->is_log_msg_count);
	msg_count_t::write_msg_send_count_data_file(env::xml->is_log_msg_count);

    transfer_manager_t::clear_transfer();

    m_network.close();
}

void center_server_t::stop_network()
{
	m_network.stop();
}

//--------------------------------------------------------------------------------------------
bool center_server_t::init_zdb()
{
// 	boost::tie(m_db_url, env::conn_zdb_pool) = zdb_util_t::open_zdb(
// 		env::xml->mysql.host,
// 		env::xml->mysql.db,
// 		env::xml->mysql.user,
// 		env::xml->mysql.pwd,
// 		env::xml->mysql.max_conn,
// 		env::xml->mysql.init_conn);
// 	if (NULL == env::conn_zdb_pool)
// 	{
// 		log_error("init zdb connection pool failed!");
// 		return false;
// 	}

    return true;
}
void center_server_t::close_zdb()
{
    //zdb_util_t::close_zdb(env::conn_zdb_pool, m_db_url);
}

//--------------------------------------------------------------------------------------------------
bool center_server_t::init_redis()
{
	if (!redis_client_t::init(env::server->get_ios(),
		env::xml->redis.host.c_str(),
		env::xml->redis.port,
		env::xml->redis.pwd.c_str(),
		env::xml->redis.db,
		all_db_table,
		env::xml->redis.conn_overtime))
	{
		log_error("init redis client failed!");
		return false;
	}

    return true;
}

void center_server_t::close_redis()
{
	redis_client_t::close();
}

//--------------------------------------------------------------------------------------------------
void center_server_t::on_add_gate(const tcp_socket_ptr& s)
{
    gate_manager_t::add_gate(s);
}
void center_server_t::on_del_gate(const tcp_socket_ptr& s)
{
	global_user_manager_t::kick_gate_all_users(gate_manager_t::get_gate_id_by_socket(s));

    gate_manager_t::del_gate(s);
}
void center_server_t::on_gate_msg(const tcp_socket_ptr& s, uint64_t uid, uint16_t cmd, const msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
    if (cmd > op_cmd::AM_BEGIN && cmd < op_cmd::AM_END)
    {
        send_msg_from_gate_to_gm(cmd, uid, msg_buf);
    }
    else
	{
        gate_msg_handler_t::handle_server_msg(s, uid, cmd, msg_buf);
    }
}

//--------------------------------------------------------------------------------------------------
void center_server_t::on_add_game(const tcp_socket_ptr& s)
{
	game_manager_t::add_game(s);

	//当game重连重新通知天下大势数据
	general_info_manager_t::sync_general_data();

	general_event_manager_t::sync_general_event_data();
}

void center_server_t::on_del_game(const tcp_socket_ptr& s)
{
	uint32_t game_id = game_manager_t::get_game_id_by_socket(s);

    global_user_manager_t::kick_game_all_users(game_id);

	global_scene_manager_t::on_del_game(game_id);

    game_manager_t::del_game(s);
}

void center_server_t::on_game_msg(const tcp_socket_ptr& s, uint64_t uid, uint16_t cmd, const msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
	if (cmd > op_cmd::GM_BEGIN && cmd < op_cmd::GM_END)
	{
		send_msg_from_game_to_gm(cmd, uid, msg_buf);
	}
	else if (cmd > op_cmd::GR_BEGIN && cmd < op_cmd::GR_END)
	{
		send_msg_from_game_to_transfer(cmd, uid, msg_buf);
	}
	else
	{
		game_msg_handler_t::handle_server_msg(s, uid, cmd, msg_buf);
	}
}

//--------------------------------------------------------------------------------------------------
void center_server_t::on_add_db(const tcp_socket_ptr& s)
{
    m_db_connector.set_socket(s);
}
void center_server_t::on_del_db(const tcp_socket_ptr& s)
{
    env::server->post_network_close_socket(s);
    m_db_connector.reset_socket();

	log_error("db server disconnect when center is alive");

	// db断开删除所有玩家连接
	global_user_manager_t::kick_all_global_users(common::KICK_ROLE_REASON_DB_SERVER_CLOSED);

	// 通知所有gate删除玩家
	proto::server::ea_kick_all_user_notify ntf_gate;
	ntf_gate.set_reason(common::KICK_ROLE_REASON_DB_SERVER_CLOSED);
	send_msg_to_all_gate(op_cmd::ea_kick_all_user_notify, 0, ntf_gate);

 	// 通知所有game删除玩家
 	proto::server::eg_kick_all_user_notify ntf_game;
 	ntf_game.set_reason(common::KICK_ROLE_REASON_DB_SERVER_CLOSED);
 	send_msg_to_all_games(op_cmd::eg_kick_all_user_notify, 0, ntf_game);
 
 	// 通知所有chat删除玩家
 	proto::server::eh_kick_all_user_notify ntf_chat;
 	ntf_chat.set_reason(common::KICK_ROLE_REASON_DB_SERVER_CLOSED);
 	send_msg_to_chat(op_cmd::eh_kick_all_user_notify, 0, ntf_chat);
}
void center_server_t::on_db_msg(uint64_t uid, uint16_t cmd, const msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
    db_msg_handler_t::handle_server_msg(uid, cmd, msg_buf);
}

//--------------------------------------------------------------------------------------------------
void center_server_t::on_add_gm(const tcp_socket_ptr& s)
{
    m_gm_listener.set_socket(s);
}
void center_server_t::on_del_gm(const tcp_socket_ptr& s)
{
    env::server->post_network_close_socket(s);
    m_gm_listener.close_socket();
}
void center_server_t::on_gm_msg(const network::tcp_socket_ptr& sock, uint64_t uid, uint16_t cmd, const network::msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
	if (cmd > op_cmd::MG_BEGIN && cmd < op_cmd::MG_END)
	{//gm to game
        global_user_ptr p_user = global_user_manager_t::get_global_user_by_uid(uid);
        if (NULL != p_user)
        {
            const network::tcp_socket_ptr& p_game_socket = p_user->get_game_socket();
            if (NULL == p_game_socket)
            {
                log_error("NULL == p_game_socket");
                return;
            }
            send_msg_from_gm_to_game(p_game_socket, cmd, uid, msg_buf);
        }
        else
        {
            send_msg_from_gm_to_all_games(cmd,uid, msg_buf);
        }
        
	}
    else if (cmd > op_cmd::MA_BEGIN && cmd < op_cmd::MA_END)
    {//gm to gate
        global_user_ptr p_user = global_user_manager_t::get_global_user_by_uid(uid);
        if (NULL != p_user)
        {
            const network::tcp_socket_ptr& p_gate_socket = p_user->get_gate_socket();
            if (NULL == p_gate_socket)
            {
                log_error("NULL == p_gate_socket");
                return;
            }
            send_msg_from_gm_to_gate(p_gate_socket, cmd, uid, msg_buf);
        }
        else
        {
            send_msg_from_gm_to_all_gates(cmd, msg_buf);
        }
    }
	else if (cmd > op_cmd::MH_BEGIN && cmd < op_cmd::MH_END)
	{
		send_msg_from_gm_to_chat(sock, cmd, uid, msg_buf);
	}
	else
	{//gm to center
		gm_msg_handler_t::handle_server_msg(sock, uid, cmd, msg_buf);
	}
}

//--------------------------------------------------------------------------------------------------
void center_server_t::on_add_log(const tcp_socket_ptr& s)
{
    m_log_connector.set_socket(s);
}
void center_server_t::on_del_log(const tcp_socket_ptr& s)
{
    env::server->post_network_close_socket(s);
    m_log_connector.close_socket();
}

//--------------------------------------------------------------------------------------------------
void center_server_t::on_add_chat(const network::tcp_socket_ptr& s)
{
	m_chat_listener.set_socket(s);
}

void center_server_t::on_del_chat(const network::tcp_socket_ptr& s)
{
	env::server->post_network_close_socket(s);
	m_chat_listener.close_socket();
}

void center_server_t::on_chat_msg(const network::tcp_socket_ptr& sock, uint64_t uid, uint16_t cmd, const network::msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
	if (cmd > op_cmd::HM_BEGIN && cmd < op_cmd::HM_END)
	{
		send_msg_from_chat_to_gm(cmd, uid, msg_buf);
	}
	else
	{
		chat_msg_handler_t::handle_server_msg(uid,cmd, msg_buf);
	}
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
void center_server_t::on_add_transfer(const tcp_socket_ptr& s)
{
	transfer_manager_t::add_transfer(s);

    // 同步center服务器信息给该transfer服务器
    proto::server::ef_syn_center_info_notify noti;
    noti.set_server_id(env::xml->srv_group.group_id);
    env::server->send_msg_to_transfer(s, op_cmd::ef_syn_center_info_notify, 0, noti);

}
void center_server_t::on_del_transfer(const tcp_socket_ptr& s)
{
	transfer_manager_t::del_transfer(s);
}
void center_server_t::on_transfer_msg(const network::tcp_socket_ptr& s, uint64_t uid, uint16_t cmd, const network::msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
	transfer_msg_handler_t::handle_server_msg(s,uid, cmd, msg_buf);
}

void center_server_t::one_day(uint32_t id, bool is_in_time)
{
    if(is_in_time)
    {
    	log_info("one_day");
		time_t cur_time = common::time_util_t::now_time();

		// 国家每日更新（先于global_user_manager_t::one_day）
		country_mgr_t::one_day();

		global_user_manager_t::one_day(cur_time);
        field_boss_manager_t::new_day(cur_time);
		challenge_manager_t::one_day();
		offline_fight_manager_t::one_day();
		offline_arena_manager_t::one_day();
		global_data_mgr_t::save_day_update_time(cur_time);
		general_info_manager_t::one_day();
		friend_mgr_t::update_online(cur_time);
    }
}

void center_server_t::open_update_one_day()
{
	log_info("open_update_one_day");
	time_t cur_time = common::time_util_t::now_time();
	// 开服后需要每天跨天处理的逻辑放这里
	country_mgr_t::one_day();
	// 记录更新时间
	global_data_mgr_t::save_day_update_time(cur_time);
}

void center_server_t::post_network_send_msg(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& buf)
{
	msg_count_t::push_msg_send_info(buf, env::xml->is_log_msg_count);
	m_network.post_send_msg(s, buf);
}

