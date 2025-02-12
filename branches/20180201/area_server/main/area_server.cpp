#include "area_server.hpp"
#include "area_server_fwd.hpp"
#include "common/log.hpp"
#include "config/config_manager.hpp"
#include "common/redis_client.hpp"
#include "transfer/transfer_msg_handler.hpp"
#include "cross/cross_msg_handler.hpp"
#include "role/arena_role_manager.hpp"
#include "common/time_manager.hpp"
#include "arena/arena_manager.hpp"
#include "arena/arena_score_rank_mgr.hpp"
#include "world_cup/world_cup_manager.hpp"
#include "common/global_id.hpp"
#include "msg_count.hpp"
#include "tblh/time_type_enum.hpp"
#include "hero_strategy/hero_strategy_manager.hpp"

USING_NS_NETWORK;
USING_NS_COMMON;

area_server_t::area_server_t()
{
}

area_server_t::~area_server_t()
{
}

bool area_server_t::on_init_server()
{
	CMD_INIT;

    // xml
    if (!load_xml())
    {
        close_xml();
        COMMON_ASSERT(false);
        return false;
    }

    // log
    if (!g_log.open(env::xml->log_path, server_name(), env::xml->is_log_trace, env::xml->is_log_debug, env::xml->is_log_info, env::xml->is_new_logfile_per_hour))
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

    // config
    if (!load_config())
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

bool area_server_t::on_close_server()
{
	clear_game_logic();
	release_config();

    close_network();

    close_redis();

    close_zdb();

    close_xml();

    g_log.close();

	CMD_CLOSE;

    return true;
}

void area_server_t::on_run_server()
{
    log_info("area server running...");
}

//-------------------------------------------------------------------------------
bool area_server_t::load_xml()
{
    if (!load_global_server_xml())
    {
        printf("\033[31mload [%s] failed!\033[0m\n", GLOBAL_SERVER_XML_PATH);
        return false;
    }
    printf("load [%s] success!\n", GLOBAL_SERVER_XML_PATH);

    return true;
}
bool area_server_t::load_global_server_xml()
{
    try
    {
        boost::property_tree::ptree pt;
        boost::property_tree::xml_parser::read_xml(GLOBAL_SERVER_XML_PATH, pt);

        env::xml = new area_xml_t;

        env::xml->srv_group.start_time = pt.get<std::string>("server.start_time");
		env::xml->srv_group.group_id = pt.get<std::uint32_t>("server.server_id");

        env::xml->thread_num.network = pt.get<uint32_t>("server.area_server.thread_num.network");

        env::xml->mysql.host = pt.get<std::string>("server.area_server.mysql.host");
        env::xml->mysql.db = pt.get<std::string>("server.area_server.mysql.db");
        env::xml->mysql.user = pt.get<std::string>("server.area_server.mysql.user");
        env::xml->mysql.pwd = pt.get<std::string>("server.area_server.mysql.pwd");
        env::xml->mysql.init_conn = pt.get<uint32_t>("server.area_server.mysql.init_connections");
        env::xml->mysql.max_conn = pt.get<uint32_t>("server.area_server.mysql.max_connections");

        env::xml->redis.host = pt.get<std::string>("server.area_server.redis.host");
        env::xml->redis.port = pt.get<uint32_t>("server.area_server.redis.port");
        env::xml->redis.pwd = pt.get<std::string>("server.area_server.redis.pwd");
        env::xml->redis.db = pt.get<uint32_t>("server.area_server.redis.db");
        env::xml->redis.save_interval = pt.get<uint32_t>("server.area_server.redis.save_interval");
        env::xml->redis.save_count_limit = pt.get<uint32_t>("server.area_server.redis.save_count_limit");

        std::string str_ip_white_list = pt.get<std::string>("server.common.ip_white_list");
        string_util_t::split(str_ip_white_list, env::xml->ip_white_list, ";");
        if (env::xml->ip_white_list.empty())
        {
            printf("\033[31mip_white_list is empty!\033[0m\n");
            return false;
        }

        env::xml->log_path = pt.get<std::string>("server.common.log_path");
        env::xml->is_log_trace = pt.get<bool>("server.common.log_trace");
        env::xml->is_log_debug = pt.get<bool>("server.common.log_debug");
		env::xml->is_log_info = pt.get<bool>("server.common.log_info");
		env::xml->is_log_msg_count = pt.get<bool>("server.common.log_msg_count");
		env::xml->is_new_logfile_per_hour = pt.get<bool>("server.common.new_logfile_per_hour");
		env::xml->log_debug.worldcup = pt.get<bool>("server.common.log.worldcup");
		env::xml->log_debug.arena = pt.get<bool>("server.common.log.arena");

        env::xml->listen_at_transfer.ip = pt.get<std::string>("server.area_server.listen.transfer.ip");
        env::xml->listen_at_transfer.port = pt.get<uint32_t>("server.area_server.listen.transfer.port");
        CHECK_PORT(env::xml->listen_at_transfer.port);

        env::xml->config_path = pt.get<std::string>("server.common.path.config");

        env::xml->listen_at_cross.ip = pt.get<std::string>("server.area_server.listen.cross.ip");
        env::xml->listen_at_cross.port = pt.get<uint32_t>("server.area_server.listen.cross.port");
        CHECK_PORT(env::xml->listen_at_cross.port);
    }
    catch (boost::property_tree::ptree_error& e)
    {
        printf("\033[31mthrow error[%s]!\033[0m\n", e.what());
        return false;
    }

    if (NULL == env::xml)
    {
        printf("\033[31menv::xml is NULL!\033[0m\n");
        return false;
    }

    return true;
}
void area_server_t::close_xml()
{
    SAFE_DELETE_P(env::xml);
    log_info("close xml success!");
}

//------------------------------------------------------
bool area_server_t::init_zdb()
{
    boost::tie(m_db_url, env::conn_zdb_pool) = zdb_util_t::open_zdb(
        env::xml->mysql.host, 
        env::xml->mysql.db, 
        env::xml->mysql.user, 
        env::xml->mysql.pwd,
        env::xml->mysql.max_conn,
        env::xml->mysql.init_conn);
    if (NULL == env::conn_zdb_pool)
    {
        log_error("init zdb connection pool failed!");
        return false;
    }

    return true;
}
void area_server_t::close_zdb()
{
    zdb_util_t::close_zdb(env::conn_zdb_pool, m_db_url);
}

//------------------------------------------------------
bool area_server_t::init_redis()
{
    redis_client_t::simple_table_field_list tables;
    tables.insert(std::make_pair("arena_score_rank", ""));
    tables.insert(std::make_pair("arena_score_rank_history", "num"));

	if (g_redis_save.init(env::xml->redis.host,
		env::xml->redis.port,
		env::xml->redis.pwd.c_str(),
		env::xml->redis.db,
		env::xml->redis.save_interval,
		env::xml->redis.save_count_limit,
		env::conn_zdb_pool))
	{
		log_error("init redis_save client failed!");
		return false;
	}

    if (!redis_client_t::init(env::xml->redis.host,
        env::xml->redis.port,
        env::xml->redis.pwd.c_str(),
        env::xml->redis.db,
        env::xml->redis.save_interval,
        env::xml->redis.save_count_limit,
        env::server->get_ios(),
        env::conn_zdb_pool,
        tables))
    {
        log_error("init redis client failed!");
        return false;
    }

    return true;
}
void area_server_t::close_redis()
{
	g_redis_save.close(env::conn_zdb_pool);
    redis_client_t::close(env::conn_zdb_pool);
}

//--------------------------------------------------------------------------------------------------
bool area_server_t::init_network()
{
    if (!m_network.init(env::xml->thread_num.network, env::xml->ip_white_list))
    {
        log_error("init network failed!");
        return false;
    }

    // listen for transfer
    if (!m_network.listen_at(env::xml->listen_at_transfer.ip, env::xml->listen_at_transfer.port, &m_transfer_listener))
    {
        return false;
    }
    else
    {
        if (!transfer_msg_handler_t::init_msg_handler())
        {
            log_error("init transfer msg handler failed!");
            return false;
        }
    }

    // listen for cross
    if (!m_network.listen_at(env::xml->listen_at_cross.ip, env::xml->listen_at_cross.port, &m_cross_listener))
    {
        return false;
    }
    else
    {
        if (!cross_msg_handler_t::init_msg_handler())
        {
            log_error("init cross msg handler failed!");
            return false;
        }
    }

    return true;
}
void area_server_t::close_network()
{
	msg_count_t::write_msg_recv_count_data_file(env::xml->is_log_msg_count);
	msg_count_t::write_msg_send_count_data_file(env::xml->is_log_msg_count);

    m_network.close();
}

//--------------------------------------------------------------------------------
bool area_server_t::init_game_logic()
{
	// global id
	if (!g_id_allocator.init(env::xml->srv_group.group_id, 0, env::xml->srv_group.start_time, true))
	{
		return false;
	}
    // 活动时间管理器初始化
    if (!time_manager_t::init(env::server->get_ios(), env::xml->srv_group.start_time))
    {
        log_error("init timer failed!");
        return false;
    }
    time_manager_t::register_func(time_type_enum::time_one_day, area_server_t::one_day);
    log_info("init timer success!");

    arena_manager_t::init();
	world_cup_manager_t::init();
	hero_strategy_manager_t::init();
    return true;
}
void area_server_t::one_day(uint32_t id, bool is_in_time)
{
    if (!is_in_time)
        return;

	hero_strategy_manager_t::update_hero_strategy_data();
    arena_role_manager_t::clear_some_role();
}
void area_server_t::clear_game_logic()
{
    time_manager_t::cancel_timer();
}

//--------------------------------------------------------------------------------------------------
void area_server_t::on_add_transfer(const network::tcp_socket_ptr& s)
{
	transfer_manager_t::add_transfer(s);
}
void area_server_t::on_del_transfer(const network::tcp_socket_ptr& s)
{
	transfer_ptr p_transfer = transfer_manager_t::get_transfer(s);
	if (NULL == p_transfer)
	{
        log_error("NULL == p_transfer");
		return;
	}
	transfer_manager_t::del_transfer(s);

	log_info("delete transfer[%u]", p_transfer->get_id());
}
void area_server_t::on_transfer_msg(const network::tcp_socket_ptr& s, uint64_t uid, uint16_t cmd, const msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
	transfer_msg_handler_t::handle_server_msg(s, uid, cmd, msg_buf);
}

//--------------------------------------------------------------------------------------------------
void area_server_t::on_add_cross(const network::tcp_socket_ptr& s)
{
	cross_manager_t::add_cross(s);

	//proto::server::rs_syn_area_info_notify ntf;
	//env::server->send_msg_to_cross(s, op_cmd::rs_syn_area_info_notify, 0, ntf);
}
void area_server_t::on_del_cross(const network::tcp_socket_ptr& s)
{
	cross_manager_t::del_cross(s);
}
void area_server_t::on_cross_msg(const network::tcp_socket_ptr& s, uint64_t uid, uint16_t cmd, const msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
	cross_msg_handler_t::handle_server_msg(s, uid, cmd, msg_buf);
}

void area_server_t::post_network_send_msg(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& buf)
{
	msg_count_t::push_msg_send_info(buf, env::xml->is_log_msg_count);
	m_network.post_send_msg(s, buf);
}
