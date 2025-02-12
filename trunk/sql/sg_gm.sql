CREATE DATABASE IF NOT EXISTS `sg_gm` DEFAULT CHARSET utf8 COLLATE utf8_general_ci;

USE `sg_gm`;

CREATE TABLE IF NOT EXISTS `cdkey` (
    `channel_id` int(10) NOT NULL DEFAULT '0' COMMENT '渠道id',
    `key_code` varchar(128) NOT NULL DEFAULT '0' COMMENT 'cdkey',
    `key_id` int(10) NOT NULL DEFAULT '0' COMMENT 'cdkey_id',
    `key_type` int(10) NOT NULL DEFAULT '0' COMMENT 'cdkey类型',
    `use_count` int(10) NOT NULL DEFAULT '0' COMMENT '已使用次数',
    `max_use_count` int(10) NOT NULL DEFAULT '0' COMMENT '可使用次数',
    `limit_time` int(10) NOT NULL DEFAULT '0' COMMENT '使用结束时间',
	 PRIMARY KEY (`key_code`)
)
COMMENT='cdkey兑换表' ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE='utf8_general_ci';

CREATE TABLE IF NOT EXISTS `use_cdkey` (
    `user_id` bigint(20) NOT NULL DEFAULT '0' COMMENT '玩家uid',
    `key_code` varchar(128) NOT NULL DEFAULT '0' COMMENT 'cdkey',
    `key_id` int(10) NOT NULL DEFAULT '0' COMMENT 'cdkey_id',
    `key_type` int(10) NOT NULL DEFAULT '0' COMMENT 'cdkey类型',
    `use_time` int(10) NOT NULL DEFAULT '0' COMMENT '使用时间',
	 PRIMARY KEY (`user_id`, `key_code`)
)
COMMENT='玩家兑换表' ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE='utf8_general_ci';

CREATE TABLE IF NOT EXISTS `gm_server_list` (
    `server_id` int(10) NOT NULL DEFAULT '0' COMMENT '服务器ID',
    `server_name` varchar(64) NOT NULL DEFAULT '0' COMMENT '服务器名称',
    `ip` varchar(64) NOT NULL DEFAULT '0' COMMENT '服务器ip',
    `port` int(10) NOT NULL DEFAULT '0' COMMENT '端口',
	 PRIMARY KEY (`server_id`)
)
COMMENT='gm服务器列表' ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE='utf8_general_ci';

CREATE TABLE IF NOT EXISTS `manage_role_log` (
    `id` bigint(20) NOT NULL DEFAULT '0' COMMENT '玩家id',
    `manage_type` int(10) NOT NULL DEFAULT '0' COMMENT '操作类型0为封号1为解封2为禁言3为解禁',
    `offset_time` int(10) NOT NULL DEFAULT '0' COMMENT '封号（禁言）时间',
    `oper_time` int(10) NOT NULL DEFAULT '0' COMMENT '操作时间',
	`server_id` int(10) NOT NULL DEFAULT '0' COMMENT '服务器ID',
	`reason` varchar(128) NOT NULL DEFAULT '' COMMENT '操作理由',
	`expired_time` int(11) NOT NULL DEFAULT '0' COMMENT '到期时间',
	`oper_name` varchar(20) NOT NULL DEFAULT '' COMMENT '操作人'
)
COMMENT='玩家封号禁言表' ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE='utf8_general_ci';

CREATE TABLE IF NOT EXISTS `send_mail_log` (
    `id` bigint(20) NOT NULL DEFAULT '0' COMMENT '玩家id',
    `mail_type` int(10) NOT NULL DEFAULT '0' COMMENT '邮件类型',
    `title` varchar(32) NOT NULL DEFAULT '' COMMENT '标题',
    `content` varchar(512) NOT NULL DEFAULT '' COMMENT '内容',
    `send_name` varchar(20) NOT NULL DEFAULT '' COMMENT '发送者',
    `send_time` int(10) NOT NULL DEFAULT '0' COMMENT '发送时间',
    `end_time` int(10) NOT NULL DEFAULT '0' COMMENT '结束时间',
    `on_time` int(10) NOT NULL DEFAULT '0' COMMENT '定时',
    `items` varchar(128) NOT NULL DEFAULT '' COMMENT '道具列表',
    `limit_level` int(10) NOT NULL DEFAULT '0' COMMENT '限制等级',
    `limit_cond` int(10) NOT NULL DEFAULT '0' COMMENT '限制条件',
	`server_id` int(10) NOT NULL DEFAULT '0' COMMENT '服务器ID',
	`oper_time` int(10) NOT NULL DEFAULT '0' COMMENT '操作时间',
	`oper_name` varchar(20) NOT NULL DEFAULT '' COMMENT '操作人'
)
COMMENT='发送邮件记录表' ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE='utf8_general_ci';

CREATE TABLE IF NOT EXISTS `send_notice_log` (
    `notice_type` int(10) NOT NULL DEFAULT '0' COMMENT '公告类型',
    `tick` int(10) NOT NULL DEFAULT '0' COMMENT '间隔',
    `start_time` int(10) NOT NULL DEFAULT '0' COMMENT '开始时间',
    `end_time` int(10) NOT NULL DEFAULT '0' COMMENT '结束时间',
    `notice` varchar(512) NOT NULL DEFAULT '' COMMENT '公告内容',
    `oper_time` int(10) NOT NULL DEFAULT '0' COMMENT '操作时间',
	`server_id` int(10) NOT NULL DEFAULT '0' COMMENT '服务器ID',
	`oper_name` varchar(20) NOT NULL DEFAULT '' COMMENT '操作人'
)
COMMENT='发送公告记录表' ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE='utf8_general_ci';

CREATE TABLE IF NOT EXISTS `server_ssh` (
    `server_id` int(10) NOT NULL DEFAULT '0' COMMENT '服务器ID',
    `ip` varchar(256) NOT NULL DEFAULT '0' COMMENT 'sship',
    `username` varchar(256) NOT NULL DEFAULT '0' COMMENT 'ssh用户名',
    `location` varchar(256) NOT NULL DEFAULT '0' COMMENT '服务器位置',
    `ssh_port` int(10) NOT NULL DEFAULT '0' COMMENT 'sshport',
    `passwd` varchar(256) NOT NULL DEFAULT '0' COMMENT 'ssh密码'
)
COMMENT='服务器ssh信息' ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE='utf8_general_ci';

CREATE TABLE IF NOT EXISTS `server_action` (
    `server_id` int(10) NOT NULL DEFAULT '0' COMMENT '服务器ID',
    `last_server_action` int(10) NOT NULL DEFAULT '0' COMMENT '上一次操作的动作',
    `type` varchar(256) NOT NULL DEFAULT ''COMMENT '操作类型'
)
COMMENT='对服务器进行的最近一次操作' ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE='utf8_general_ci';

CREATE TABLE IF NOT EXISTS `server_group` (
    `group_name` varchar(256) NOT NULL DEFAULT '0' COMMENT '组名',
    `group_start_id` int(10) NOT NULL DEFAULT '0' COMMENT '起始ID',
    `group_end_id` int(10) NOT NULL DEFAULT '0' COMMENT '终止ID'
)
COMMENT='服务器组ID' ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE='utf8_general_ci';

CREATE TABLE IF NOT EXISTS `server_operation_log` (
    `server_id` int(10) NOT NULL DEFAULT '0' COMMENT '服务器ID',
    `operation_type` varchar(256) NOT NULL DEFAULT '0' COMMENT '操作类型',
    `operation_user` varchar(256) NOT NULL DEFAULT '0' COMMENT '操作用户',
    `extra_data` text COMMENT '额外数据',
    `operation_date` datetime(0) COMMENT '操作时间'
)
COMMENT='服务器操作日志' ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE='utf8_general_ci';
