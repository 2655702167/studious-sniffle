#ifndef PAYMENT_ORDER_MODEL_H
#define PAYMENT_ORDER_MODEL_H

#include <string>
#include <inttypes.h>

/**
 * 支付订单模型：存储缴费支付的全流程信息
 */
struct PaymentOrder {
    // 1. 订单标识
    std::string order_id;       // 系统内部订单ID（格式：PAY_ORDER + 时间戳 + 随机数）
    std::string out_trade_no;   // 商户订单号（对接微信支付，需全局唯一）
    std::string user_id;        // 关联用户ID
    std::string item_id;        // 关联缴费项目ID（绑定到具体账单）

    // 2. 支付基础信息
    std::string item_type;      // 缴费类型（冗余存储，避免关联查询）
    double amount = 0.0;        // 支付金额（与缴费项目金额一致，单位：元）
    std::string pay_type;       // 支付方式（仅支持wechat，适配老年人习惯）

    // 3. 订单状态（核心字段，控制流程）
    enum class OrderStatus {
        UNPAID = 0,     // 未支付（初始状态）
        PAID = 1,       // 已支付（支付成功）
        PAY_FAILED = 2, // 支付失败（用户取消/支付超时/系统错误）
        EXPIRED = 3,    // 已过期（超过有效期未支付）
        CANCELED = 4,   // 已取消（用户主动取消）
        UNKNOWN = 5     // 未知状态（异常 fallback）
    };
    OrderStatus status = OrderStatus::UNPAID; // 默认未支付

    // 4. 时间戳信息
    int64_t create_time = 0;    // 订单创建时间戳
    int64_t expire_time = 0;    // 订单过期时间戳（默认创建后5分钟）
    int64_t pay_time = 0;       // 支付完成时间戳（已支付时记录）
    int64_t update_time = 0;    // 订单最后更新时间戳（状态变更时）

    // 5. 支付回调关联信息
    std::string transaction_id; // 第三方支付单号（如微信支付transaction_id，冗余存储）
    std::string callback_data;  // 支付回调原始数据（用于问题排查）
};

// 辅助函数：状态枚举 ↔ 字符串转换（用于JSON序列化、数据库存储）
inline std::string OrderStatusToString(PaymentOrder::OrderStatus status) {
    switch (status) {
        case PaymentOrder::OrderStatus::UNPAID: return "未支付";
        case PaymentOrder::OrderStatus::PAID: return "已支付";
        case PaymentOrder::OrderStatus::PAY_FAILED: return "支付失败";
        case PaymentOrder::OrderStatus::EXPIRED: return "已过期";
        case PaymentOrder::OrderStatus::CANCELED: return "已取消";
        default: return "未知状态";
    }
}

inline PaymentOrder::OrderStatus StringToOrderStatus(const std::string& status_str) {
    if (status_str == "未支付") return PaymentOrder::OrderStatus::UNPAID;
    if (status_str == "已支付") return PaymentOrder::OrderStatus::PAID;
    if (status_str == "支付失败") return PaymentOrder::OrderStatus::PAY_FAILED;
    if (status_str == "已过期") return PaymentOrder::OrderStatus::EXPIRED;
    if (status_str == "已取消") return PaymentOrder::OrderStatus::CANCELED;
    return PaymentOrder::OrderStatus::UNKNOWN;
}

#endif // PAYMENT_ORDER_MODEL_H