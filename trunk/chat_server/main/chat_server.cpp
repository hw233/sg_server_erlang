#include "chat_server.hpp"
#include "common/log.hpp"
#include "game/game_msg_handler.hpp"
#include "user/user_manager.hpp"
#include "gate/gate_msg_handler.hpp"
#include "transfer/transfer_msg_handler.hpp"
#include "config/config_manager.hpp"
#include "game/send_game_msg.hpp"
#include "center/center_msg_handler.hpp"
#include "msg_count.hpp"
#include "hero_strategy/hero_strategy_manager.hpp"
#include "common/time_manager.hpp"
#include "tblh/time_type_enum.hpp"

USING_NS_NETWORK;
USING_NS_COMMON;

chat_server_t::chat_server_t()
{
}

chat_server_t::~chat_server_t()
{
}

bool chat_server_t::on_init_server()
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

    // init logic
    if (!init_logic())
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

bool chat_server_t::on_close_server()
{
	stop_network();

    {
        user_manager_t::close_clear();
		time_manager_t::cancel_timer();
        release_config();
    }

    close_network();

    close_xml();

    g_log.close();

    CMD_CLOSE;

    return true;
}

void chat_server_t::on_run_server()
{
    log_info("chat server running...");
}

//----------------------------------------------------------------------------------------------------
bool chat_server_t::load_xml()
{
    if (!load_server_xml())
    {
        printf("\033[31mload [%s] failed!\033[0m\n", SERVER_XML_PATH);
        return false;
    }
    printf("load [%s] success!\n", SERVER_XML_PATH);

    return true;
}
bool chat_server_t::load_server_xml()
{
    try
    {
        boost::property_tree::ptree pt;
        boost::property_tree::xml_parser::read_xml(SERVER_XML_PATH, pt);

        env::xml = new chat_xml_t;

        env::xml->srv_group.group_id = pt.get<uint32_t>("server.server_id");
		env::xml->srv_group.group_name = pt.get<std::string>("server.server_name");
		env::xml->srv_group.start_time = pt.get<std::string>("server.start_time");

        env::xml->thread_num.network = pt.get<uint32_t>("server.chat_server.thread_num.network");

        env::xml->listen_at_gate.ip = pt.get<std::string>("server.chat_server.listen.gate.ip");
        env::xml->listen_at_gate.port = pt.get<uint32_t>("server.chat_server.listen.gate.port");
        CHECK_PORT(env::xml->listen_at_gate.port);

        env::xml->listen_at_game.ip = pt.get<std::string>("server.chat_server.listen.game.ip");
        env::xml->listen_at_game.port = pt.get<uint32_t>("server.chat_server.listen.game.port");
        CHECK_PORT(env::xml->listen_at_game.port);

        //connect transfer
        {
            std::string str_ip_list = pt.get<std::string>("server.chat_server.connect.transfer.ip");
            std::vector<std::string> ip_list;
            string_util_t::split(str_ip_list, ip_list, ";");
            if (ip_list.empty())
            {
                printf("\033[31mconnect to transfer: ip_list is empty!\033[0m\n");
                return false;
            }

            std::string str_port_list = pt.get<std::string>("server.chat_server.connect.transfer.port");
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

		env::xml->connect_to_center.ip = pt.get<std::string>("server.center_server.listen.chat.ip");
		env::xml->connect_to_center.port = pt.get<uint32_t>("server.center_server.listen.chat.port");
		CHECK_PORT(env::xml->connect_to_center.port);

		env::xml->connect_to_video.ip = pt.get<std::string>("server.chat_server.connect.video.ip");
		env::xml->connect_to_video.port = pt.get<uint32_t>("server.chat_server.connect.video.port");
		CHECK_PORT(env::xml->connect_to_video.port);

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
        env::xml->reconnect_interval_time = pt.get<uint32_t>("server.common.reconnect_interval_time");
		env::xml->config_path = pt.get<std::string>("server.common.path.config");
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

void chat_server_t::close_xml()
{
    SAFE_DELETE_P(env::xml);
    log_info("close xml success!");
}

//---------------------------------------------------------------------------------------------
bool chat_server_t::init_logic()
{
    // 活动时间管理器初始化
    if (!time_manager_t::init(env::server->get_ios(), env::xml->srv_group.start_time))
    {
        log_error("init timer failed!");
        return false;
    }
    time_manager_t::register_func(time_type_enum::time_chat_get_arena_data, hero_strategy_manager_t::chat_get_arena_data_one_day);
    log_info("init timer success!");

    return true;
}

//----------------------------------------------------------------------------------------------------
bool chat_server_t::init_network()
{
    if (!m_network.init(env::xml->thread_num.network, env::xml->ip_white_list))
    {
        log_error("init network failed!");
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

    // connect to center
    if (!m_network.connect_to(env::xml->connect_to_center.ip, env::xml->connect_to_center.port, &m_center_connector, env::xml->reconnect_interval_time))
    {
        return false;
    }
    else
    {
        if (!center_msg_handler_t::init_msg_handler())
        {
            log_error("init center msg handler failed!");
            return false;
        }
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

	// connect to video
	if (!m_network.connect_to(env::xml->connect_to_video.ip, env::xml->connect_to_video.port, &m_video_connector, env::xml->reconnect_interval_time))
	{
		return false;
	}

    return true;
}
void chat_server_t::close_network()
{
	msg_count_t::write_msg_recv_count_data_file(env::xml->is_log_msg_count);
	msg_count_t::write_msg_send_count_data_file(env::xml->is_log_msg_count);

    gate_manager_t::clear_gate();
    transfer_manager_t::clear_transfer();

    m_network.close();
}

//---------------------------------------------------------------------------------
void chat_server_t::on_add_gate(const network::tcp_socket_ptr& s)
{
    gate_manager_t::add_gate(s);
}
void chat_server_t::on_del_gate(const network::tcp_socket_ptr& s)
{
    gate_manager_t::del_gate(s);
}
void chat_server_t::on_gate_msg(const network::tcp_socket_ptr& s, uint64_t uid, uint16_t cmd, const msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
    gate_msg_handler_t::handle_server_msg(s, uid, cmd, msg_buf);
}

//---------------------------------------------------------------------------------
void chat_server_t::on_add_game(const network::tcp_socket_ptr& s)
{
    game_manager_t::add_game(s);

	send_game_msg_t::send_get_online_user_list_request();
}
void chat_server_t::on_del_game(const network::tcp_socket_ptr& s)
{
    game_manager_t::del_game(s);
}
void chat_server_t::on_game_msg(const network::tcp_socket_ptr& s, uint64_t uid, uint16_t cmd, const msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
	if (cmd > op_cmd::HR_BEGIN && cmd < op_cmd::HR_END)
	{
		send_msg_from_game_to_transfer(cmd, uid, msg_buf);
	}
	else if (cmd > op_cmd::GV_BEGIN && cmd < op_cmd::GV_END) 
	{
		send_msg_from_game_to_video(cmd, uid, msg_buf);
	}
	else
	{
		game_msg_handler_t::handle_server_msg(s, uid, cmd, msg_buf);
	}
}

//---------------------------------------------------------------------------------
void chat_server_t::on_add_transfer(const tcp_socket_ptr& s)
{
	transfer_manager_t::add_transfer(s);
}
void chat_server_t::on_del_transfer(const tcp_socket_ptr& s)
{
	transfer_manager_t::del_transfer(s);
}
void chat_server_t::on_transfer_msg(const network::tcp_socket_ptr& s,uint64_t uid, uint16_t cmd, const network::msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
	transfer_msg_handler_t::handle_server_msg(s, uid, cmd, msg_buf);
}
//---------------------------------------------------------------------------------
void chat_server_t::on_add_center(const network::tcp_socket_ptr& s)
{
	m_center_connector.set_socket(s);
}

void chat_server_t::on_del_center(const network::tcp_socket_ptr& s)
{
	env::server->post_network_close_socket(s);
	m_center_connector.reset_socket();
}

void chat_server_t::on_center_msg(const network::tcp_socket_ptr& s, uint64_t uid, uint16_t cmd, const network::msg_buf_ptr& msg_buf)
{
	msg_count_t::push_msg_recv_info(msg_buf, cmd, env::xml->is_log_msg_count);
	center_msg_handler_t::handle_server_msg(uid, cmd, msg_buf);
}

void chat_server_t::post_network_send_msg(const network::tcp_socket_ptr& s, const network::msg_buf_ptr& buf)
{
	msg_count_t::push_msg_send_info(buf, env::xml->is_log_msg_count);
	m_network.post_send_msg(s, buf);
}

void chat_server_t::stop_network()
{
	m_network.stop();
}

void chat_server_t::on_add_video(const network::tcp_socket_ptr& s)
{
	m_video_connector.set_socket(s);
}

void chat_server_t::on_del_video(const network::tcp_socket_ptr& s)
{
	m_video_connector.close_socket();
}
