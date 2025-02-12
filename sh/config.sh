#/bin/bash

#linux账户名字
user_name=$USER
user_passwd=$user_name

#项目主目录
main_path="/home/$user_name/sg_server"

. $main_path/sh/my_config.sh

#服务器组的名称 如：内网测试服[trunk]
if [ $server_name_show_version -eq 0 ];then
    server_name="$server_desc"
else
    server_name="$server_desc[$version_name]"
fi
    
#服务器分支路径名
branches_path=""
server_svn_path=""
design_svn_path=""
if [ "$version_name" = "trunk" ];then
    branches_path="trunk"
    server_svn_path="svn://192.168.1.7/card_game/trunk/sg_server/trunk" 
    design_svn_path="svn://192.168.1.7/card_game/trunk/design/Data/表格" 
else
    branches_path="branches/$version_name"
    server_svn_path="svn://192.168.1.7/card_game/$branches_path/server" 
    design_svn_path="svn://192.168.1.7/card_game/$branches_path/design"
fi

#工作目录
work_path="$main_path/$branches_path"

###########################################################################
# 以下是nohup_run/*.sh所需的参数！
###########################################################################
#使用ps命令时所需的用户名
ps_name=$(ps aux | grep 'ps aux' | grep -v 'grep' | cut -d " " -f1 | head -n 1)

#运行|关闭[单服务器组]的列表以及顺序
run_group_server_list="db log gm game center chat gate login"
stop_group_server_list="login gate game center chat db log gm"

#运行|关闭[全局服务器]的列表以及顺序(除了account和glog需要单独开启)
all_global_server_list="account glog area transfer cross"
run_global_server_list="area transfer cross"
stop_global_server_list="transfer cross area"

#存放core文件路径的数组
declare -A core_list=()
declare -A asan_list=()

if [ ! -d $main_path/logs/ ];then
    mkdir -p $main_path/logs/
fi
asan_log_dir=$main_path/logs/asan_log
core_log_dir=$main_path/logs/core_log
combat_log_dir=$main_path/logs/combat_log #这里修改的话，同时也要修改server.xml中的对应字段
combat_log_backup_dir="" #为空，表示不备份 $main_path/logs/combat_log_backup
rm_log_dir=$main_path/logs/rm_log
pm_log_dir=$main_path/logs/pm_log
valgrind_log_dir=$main_path/logs/valgrind_log
