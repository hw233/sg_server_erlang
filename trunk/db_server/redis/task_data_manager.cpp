#include "task_data_manager.hpp"
#include "redis_client.hpp"
#include "config/config_manager.hpp"
#include "tblh/Task.tbls.h"

task_data_manager::task_data_manager()
{
}

task_data_manager::~task_data_manager()
{
}

bool task_data_manager::load_task_data_from_redis(uint64_t uid, proto::common::task_data* data)
{
	if (data == NULL)
	{
		log_error("load_task_data_from_redis [%lu] data == null", uid);
		return false;
	}

	//当前任务数据
	proto::common::cur_task_date cur_data;
	redis_client_t::get_protobuf("task", "cur_task_list", uid, cur_data);
	data->mutable_tasks()->CopyFrom(cur_data.tasks());
	
	//已经完成任务数据
	proto::common::comp_task_date comp_data;
	redis_client_t::get_protobuf("task", "comp_task_list", uid, comp_data);
	data->mutable_comp_task_list()->CopyFrom(comp_data.task_list());

	//任务事件
	redis_client_t::get_protobuf("task", "task_event", uid, *data->mutable_event_data());

	//试炼数据在这里
	data->set_shilian_count(redis_client_t::get_uint32("task", "shilian_count", uid));
	std::string seq_info = redis_client_t::get_string("task", "shilian_seq", uid);
	if (!seq_info.empty())
	{
		std::vector<uint32_t> seq;
		string_util_t::split<uint32_t>(seq_info, seq, "$");
		for (auto star : seq)
		{
			data->add_shilian_seq(star);
		}
	}

	//环任务数据
	std::string circle_info_str = redis_client_t::get_string("task", "circle_task_comp_list", uid);
	if (!circle_info_str.empty())
	{
		std::vector<uint32_t> seq;
		string_util_t::split<uint32_t>(circle_info_str, seq, "$");
		for (auto it: seq)
		{
			data->add_circle_task_comp_list(it);
		}
	}

	return true;
}

bool task_data_manager::save_shilian_data_to_redis(uint64_t uid, const proto::common::task_data& data)
{
	//试炼数据存放在role_ex
	redis_client_t::set_uint32("task", "shilian_count", uid, data.shilian_count());
	std::ostringstream sss;
	for (auto star : data.shilian_seq())
	{
		if (!sss.str().empty())
		{
			sss << "$";
		}

		sss << star;
	}

	redis_client_t::set_string("task", "shilian_seq", uid, sss.str());

	std::ostringstream sss2;
	for (auto id: data.circle_task_comp_list())
	{
		if (!sss2.str().empty())
		{
			sss2 << "$";
		}

		sss2 << id;
	}

	redis_client_t::set_string("task", "circle_task_comp_list", uid, sss2.str());
	return true;
}

bool task_data_manager::save_cur_task_data_to_redis(uint64_t uid, const proto::common::cur_task_date& data)
{
	redis_client_t::set_protobuf("task", "cur_task_list", uid, data);
	return true;
}

bool task_data_manager::save_comp_task_data_to_redis(uint64_t uid, const proto::common::comp_task_date& data)
{
	redis_client_t::set_protobuf("task", "comp_task_list", uid, data);
	return true;
}

bool task_data_manager::save_task_event_data_to_redis(uint64_t uid, const proto::common::task_event& data)
{
	redis_client_t::set_protobuf("task", "task_event", uid, data);
	return true;
}

