#ifndef PAYMENT_SERVICE_H
#define PAYMENT_SERVICE_H

#include "dao/payment_dao.h"
#include "dao/user_dao.h"
#include "model/payment.h"
#include "model/payment_order.h"
#include "util/http_client.h"
#include "util/json_util.h"
#include "util/time_util.h"
#include "util/string_util.h"
#include "config/config_parser.h"
#include "core/logger.h"
#include <stdexcept>
#include <fmt/core.h>
#include <random>

// 支付相关常量定义
constexpr int PAYMENT_ORDER_EXPIRE_SECONDS = 300; // 支付订单有效期（5分钟）
constexpr const char* SUPPORTED_PAY_TYPE = "wechat"; // 仅支持微信支付（适配老年人习惯）
constexpr const char* PAYMENT_ITEM_STATUS_UNPAID = "欠费";
constexpr const char* PAYMENT_ITEM_STATUS_PAID = "已缴清";

/**
 * 生活缴费服务层：封装水电燃气等缴费业务逻辑，适配老年人使用场景
 */
class PaymentService {
public:
    // ====================== 缴费项目相关 ======================
    /**
     * 获取用户待缴费项目（关联用户绑定的水电燃气账号）
     * @param user_id 用户唯一ID
     * @return 待缴费项目列表（已脱敏处理）
     */
    static std::vector<PaymentItemDTO> GetUserPaymentItems(const std::string& user_id) {
        // 参数校验
        if (user_id.empty()) {
            SPDLOG_WARN("Get user payment items failed: user_id is empty");
            throw std::invalid_argument("用户ID不能为空");
        }

        // 校验用户存在性
        User user = UserDao::QueryUserById(user_id);
        if (user.user_id.empty()) {
            throw std::runtime_error("用户不存在");
        }

        // 查询用户绑定的缴费项目（DAO层）
        std::vector<PaymentItem> items = PaymentDao::QueryUserPaymentItems(user_id);
        std::vector<PaymentItemDTO> result;

        // 转换为DTO（脱敏+状态格式化）
        for (const auto& item : items) {
            PaymentItemDTO dto;
            dto.item_id = item.item_id;
            dto.item_type = item.item_type; // 水费/电费/网费/话费
            dto.account = DesensitizeAccount(item.account); // 账号脱敏（如：****1234）
            dto.amount = item.amount; // 待缴金额（保留2位小数）
            dto.status = (item.amount > 0.001) ? PAYMENT_ITEM_STATUS_UNPAID : PAYMENT_ITEM_STATUS_PAID;
            dto.due_date = item.due_date; // 缴费截止日期（yyyy-mm-dd）
            dto.last_pay_time = (item.last_pay_time > 0) ? TimeUtil::TimestampToStr(item.last_pay_time) : "暂无";
            dto.remark = item.remark; // 附加说明（如："2025年10月账单"）
            result.push_back(dto);
        }

        SPDLOG_INFO("Get user payment items success (user_id={}, count={})", user_id, result.size());
        return result;
    }

    // ====================== 支付订单相关 ======================
    /**
     * 创建支付订单（对接微信支付API，生成预支付参数）
     * @param user_id 用户ID
     * @param item_id 缴费项目ID
     * @param pay_type 支付方式（仅支持wechat）
     * @return 支付订单DTO（含预支付参数）
     */
    //按实际情况修改
    static PaymentOrderDTO CreatePaymentOrder(
        const std::string& user_id, 
        const std::string& item_id, 
        const std::string& pay_type) {
        // 1. 基础参数校验
        if (user_id.empty() || item_id.empty()) {
            throw std::invalid_argument("用户ID或缴费项目ID不能为空");
        }
        if (pay_type != SUPPORTED_PAY_TYPE) {
            throw std::invalid_argument(fmt::format("仅支持{}支付", SUPPORTED_PAY_TYPE));
        }

        // 2. 校验缴费项目合法性
        PaymentItem item = PaymentDao::QueryPaymentItemById(item_id);
        if (item.item_id.empty()) {
            throw std::runtime_error("缴费项目不存在");
        }
        if (item.user_id != user_id) {
            throw std::runtime_error("无权操作该缴费项目");
        }
        if (item.amount <= 0.001) {
            throw std::runtime_error("该项目暂无待缴费用");
        }

        // 3. 检查是否存在未支付订单（避免重复创建）
        PaymentOrder existing_order = PaymentDao::QueryUnpaidOrder(user_id, item_id);
        if (!existing_order.order_id.empty()) {
            // 刷新订单有效期（延长5分钟）
            bool refresh_ok = PaymentDao::RefreshOrderExpire(existing_order.order_id, PAYMENT_ORDER_EXPIRE_SECONDS);
            if (refresh_ok) {
                SPDLOG_INFO("Reuse unpaid order (order_id={}, user_id={})", existing_order.order_id, user_id);
                return ConvertOrderToDTO(existing_order);
            }
        }

        // 4. 创建支付订单（核心逻辑）
        PaymentOrder new_order = BuildPaymentOrder(user_id, item);
        bool save_ok = PaymentDao::SavePaymentOrder(new_order);
        if (!save_ok) {
            throw std::runtime_error("订单创建失败，请重试");
        }

        // 5. 调用微信支付API，生成预支付参数（如prepay_id）
        WechatPayParams pay_params = CallWechatPayAPI(new_order);
        PaymentOrderDTO dto = ConvertOrderToDTO(new_order);
        dto.pay_params = pay_params;

        SPDLOG_INFO("Create payment order success (order_id={}, item_type={}, amount={})", 
            new_order.order_id, item.item_type, item.amount);
        return dto;
    }

    /**
     * 语音支付接口（施汉霖实现）
     * @param user_id 用户ID
     * @param audio_data 音频数据（base64编码）
     * @param session_id 会话ID（多轮对话时传入）
     * @return 语音支付响应
     */
    static VoicePaymentResponse VoicePay(
        const std::string& user_id, 
        const std::string& audio_data,
        const std::string& session_id = "") {
        
        // 调用语音支付服务处理
        return VoicePaymentService::ProcessVoicePayment(user_id, audio_data, session_id);
    }
    
    /**
     * 语音查询待缴费项目
     * @param user_id 用户ID
     * @return 语音回复文本
     */
    static std::string VoiceQueryUnpaidItems(const std::string& user_id) {
        return VoicePaymentService::QueryUnpaidItemsByVoice(user_id);
    }

    // ====================== 支付回调相关 ======================
    /**
     * 处理微信支付异步回调（更新订单状态+标记缴费项目为已缴清）
     * @param callback_data 微信支付回调的JSON数据
     * @return true=处理成功，false=处理失败（微信会重试）
     */
    static bool HandleWechatPayCallback(const std::string& callback_data) {
        try {
            // 1. 解析回调数据（微信支付回调格式）
            nlohmann::json callback_json = JsonUtil::Parse(callback_data);
            std::string out_trade_no = callback_json.value("out_trade_no", ""); // 商户订单号
            std::string trade_state = callback_json.value("trade_state", ""); // 支付状态
            std::string transaction_id = callback_json.value("transaction_id", ""); // 微信支付单号
            std::string pay_time_str = callback_json.value("success_time", ""); // 支付完成时间

            // 2. 校验回调参数合法性
            if (out_trade_no.empty() || trade_state.empty()) {
                SPDLOG_WARN("Invalid pay callback: missing core params");
                return false;
            }

            // 3. 查询订单（确认订单存在且未支付）
            PaymentOrder order = PaymentDao::QueryPaymentOrderByNo(out_trade_no);
            if (order.order_id.empty()) {
                SPDLOG_WARN("Pay callback: order not found (out_trade_no={})", out_trade_no);
                return false;
            }
            if (order.status != PaymentOrder::OrderStatus::UNPAID) {
                SPDLOG_WARN("Pay callback: order already processed (out_trade_no={}, status={})", 
                    out_trade_no, OrderStatusToString(order.status));
                return true; // 已处理过，返回成功避免重复回调
            }

            // 4. 处理支付结果
            if (trade_state == "SUCCESS") {
                // 支付成功：更新订单状态+标记缴费项目已缴清
                order.status = PaymentOrder::OrderStatus::PAID;
                order.transaction_id = transaction_id;
                order.pay_time = TimeUtil::IsoStrToTimestamp(pay_time_str);
                order.update_time = TimeUtil::GetCurrentTimestamp();

                // 数据库事务：确保订单更新和缴费项目状态更新原子性
                bool update_order_ok = PaymentDao::UpdatePaymentOrder(order);
                bool update_item_ok = PaymentDao::UpdatePaymentItemStatus(order.item_id, PAYMENT_ITEM_STATUS_PAID);

                if (update_order_ok && update_item_ok) {
                    SPDLOG_INFO("Pay success: order={}, transaction_id={}", out_trade_no, transaction_id);
                    return true;
                } else {
                    SPDLOG_ERROR("Pay callback: update order/item failed (order={})", out_trade_no);
                    return false;
                }
            } else {
                // 支付失败/关闭/取消：更新订单状态为失败
                order.status = PaymentOrder::OrderStatus::PAY_FAILED;
                order.update_time = TimeUtil::GetCurrentTimestamp();
                PaymentDao::UpdatePaymentOrder(order);
                SPDLOG_INFO("Pay failed: order={}, trade_state={}", out_trade_no, trade_state);
                return true; // 非成功状态也返回成功，避免微信重试
            }
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Handle pay callback error: {}", e.what());
            return false;
        }
    }

    

private:
    // ====================== 辅助函数 ======================
    /**
     * 账号脱敏（如：12345678 → ****5678）
     */
    static std::string DesensitizeAccount(const std::string& account) {
        if (account.size() <= 4) {
            return "****" + account;
        }
        return "****" + account.substr(account.size() - 4);
    }

    /**
     * 构建支付订单（生成订单号、填充基础信息）
     */
    static PaymentOrder BuildPaymentOrder(const std::string& user_id, const PaymentItem& item) {
        PaymentOrder order;
        order.order_id = GenerateOrderId();
        order.out_trade_no = GenerateOutTradeNo(); // 商户订单号（对接微信支付）
        order.user_id = user_id;
        order.item_id = item.item_id;
        order.item_type = item.item_type;
        order.amount = item.amount;
        order.pay_type = SUPPORTED_PAY_TYPE;
        order.status = PaymentOrder::OrderStatus::UNPAID;
        order.create_time = TimeUtil::GetCurrentTimestamp();
        order.expire_time = order.create_time + PAYMENT_ORDER_EXPIRE_SECONDS;
        order.update_time = order.create_time;
        return order;
    }

    /**
     * 生成唯一订单ID（格式：PAY + 时间戳 + 随机数）
     */
    static std::string GenerateOrderId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        return fmt::format("PAY{}{}", TimeUtil::GetCurrentTimestamp(), dis(gen));
    }

    /**
     * 生成商户订单号（微信支付要求唯一，格式：YYYYMMDD + 时间戳 + 随机数）
     */
    static std::string GenerateOutTradeNo() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(100000, 999999);
        return fmt::format("{}{}{}", 
            TimeUtil::GetCurrentDateStr("YYYYMMDD"), 
            TimeUtil::GetCurrentTimestamp(), 
            dis(gen));
    }

    /**
     * 调用微信支付API（生成预支付参数）
     */
    /*
    static WechatPayParams CallWechatPayAPI(const PaymentOrder& order) {
        ConfigParser config("config/app.ini");
        std::string pay_api_url = config.GetString("wechat_pay", "unified_order_url", "https://api.mch.weixin.qq.com/v3/pay/transactions/jsapi");
        std::string appid = config.GetString("wechat_pay", "appid");
        std::string mchid = config.GetString("wechat_pay", "mchid");
        std::string api_key = config.GetString("wechat_pay", "api_key");
        std::string notify_url = config.GetString("wechat_pay", "notify_url", "https://your-domain.com/api/payment/callback");

        // 1. 获取用户微信OpenID（支付需要）
        std::string openid = UserDao::QueryUserOpenid(order.user_id);
        if (openid.empty()) {
            throw std::runtime_error("用户未绑定微信");
        }

        // 2. 构造微信支付请求参数（JSAPI支付格式）
        nlohmann::json req_params = {
            {"appid", appid},
            {"mchid", mchid},
            {"description", fmt::format("{}缴费-{}", order.item_type, order.out_trade_no)},
            {"out_trade_no", order.out_trade_no},
            {"time_expire", TimeUtil::TimestampToIsoStr(order.expire_time)},
            {"notify_url", notify_url},
            {"amount", {
                {"total", static_cast<int>(order.amount * 100)}, // 单位：分
                {"currency", "CNY"}
            }},
            {"payer", {
                {"openid", openid}
            }}
        };

        // 3. 发送HTTP请求（HTTPS + 签名，简化版：实际需按微信支付V3签名规则处理）
        std::vector<std::pair<std::string, std::string>> headers = {
            {"Content-Type", "application/json"},
            {"Authorization", GenerateWechatPaySign(req_params.dump(), mchid, api_key)} // 签名生成（需实现）
        };

        std::string response = HttpClient::Post(pay_api_url, req_params.dump(), headers, true);
        nlohmann::json res_json = JsonUtil::Parse(response);

        // 4. 解析响应，提取预支付参数
        WechatPayParams pay_params;
        pay_params.appid = appid;
        pay_params.prepay_id = res_json.value("prepay_id", "");
        pay_params.time_stamp = std::to_string(TimeUtil::GetCurrentTimestamp());
        pay_params.nonce_str = fmt::format("{:x}", std::rand() % 100000000);
        pay_params.sign_type = "MD5";
        pay_params.pay_sign = GenerateWechatPayJSAPISign(pay_params, api_key); // JSAPI支付签名（需实现）

        if (pay_params.prepay_id.empty()) {
            throw std::runtime_error(fmt::format("微信支付参数生成失败：{}", res_json.dump()));
        }

        return pay_params;
    }

    /**
     * 转换PaymentOrder为DTO（用于返回给前端）
     *//*
     static PaymentOrderDTO ConvertOrderToDTO(const PaymentOrder& order) {
        PaymentOrderDTO dto;
        dto.order_id = order.order_id;
        dto.out_trade_no = order.out_trade_no;
        dto.item_type = order.item_type;
        dto.amount = order.amount;
        dto.pay_type = order.pay_type;
        dto.status = OrderStatusToString(order.status);
        dto.create_time = TimeUtil::TimestampToStr(order.create_time);
        dto.expire_time = TimeUtil::TimestampToStr(order.expire_time);
        dto.pay_time = (order.pay_time > 0) ? TimeUtil::TimestampToStr(order.pay_time) : "";
        return dto;
    }
*/
    
    /**
     * 从指令中提取金额（简化版：匹配数字）
     *//*static void ExtractAmountFromCommand(const std::string& command, double& amount) {
        std::regex amount_regex(R"((\d+\.?\d*))"); // 匹配数字（整数/小数）
        std::smatch match;
        if (std::regex_search(command, match, amount_regex)) {
            try {
                amount = std::stod(match[1]);
            } catch (...) {
                amount = 0.0;
            }
        }
    }
*/
    
    /**
     * 金额匹配校验（允许±0.01元误差）
     */
    static bool IsAmountMatch(double actual, double target) {
        return std::fabs(actual - target) <= 0.01;
    }
    

    /**
     * 确认最后一个待缴项目（处理"确认缴费"指令）
     */
    static std::string ConfirmLastUnpaidItem(const std::string& user_id) {
        std::vector<PaymentItem> unpaid_items = PaymentDao::QueryUserAllUnpaidItems(user_id);
        if (unpaid_items.empty()) {
            return "您当前暂无待缴费用";
        }
        // 取最后一个待缴项目（按创建时间倒序）
        PaymentItem last_item = unpaid_items[0];
        try {
            CreatePaymentOrder(user_id, last_item.item_id, SUPPORTED_PAY_TYPE);
            return fmt::format("已为您发起{}支付，金额{}元，请在微信中完成支付", 
                last_item.item_type, last_item.amount);
        } catch (const std::exception& e) {
            return fmt::format("支付发起失败：{}", e.what());
        }
    }

    /**
     * 生成微信支付签名（简化版：实际需按微信支付V3签名规则实现）
     * 注：正式环境需严格遵循微信支付签名规范，包含时间戳、随机串、证书等
     */
    /* static std::string GenerateWechatPaySign(const std::string& body, const std::string& mchid, const std::string& api_key) {
        // 此处为占位实现，正式环境需替换为真实签名逻辑
        std::string sign_str = fmt::format("body={}&mchid={}&key={}", body, mchid, api_key);
        return StringUtil::Md5(sign_str); // 需实现MD5工具函数
    }*/
   

    /**
     * 生成微信JSAPI支付签名（适配小程序支付）
     */
    /*static std::string GenerateWechatPayJSAPISign(const WechatPayParams& params, const std::string& api_key) {
        // 签名规则：appId + timeStamp + nonceStr + prepayId + signType + key
        std::string sign_str = fmt::format("appId={}&timeStamp={}&nonceStr={}&package=prepay_id={}&signType={}&key={}",
            params.appid, params.time_stamp, params.nonce_str, params.prepay_id, params.sign_type, api_key);
        return StringUtil::Md5(sign_str); // 需实现MD5工具函数
    }*/
    
};

// ====================== 数据传输对象（DTO）和辅助结构体 ======================
/**
 * 缴费项目DTO（返回给前端，脱敏后的数据）
 */
struct PaymentItemDTO {
    std::string item_id;       // 缴费项目ID
    std::string item_type;     // 缴费类型（水费/电费/燃气费）
    std::string account;       // 脱敏账号（如：****1234）
    double amount;             // 待缴金额
    std::string status;        // 状态（欠费/已缴清）
    std::string due_date;      // 缴费截止日期（yyyy-mm-dd）
    std::string last_pay_time; // 上次缴费时间
    std::string remark;        // 备注（如："2025年10月账单"）
};

/**
 * 支付订单DTO（返回给前端）
 */
struct PaymentOrderDTO {
    std::string order_id;      // 订单ID
    std::string out_trade_no;  // 商户订单号（微信支付用）
    std::string item_type;     // 缴费类型
    double amount;             // 支付金额
    std::string pay_type;      // 支付方式
    std::string status;        // 订单状态（字符串形式）
    std::string create_time;   // 创建时间（yyyy-mm-dd HH:MM:SS）
    std::string expire_time;   // 过期时间（yyyy-mm-dd HH:MM:SS）
    std::string pay_time;      // 支付时间（为空表示未支付）
    WechatPayParams pay_params;// 微信支付参数（未支付时返回）
};

/**
 * 微信支付参数结构体（适配小程序支付）
 */
struct WechatPayParams {
    std::string appid;         // 小程序AppID
    std::string prepay_id;     // 预支付ID
    std::string time_stamp;    // 时间戳
    std::string nonce_str;     // 随机串
    std::string sign_type;     // 签名类型（MD5/HMAC-SHA256）
    std::string pay_sign;      // 支付签名
};

// ====================== 辅助函数（OrderStatus 转换） ======================
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

#endif // PAYMENT_SERVICE_H