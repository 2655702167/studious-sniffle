#ifndef PAYMENT_ITEM_MODEL_H
#define PAYMENT_ITEM_MODEL_H

#include <string>
#include <inttypes.h>

/**
 * 缴费项目模型：存储用户绑定的水电燃气等缴费账号及账单信息
 */
struct PaymentItem {
    // 1. 核心标识
    std::string item_id;        // 缴费项目唯一ID（格式：PAY_ITEM + 时间戳 + 随机数，如 PAY_ITEM1761234567890123）
    std::string user_id;        // 关联用户ID（绑定到具体用户）
    std::string item_type;      // 缴费类型（固定枚举：水费/电费/网费/话费）
    std::string account;        // 缴费账号（如水电表号，明文存储但前端脱敏显示）

    // 2. 账单信息
    double amount = 0.0;        // 待缴金额（保留2位小数，单位：元）
    std::string status;         // 状态（欠费/已缴清，与服务层常量一致）
    std::string due_date;       // 缴费截止日期（格式：yyyy-mm-dd，如 "2025-12-10"）
    std::string bill_month;     // 账单所属月份（如 "2025-10"，便于按月份查询）
    std::string remark;         // 备注（如 "2025年10月水费账单"，提升老年人可读性）

    // 3. 时间戳信息
    int64_t create_time = 0;    // 项目创建时间戳（用户绑定账号时）
    int64_t update_time = 0;    // 最后更新时间戳（账单更新/缴费状态变更时）
    int64_t last_pay_time = 0;  // 上次缴费时间戳（已缴清时记录）
};

#endif // PAYMENT_ITEM_MODEL_H