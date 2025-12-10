#ifndef VOICE_PAYMENT_SERVICE_H
#define VOICE_PAYMENT_SERVICE_H

#include "voice_recognition.h"
#include "dao/payment_dao.h"
#include "dao/user_dao.h"
#include "model/payment.h"
#include "model/payment_order.h"
#include "core/logger.h"
#include <string>
#include <vector>
#include <fmt/core.h>

/**
 * 语音支付服务层：为老年人提供语音交互式缴费体验
 */
class VoicePaymentService {
public:
    /**
     * 语音支付会话状态（用于多轮对话）
     */
    struct VoicePaymentSession {
        std::string session_id;        // 会话ID
        std::string user_id;           // 用户ID
        std::string payment_type;      // 待缴类型（水费/电费等）
        std::string item_id;           // 缴费项目ID
        double amount;                 // 金额
        std::string status;            // 会话状态（waiting_type/waiting_confirm/completed）
        int64_t create_time;           // 创建时间
        int64_t expire_time;           // 过期时间（3分钟）
    };

    /**
     * 语音支付响应结构体
     */
    struct VoicePaymentResponse {
        bool success;                  // 操作是否成功
        std::string reply_text;        // 语音回复文本（供TTS播报）
        std::string session_id;        // 会话ID（多轮对话用）
        std::string next_action;       // 下一步操作（continue/complete/error）
        PaymentOrderDTO payment_order; // 支付订单（支付成功时返回）
    };

    /**
     * 语音支付主流程（处理用户语音输入）
     * @param user_id 用户ID
     * @param audio_data 音频数据（base64编码）
     * @param session_id 会话ID（首次调用传空，多轮对话传上次返回的session_id）
     * @return 语音支付响应
     */
    static VoicePaymentResponse ProcessVoicePayment(
        const std::string& user_id,
        const std::string& audio_data,
        const std::string& session_id = "") {
        
        VoicePaymentResponse response;
        response.success = false;
        response.next_action = "error";

        try {
            // 1. 语音识别（转文字）
            auto recognition_result = VoiceRecognition::SpeechToText(audio_data);
            if (!recognition_result.success) {
                response.reply_text = "抱歉，没有听清您说的话，请再说一遍";
                return response;
            }

            std::string text = recognition_result.text;
            SPDLOG_INFO("用户语音识别结果: user_id={}, text={}", user_id, text);

            // 2. 提取支付意图
            auto intent = VoiceRecognition::ExtractPaymentIntent(text);

            // 3. 根据会话状态处理
            if (session_id.empty()) {
                // 首次交互：创建新会话
                return HandleNewPaymentSession(user_id, intent);
            } else {
                // 多轮对话：处理确认/取消等操作
                return HandleExistingPaymentSession(user_id, session_id, intent);
            }

        } catch (const std::exception& e) {
            SPDLOG_ERROR("语音支付处理异常: user_id={}, error={}", user_id, e.what());
            response.reply_text = "抱歉，系统出现了一点问题，请稍后再试";
            return response;
        }
    }

    /**
     * 语音查询待缴费项目（主动询问）
     * @param user_id 用户ID
     * @return 语音响应文本
     */
    static std::string QueryUnpaidItemsByVoice(const std::string& user_id) {
        try {
            // 查询用户所有待缴费项目
            std::vector<PaymentItem> unpaid_items = PaymentDao::QueryUserAllUnpaidItems(user_id);
            
            if (unpaid_items.empty()) {
                return "您当前没有待缴费用，真棒！";
            }

            // 构造语音播报文本
            std::string reply_text = "您有以下待缴费用：";
            int count = 0;
            for (const auto& item : unpaid_items) {
                count++;
                reply_text += fmt::format("{}. {}，金额{}元；",
                    count, item.item_type, item.amount);
                
                // 最多播报3条，避免过长
                if (count >= 3) {
                    if (unpaid_items.size() > 3) {
                        reply_text += fmt::format("还有{}条待缴费。", unpaid_items.size() - 3);
                    }
                    break;
                }
            }
            reply_text += "请问您要缴哪一项？";
            
            return reply_text;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("语音查询待缴费项目失败: user_id={}, error={}", user_id, e.what());
            return "抱歉，查询失败，请稍后再试";
        }
    }

private:
    /**
     * 处理新的支付会话（首次交互）
     */
    static VoicePaymentResponse HandleNewPaymentSession(
        const std::string& user_id,
        const VoiceRecognition::PaymentIntent& intent) {
        
        VoicePaymentResponse response;
        response.success = false;

        // 1. 判断是否为支付意图
        if (!intent.is_payment) {
            response.reply_text = "我可以帮您缴纳水费、电费、网费、话费。请问您要缴哪一项？";
            response.next_action = "continue";
            return response;
        }

        // 2. 查询用户待缴费项目
        std::vector<PaymentItem> unpaid_items = PaymentDao::QueryUserAllUnpaidItems(user_id);
        if (unpaid_items.empty()) {
            response.reply_text = "您当前没有待缴费用";
            response.next_action = "complete";
            return response;
        }

        // 3. 匹配缴费类型
        PaymentItem matched_item;
        bool found = false;

        if (!intent.payment_type.empty()) {
            // 用户指定了缴费类型（如"水费"）
            for (const auto& item : unpaid_items) {
                if (item.item_type == intent.payment_type) {
                    matched_item = item;
                    found = true;
                    break;
                }
            }
        } else {
            // 用户未指定类型，提示选择
            std::string item_list;
            int count = 0;
            for (const auto& item : unpaid_items) {
                count++;
                item_list += fmt::format("{}. {}，金额{}元；", count, item.item_type, item.amount);
            }
            response.reply_text = fmt::format("您有{}项待缴费用：{}请说出要缴纳的费用类型", 
                unpaid_items.size(), item_list);
            response.next_action = "continue";
            return response;
        }

        if (!found) {
            response.reply_text = fmt::format("没有找到{}的待缴费用。您当前待缴：{}",
                intent.payment_type, GetUnpaidItemsSummary(unpaid_items));
            response.next_action = "continue";
            return response;
        }

        // 4. 创建支付会话（等待用户确认）
        VoicePaymentSession session;
        session.session_id = GenerateSessionId();
        session.user_id = user_id;
        session.payment_type = matched_item.item_type;
        session.item_id = matched_item.item_id;
        session.amount = matched_item.amount;
        session.status = "waiting_confirm";
        session.create_time = TimeUtil::GetCurrentTimestamp();
        session.expire_time = session.create_time + 180; // 3分钟有效期

        // 保存会话（简化实现：实际需存储到缓存或数据库）
        SaveSession(session);

        // 5. 返回确认提示
        response.success = true;
        response.reply_text = fmt::format("您要缴纳{}，金额{}元。请说"确认"继续支付，或说"取消"放弃",
            matched_item.item_type, matched_item.amount);
        response.session_id = session.session_id;
        response.next_action = "continue";

        return response;
    }

    /**
     * 处理现有支付会话（多轮对话）
     */
    static VoicePaymentResponse HandleExistingPaymentSession(
        const std::string& user_id,
        const std::string& session_id,
        const VoiceRecognition::PaymentIntent& intent) {
        
        VoicePaymentResponse response;
        response.success = false;

        // 1. 查询会话
        VoicePaymentSession session = GetSession(session_id);
        if (session.session_id.empty() || session.user_id != user_id) {
            response.reply_text = "会话已过期，请重新发起支付";
            response.next_action = "complete";
            return response;
        }

        // 2. 检查会话状态
        if (session.status != "waiting_confirm") {
            response.reply_text = "当前会话状态异常，请重新发起";
            response.next_action = "complete";
            return response;
        }

        // 3. 判断用户意图
        if (intent.is_confirm) {
            // 用户确认支付
            try {
                // 调用支付接口
                PaymentOrderDTO order = PaymentService::CreatePaymentOrder(
                    user_id, session.item_id, "wechat");
                
                // 更新会话状态
                session.status = "completed";
                UpdateSession(session);

                response.success = true;
                response.reply_text = fmt::format("已为您发起{}支付，金额{}元，请在微信中完成支付",
                    session.payment_type, session.amount);
                response.payment_order = order;
                response.next_action = "complete";
                
                SPDLOG_INFO("语音支付成功: user_id={}, item_type={}, amount={}", 
                    user_id, session.payment_type, session.amount);

            } catch (const std::exception& e) {
                response.reply_text = fmt::format("支付发起失败：{}，请稍后重试", e.what());
                response.next_action = "error";
            }
        } else {
            // 用户取消或其他意图
            session.status = "canceled";
            UpdateSession(session);

            response.success = true;
            response.reply_text = "已取消支付。如需帮助，请随时对我说话";
            response.next_action = "complete";
        }

        return response;
    }

    /**
     * 辅助函数：获取待缴费项目摘要
     */
    static std::string GetUnpaidItemsSummary(const std::vector<PaymentItem>& items) {
        std::string summary;
        int count = 0;
        for (const auto& item : items) {
            count++;
            summary += fmt::format("{}（{}元）", item.item_type, item.amount);
            if (count < items.size()) {
                summary += "、";
            }
        }
        return summary;
    }

    /**
     * 辅助函数：生成会话ID
     */
    static std::string GenerateSessionId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(100000, 999999);
        return fmt::format("VOICE_PAY_SESSION{}{}", 
            TimeUtil::GetCurrentTimestamp(), dis(gen));
    }

    /**
     * 辅助函数：保存会话（简化实现，实际需用Redis等缓存）
     */
    static void SaveSession(const VoicePaymentSession& session) {
        // 实际环境需存储到Redis或内存缓存
        // RedisClient::Set(session.session_id, JsonUtil::Serialize(session), 180);
        SPDLOG_INFO("保存语音支付会话: session_id={}, user_id={}", 
            session.session_id, session.user_id);
    }

    /**
     * 辅助函数：获取会话
     */
    static VoicePaymentSession GetSession(const std::string& session_id) {
        // 实际环境需从Redis读取
        // std::string data = RedisClient::Get(session_id);
        // return JsonUtil::Deserialize<VoicePaymentSession>(data);
        
        // 简化实现：返回空会话
        VoicePaymentSession session;
        session.session_id = "";
        return session;
    }

    /**
     * 辅助函数：更新会话
     */
    static void UpdateSession(const VoicePaymentSession& session) {
        // 实际环境需更新Redis
        // RedisClient::Set(session.session_id, JsonUtil::Serialize(session), 180);
        SPDLOG_INFO("更新语音支付会话: session_id={}, status={}", 
            session.session_id, session.status);
    }
};

#endif // VOICE_PAYMENT_SERVICE_H
