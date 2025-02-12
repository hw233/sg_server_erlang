#pragma once
#include "common/DataManager.h"
#include "common/Entry.h"
#include "common/macros.hpp"

/// @brief    表格结构:RankTypeBase
/// @remark   记录大小:24 X32
///           记录大小:24 X64
///           字段数量:6
///           数据文件:p排行榜.xlsx 排行榜类型
///           排序主键:类型ID
#pragma pack(1)
struct RankTypeBase
{
#ifdef _PLATFORM_X64
	static const int          FormatCheckCode = 0x010DDD89; ///< I|IIIII
#else
	static const int          FormatCheckCode = 0x010DDD89; ///< I|IIIII
#endif

	//------------------------
	uint32_t getUniqueID() const { return id; }; // TODO: 请用 类型ID 生成唯一键

	uint32_t                  id;                           ///< 类型ID

	uint32_t                  client_id;                    ///< ID
	uint32_t                  min_value;                    ///< 上榜条件
	uint32_t                  count;                        ///< 显示名次
	uint32_t                  type;                         ///< 排行榜类型
	uint32_t                  reward_mail;                  ///< 奖励邮件ID
};
#pragma pack()

struct RankType : public Entry
{
	uint32_t                  id()                          { return t_id; }

	uint32_t                  client_id()                   { return t_client_id; }
	uint32_t                  min_value()                   { return t_min_value; }
	uint32_t                  count()                       { return t_count; }
	uint32_t                  type()                        { return t_type; }
	uint32_t                  reward_mail()                 { return t_reward_mail; }

private:
	uint32_t                  t_id;                         ///< 类型ID

	uint32_t                  t_client_id;                  ///< ID
	uint32_t                  t_min_value;                  ///< 上榜条件
	uint32_t                  t_count;                      ///< 显示名次
	uint32_t                  t_type;                       ///< 排行榜类型
	uint32_t                  t_reward_mail;                ///< 奖励邮件ID

public:
	void reset();
	void fill( const RankTypeBase& base );
	const char* getClassName() const { return "p排行榜"; }

	static const std::string getBinaryName();
};

typedef DataTableManager< RankType, RankTypeBase > RankTypeManager;

inline void RankType::reset()
{
	this->t_id = 0;
	this->t_client_id = 0;
	this->t_min_value = 0;
	this->t_count = 0;
	this->t_type = 0;
	this->t_reward_mail = 0;
};

inline void RankType::fill( const RankTypeBase& base )
{
	this->reset();
	this->m_id = base.getUniqueID();

	this->t_id = base.id;
	this->t_client_id = base.client_id;
	this->t_min_value = base.min_value;
	this->t_count = base.count;
	this->t_type = base.type;
	this->t_reward_mail = base.reward_mail;
};

inline const std::string RankType::getBinaryName()
{
#ifdef _PLATFORM_X64
	return "RankType.tbls64";
#else
	return "RankType.tbls32";
#endif
};
