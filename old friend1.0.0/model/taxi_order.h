#ifndef TAXI_ORDER_MODEL_H
#define TAXI_ORDER_MODEL_H

#include <string>
#include <inttypes.h>
#include "taxi_location.h"

/**
 * 打车订单模型：存储订单全流程信息，支持状态流转和结算
 */
struct TaxiOrder {
    // 1. 订单标识
    std::string order_id;        // 订单唯一ID（格式：TAXI_ORDER + 时间戳 + 随机数，如 TAXI_ORDER1761234567890123）
    std::string user_id;         // 下单用户ID（关联User模型）
    std::string out_trade_no;    // 支付订单号（关联Payment模块，结算用）

    // 2. 行程信息（核心字段，老年人就诊/出行凭证）
    TaxiLocation start_location; // 起点位置（经纬度+详细地址）
    TaxiLocation end_location;   // 终点位置（经纬度+详细地址）
    std::string start_time;      // 出发时间（预约单必填，格式：yyyy-mm-dd HH:MM，如 "2025-11-26 08:30"）

    // 3. 司机与车辆信息（派单后填充，冗余存储）
    std::string driver_id;       // 接单司机ID
    std::string driver_name;     // 司机姓名（脱敏，如 "张师傅"）
    std::string license_plate;   // 车牌号（脱敏，如 "沪A****12"）
    std::string driver_phone;    // 司机手机号（脱敏，便于用户联系）

    // 4. 订单状态（核心字段，控制流程）
    enum class OrderStatus {
        PENDING_DISPATCH = 0,  // 待派单（已下单未匹配司机）
        DISPATCHED = 1,        // 已派单（匹配司机，司机未接单）
        DRIVER_ACCEPTED = 2,   // 司机已接单（前往接驾）
        PICKED_UP = 3,         // 已接驾（用户上车，行程中）
        COMPLETED = 4,         // 已完成（到达目的地，已结算）
        CANCELED = 5,          // 已取消（用户/司机取消）
        EXPIRED = 6,           // 已过期（预约单未按时出行）
        FAILED = 7             // 失败（派单失败/异常）
    };
    OrderStatus status = OrderStatus::PENDING_DISPATCH; // 默认待派单

    // 5. 费用信息（结算用，适配老年人清晰对账）
    double distance = 0.0;       // 实际行驶距离（单位：km）
    int duration = 0;            // 实际行驶时长（单位：分钟）
    double base_fee = 10.0;      // 起步价（单位：元）
    double distance_fee = 0.0;   // 里程费（单位：元）
    double time_fee = 0.0;       // 时长费（单位：元）
    double extra_fee = 0.0;      // 附加费（如高速费、等候费，单位：元）
    double discount_fee = 0.0;   // 优惠金额（单位：元）
    double total_fee = 0.0;      // 总费用（单位：元，四舍五入保留2位小数）
    std::string pay_status = "未支付"; // 支付状态（未支付/已支付）


    // 6. 时间戳信息（追溯订单流转）
    int64_t create_time = 0;     // 订单创建时间戳
    int64_t dispatch_time = 0;   // 派单时间戳
    int64_t accept_time = 0;     // 司机接单时间戳
    int64_t pick_up_time = 0;    // 接驾时间戳
    int64_t complete_time = 0;   // 完成时间戳
    int64_t cancel_time = 0;     // 取消时间戳

    // 7. 取消/异常信息
    std::string cancelor = "";   // 取消人（user/driver/system）
    std::string cancel_reason = ""; // 取消原因（如 "用户临时有事"、"司机车辆故障"）
    std::string remark = "";     // 订单备注（如异常说明）
};

// 辅助函数：订单状态 ↔ 字符串转换（便于前端显示、日志输出）
inline std::string OrderStatusToString(TaxiOrder::OrderStatus status) {
    switch (status) {
        case TaxiOrder::OrderStatus::PENDING_DISPATCH: return "待派单";
        case TaxiOrder::OrderStatus::DISPATCHED: return "已派单";
        case TaxiOrder::OrderStatus::DRIVER_ACCEPTED: return "司机已接单";
        case TaxiOrder::OrderStatus::PICKED_UP: return "已接驾";
        case TaxiOrder::OrderStatus::COMPLETED: return "已完成";
        case TaxiOrder::OrderStatus::CANCELED: return "已取消";
        case TaxiOrder::OrderStatus::EXPIRED: return "已过期";
        case TaxiOrder::OrderStatus::FAILED: return "失败";
        default: return "未知状态";
    }
}

inline TaxiOrder::OrderStatus StringToOrderStatus(const std::string& status_str) {
    if (status_str == "待派单") return TaxiOrder::OrderStatus::PENDING_DISPATCH;
    if (status_str == "已派单") return TaxiOrder::OrderStatus::DISPATCHED;
    if (status_str == "司机已接单") return TaxiOrder::OrderStatus::DRIVER_ACCEPTED;
    if (status_str == "已接驾") return TaxiOrder::OrderStatus::PICKED_UP;
    if (status_str == "已完成") return TaxiOrder::OrderStatus::COMPLETED;
    if (status_str == "已取消") return TaxiOrder::OrderStatus::CANCELED;
    if (status_str == "已过期") return TaxiOrder::OrderStatus::EXPIRED;
    if (status_str == "失败") return TaxiOrder::OrderStatus::FAILED;
    return TaxiOrder::OrderStatus::PENDING_DISPATCH;
}

#endif // TAXI_ORDER_MODEL_H