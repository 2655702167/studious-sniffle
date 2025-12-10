#ifndef HOSPITAL_MODEL_H
#define HOSPITAL_MODEL_H

#include <string>
#include <vector>

/**
 * 医院模型：存储医院核心信息，适配挂号服务的查询、配额管理需求
 */
struct Hospital {
    // 1. 基础标识信息
    std::string id;                 // 医院唯一ID（推荐格式：H + 地区编码 + 序号，如 H31010001）
    std::string name;               // 医院名称（如 "上海市第一人民医院"）
    std::string level;              // 医院等级（如 "三级甲等"、"二级乙等"，老年人挂号常关注）
    std::string type;               // 医院类型（如 "综合医院"、"专科医院"、"社区医院"）
    std::string status;             // 服务状态（enabled=正常服务，disabled=暂停服务，维护中=临时关闭）

    // 2. 联系与地址信息
    std::string address;            // 详细地址（如 "上海市黄浦区武进路85号"，用于显示给用户）
    std::string phone;              // 咨询电话（老年人可能需要电话咨询，必填）
    std::string emergency_phone;    // 急诊电话（可选，提升实用性）

    // 3. 地理位置信息（用于按距离筛选医院）
    double latitude = 0.0;          // 纬度（如 31.230416）
    double longitude = 0.0;         // 经度（如 121.473701）

    // 4. 科室与预约配置（核心字段，适配挂号流程）
    std::vector<std::string> departments; // 支持的科室列表（如 ["内科", "外科", "骨科", "眼科"]）
    int daily_quota = 100;          // 每日总预约配额（默认100，可按医院规模调整）
    int available_quota = 100;      // 当日剩余预约配额（核心字段，用于判断是否可预约）
    std::vector<std::string> reserve_days; // 可预约日期范围（如 ["2025-11-25", "2025-11-30"]）
    int advance_days = 7;           // 最大预约提前天数（默认7天，符合多数医院规则）

    // 5. 附加信息（提升老年人使用体验）
    bool is_elderly_friendly = false; // 是否为老年友好医院（如提供轮椅、优先就诊等服务）
    std::string elderly_service;    // 老年专项服务（如 "老年人绿色通道"、"免费导诊"）
    std::string opening_hours;      // 门诊时间（如 "周一至周五 8:00-17:00，周六 8:00-12:00"）
};

#endif // HOSPITAL_MODEL_H