#ifndef RESERVE_ORDER_MODEL_H
#define RESERVE_ORDER_MODEL_H

#include <string>
#include <inttypes.h> // 用于跨平台整数类型（如 int64_t）

/**
 * 预约订单模型：存储挂号订单的全生命周期信息，适配预约、取消、查询流程
 */
struct ReserveOrder {
    // 1. 订单标识信息
    std::string order_id;           // 订单唯一ID（推荐格式：RES + 时间戳 + 随机数，如 RES1761234567890123）
    std::string user_id;            // 预约用户ID（关联 User 模型的 user_id）
    std::string order_no;           // 医院预约单号（冗余存储，医院官方单号，便于用户线下就诊）

    // 2. 医院与科室信息（冗余存储，避免关联查询，提升响应速度）
    std::string hospital_id;        // 医院ID（关联 Hospital 模型的 id）
    std::string hospital_name;      // 医院名称（如 "上海市第一人民医院"）
    std::string department;         // 预约科室（如 "内科"）
    std::string doctor_name;        // 医生姓名（可选，若用户选择具体医生则存储）
    std::string doctor_title;       // 医生职称（如 "主任医师"、"副主任医师"，可选）

    // 3. 预约时间信息（核心字段，用户就诊凭证）
    std::string reserve_date;       // 预约日期（格式：yyyy-mm-dd，如 "2025-11-26"）
    std::string reserve_period;     // 预约时段（如 "上午 8:00-9:00"、"下午 14:30-15:30"）
    int64_t create_time = 0;        // 订单创建时间戳（秒级，如 1761234567）
    int64_t update_time = 0;        // 订单更新时间戳（如取消、完成时更新）

    // 4. 订单状态信息（核心字段，控制流程）
    enum class OrderStatus {
        PENDING = 0,    // 待就诊（已预约成功）
        CANCELED = 1,   // 已取消（用户主动取消或超时未就诊）
        COMPLETED = 2,  // 已完成（用户已就诊）
        EXPIRED = 3,    // 已过期（超时未就诊且未取消）
        INVALID = 4     // 无效订单（如医院取消预约）
    };
    OrderStatus status = OrderStatus::PENDING; // 默认待就诊

    // 5. 取消相关信息（适配取消流程和配额回滚）
    int64_t cancel_time = 0;        // 取消时间戳（未取消则为0）
    std::string cancel_reason;      // 取消原因（可选，如 "用户临时有事"、"医院停诊"）
    bool is_quota_recovered = false; // 配额是否已恢复（避免重复回滚医院配额）

    // 6. 就诊人信息（老年人挂号需实名认证，必填）
    std::string patient_name;       // 就诊人姓名（如 "张三"）
    std::string patient_id_card;    // 就诊人身份证号（脱敏存储，如 "310101********1234"）
    std::string patient_phone;      // 就诊人手机号（用于接收医院通知）

    // 7. 附加信息（提升实用性和排障效率）
    std::string note;               // 备注（如 "需轮椅服务"、"糖尿病史"，可选）
    std::string operator_id;        // 操作人ID（如管理员ID，默认空表示用户自助预约）
};

// 辅助函数：将 OrderStatus 转换为字符串（用于JSON序列化、日志输出）
inline std::string OrderStatusToString(ReserveOrder::OrderStatus status) {
    switch (status) {
        case ReserveOrder::OrderStatus::PENDING: return "待就诊";
        case ReserveOrder::OrderStatus::CANCELED: return "已取消";
        case ReserveOrder::OrderStatus::COMPLETED: return "已完成";
        case ReserveOrder::OrderStatus::EXPIRED: return "已过期";
        case ReserveOrder::OrderStatus::INVALID: return "无效订单";
        default: return "未知状态";
    }
}

// 辅助函数：将字符串转换为 OrderStatus（用于解析数据库存储、回调参数）
inline ReserveOrder::OrderStatus StringToOrderStatus(const std::string& status_str) {
    if (status_str == "待就诊") return ReserveOrder::OrderStatus::PENDING;
    if (status_str == "已取消") return ReserveOrder::OrderStatus::CANCELED;
    if (status_str == "已完成") return ReserveOrder::OrderStatus::COMPLETED;
    if (status_str == "已过期") return ReserveOrder::OrderStatus::EXPIRED;
    if (status_str == "无效订单") return ReserveOrder::OrderStatus::INVALID;
    return ReserveOrder::OrderStatus::INVALID;
}

#endif // RESERVE_ORDER_MODEL_H