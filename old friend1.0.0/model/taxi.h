#ifndef TAXI_COMMON_ADDRESS_MODEL_H
#define TAXI_COMMON_ADDRESS_MODEL_H

#include <string>
#include <inttypes.h>
#include "taxi_location.h"

/**
 * 常用地址模型：存储用户个人高频使用的地址（如家庭、医院、子女家）
 * 适配老年人“一键选择地址”下单，避免重复输入
 */
struct TaxiCommonAddress {
    // 1. 核心标识
    std::string addr_id;        // 常用地址唯一ID（格式：COMMON_ADDR + 时间戳 + 随机数，如 COMMON_ADDR1761234567890）
    std::string user_id;        // 关联用户ID（多用户隔离，确保地址属于当前用户）

    // 2. 地址基础信息（继承位置模型核心字段，兼容经纬度和详细地址）
    TaxiLocation location;      // 地理位置（经纬度 + 详细地址）
    std::string display_name;   // 显示名称（老年人易识别，如 "我家"、"上海市第一人民医院"、"儿子家"）

    // 3. 分类与优先级（便于快速筛选）
    enum class AddressTag {
        HOME = 0,        // 家庭（优先级最高）
        HOSPITAL = 1,    // 医院（老年人高频场景）
        RELATIVE = 2,    // 亲属家
        SHOPPING = 3,    // 购物场所
        OTHER = 4        // 其他
    };
    AddressTag tag = AddressTag::OTHER; // 默认其他
    int priority = 5;           // 优先级（1-5，1最高，用于排序，默认5）
    bool is_default = false;    // 是否默认地址（下单时优先推荐）

    // 4. 老年友好补充信息（提升实用性）
    std::string building_info;   // 楼栋信息（如 "3号楼2单元501"，便于司机精准接送）
    std::string note;            // 备注（如 "小区北门接送"、"无障碍通道入口"）

    // 5. 时间戳信息（地址管理用）
    int64_t create_time = 0;    // 创建时间戳
    int64_t update_time = 0;    // 最后修改时间戳
    int64_t last_use_time = 0;  // 最后使用时间戳（用于热度排序）
};

// 辅助函数：地址标签 ↔ 字符串转换（前端显示/数据库存储）
inline std::string AddressTagToString(TaxiCommonAddress::AddressTag tag) {
    switch (tag) {
        case TaxiCommonAddress::AddressTag::HOME: return "家";
        case TaxiCommonAddress::AddressTag::HOSPITAL: return "医院";
        case TaxiCommonAddress::AddressTag::RELATIVE: return "亲属家";
        case TaxiCommonAddress::AddressTag::SHOPPING: return "超市";
        case TaxiCommonAddress::AddressTag::OTHER: return "其他";
        default: return "其他";
    }
}

inline TaxiCommonAddress::AddressTag StringToAddressTag(const std::string& tag_str) {
    if (tag_str == "家") return TaxiCommonAddress::AddressTag::HOME;
    if (tag_str == "医院") return TaxiCommonAddress::AddressTag::HOSPITAL;
    if (tag_str == "亲属家") return TaxiCommonAddress::AddressTag::RELATIVE;
    if (tag_str == "超市") return TaxiCommonAddress::AddressTag::SHOPPING;
    return TaxiCommonAddress::AddressTag::OTHER;
}

#endif // TAXI_COMMON_ADDRESS_MODEL_H