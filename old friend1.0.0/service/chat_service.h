#ifndef CHAT_SERVICE_H
#define CHAT_SERVICE_H

#include <string>
#include <vector>
#include <map>
#include <random>
#include "util/http_client.h"
#include "util/json_util.h"
#include "util/time_util.h"
#include "dao/user_dao.h"
#include "core/logger.h"
#include <fmt/core.h>

/**
 * 智能陪聊服务：基于文心一言API实现老年人情感陪伴与智能对话
 * 支持多轮对话、节日祝福、亲人音色设置等适老化功能
 */
class ChatService {
public:
    /**
     * 聊天会话结构体
     */
    struct ChatSession {
        std::string session_id;              // 会话ID
        std::string user_id;                 // 用户ID
        std::vector<ChatMessage> messages;   // 历史消息（最多保留10轮）
        std::string voice_profile;           // 音色配置（孙辈音色URL）
        int64_t create_time;                 // 创建时间
        int64_t last_active_time;            // 最后活跃时间
        int64_t expire_time;                 // 过期时间（30分钟）
    };

    /**
     * 聊天消息结构体
     */
    struct ChatMessage {
        std::string role;      // 角色：user/assistant
        std::string content;   // 消息内容
        int64_t timestamp;     // 时间戳
    };

    /**
     * 聊天响应结构体
     */
    struct ChatResponse {
        bool success;                  // 是否成功
        std::string reply_text;        // 回复文本
        std::string session_id;        // 会话ID
        std::string voice_profile;     // 建议使用的音色
        bool need_tts;                 // 是否需要TTS播报
        std::string error_message;     // 错误信息
    };

    /**
     * 主聊天接口：处理用户消息，返回AI回复
     * @param user_id 用户ID
     * @param message 用户消息内容
     * @param session_id 会话ID（首次传空，后续传上次返回的session_id）
     * @return 聊天响应
     */
    static ChatResponse Chat(
        const std::string& user_id,
        const std::string& message,
        const std::string& session_id = "") {
        
        ChatResponse response;
        response.success = false;
        response.need_tts = true;

        try {
            // 1. 获取或创建会话
            ChatSession session;
            if (session_id.empty()) {
                // 创建新会话
                session = CreateNewSession(user_id);
                // 添加主动问候
                AddGreeting(session);
            } else {
                // 获取现有会话
                session = GetSession(session_id);
                if (session.session_id.empty() || session.user_id != user_id) {
                    // 会话过期或无效，创建新会话
                    session = CreateNewSession(user_id);
                    AddGreeting(session);
                }
            }

            // 2. 添加用户消息到历史
            ChatMessage user_msg;
            user_msg.role = "user";
            user_msg.content = message;
            user_msg.timestamp = TimeUtil::GetCurrentTimestamp();
            session.messages.push_back(user_msg);

            // 3. 调用文心一言API获取回复
            std::string ai_reply = CallWenxinAPI(session);
            
            if (ai_reply.empty()) {
                response.error_message = "AI服务暂时不可用，请稍后再试";
                return response;
            }

            // 4. 添加AI回复到历史
            ChatMessage ai_msg;
            ai_msg.role = "assistant";
            ai_msg.content = ai_reply;
            ai_msg.timestamp = TimeUtil::GetCurrentTimestamp();
            session.messages.push_back(ai_msg);

            // 5. 限制历史消息数量（最多10轮）
            if (session.messages.size() > 20) {
                session.messages.erase(session.messages.begin(), session.messages.begin() + 2);
            }

            // 6. 更新会话
            session.last_active_time = TimeUtil::GetCurrentTimestamp();
            SaveSession(session);

            // 7. 构造响应
            response.success = true;
            response.reply_text = ai_reply;
            response.session_id = session.session_id;
            response.voice_profile = session.voice_profile;
            response.need_tts = true;

            SPDLOG_INFO("Chat success: user_id={}, session_id={}", user_id, session.session_id);
            return response;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Chat error: user_id={}, error={}", user_id, e.what());
            response.error_message = "聊天服务出现问题，请稍后再试";
            return response;
        }
    }

    /**
     * 设置亲人音色：保存孙辈录音URL，用于TTS合成
     * @param user_id 用户ID
     * @param audio_url 录音文件URL（云存储地址）
     * @return 是否成功
     */
    static bool SetVoiceProfile(const std::string& user_id, const std::string& audio_url) {
        try {
            // 1. 校验参数
            if (user_id.empty() || audio_url.empty()) {
                throw std::invalid_argument("用户ID或音频URL不能为空");
            }

            // 2. 保存到用户配置（实际需存储到数据库USER_VOICE_PROFILE表）
            // UserDao::SaveVoiceProfile(user_id, audio_url);
            
            SPDLOG_INFO("Set voice profile success: user_id={}, url={}", user_id, audio_url);
            return true;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Set voice profile error: user_id={}, error={}", user_id, e.what());
            return false;
        }
    }

    /**
     * 获取节日祝福：根据当前日期返回祝福语
     * @param user_id 用户ID
     * @return 祝福文本
     */
    static std::string GetFestivalGreeting(const std::string& user_id) {
        try {
            std::string current_date = TimeUtil::GetCurrentDateStr("MM-DD");
            
            // 节日祝福配置（可扩展为配置文件或数据库）
            static std::map<std::string, std::string> festival_greetings = {
                {"01-01", "新年快乐！祝您身体健康，万事如意！"},
                {"02-14", "情人节快乐！愿您和家人温馨幸福！"},
                {"05-01", "劳动节快乐！祝您节日愉快，天天开心！"},
                {"10-01", "国庆节快乐！祝福祖国繁荣昌盛，祝您健康长寿！"},
                {"10-04", "重阳节快乐！祝您福如东海，寿比南山！"}
            };

            // 检查当前日期是否为节日
            auto it = festival_greetings.find(current_date);
            if (it != festival_greetings.end()) {
                return it->second;
            }

            return ""; // 非节日返回空字符串

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Get festival greeting error: {}", e.what());
            return "";
        }
    }

    /**
     * 语音意图识别：解析用户语音指令，判断是否需要调用其他服务
     * @param user_id 用户ID
     * @param text 语音识别文本
     * @return 意图结构体
     */
    struct VoiceIntent {
        std::string intent_type;        // 意图类型：chat/taxi/payment/register/none
        std::map<std::string, std::string> slots;  // 槽位信息
        double confidence;              // 置信度
    };

    static VoiceIntent ParseVoiceIntent(const std::string& user_id, const std::string& text) {
        VoiceIntent intent;
        intent.confidence = 0.0;
        intent.intent_type = "none";

        try {
            // 1. 关键词匹配（简单规则）
            if (text.find("打车") != std::string::npos || 
                text.find("叫车") != std::string::npos ||
                text.find("去医院") != std::string::npos ||
                text.find("去超市") != std::string::npos) {
                intent.intent_type = "taxi";
                intent.confidence = 0.90;
                
                // 提取目的地
                if (text.find("医院") != std::string::npos) {
                    intent.slots["destination"] = "医院";
                } else if (text.find("超市") != std::string::npos) {
                    intent.slots["destination"] = "超市";
                } else if (text.find("回家") != std::string::npos || text.find("回去") != std::string::npos) {
                    intent.slots["destination"] = "家";
                }
            }
            else if (text.find("缴费") != std::string::npos || 
                     text.find("交费") != std::string::npos ||
                     text.find("水费") != std::string::npos ||
                     text.find("电费") != std::string::npos ||
                     text.find("网费") != std::string::npos) {
                intent.intent_type = "payment";
                intent.confidence = 0.92;
                
                // 提取缴费类型
                if (text.find("水费") != std::string::npos) {
                    intent.slots["payment_type"] = "水费";
                } else if (text.find("电费") != std::string::npos) {
                    intent.slots["payment_type"] = "电费";
                } else if (text.find("网费") != std::string::npos) {
                    intent.slots["payment_type"] = "网费";
                }
            }
            else if (text.find("挂号") != std::string::npos || 
                     text.find("预约") != std::string::npos ||
                     text.find("看病") != std::string::npos) {
                intent.intent_type = "register";
                intent.confidence = 0.88;
                
                // 提取科室
                if (text.find("内科") != std::string::npos) {
                    intent.slots["department"] = "内科";
                } else if (text.find("外科") != std::string::npos) {
                    intent.slots["department"] = "外科";
                }
            }
            else {
                // 默认为闲聊
                intent.intent_type = "chat";
                intent.confidence = 0.85;
            }

            SPDLOG_INFO("Parse intent: user_id={}, text={}, intent={}", 
                user_id, text, intent.intent_type);
            return intent;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Parse intent error: {}", e.what());
            intent.intent_type = "none";
            return intent;
        }
    }

private:
    /**
     * 创建新会话
     */
    static ChatSession CreateNewSession(const std::string& user_id) {
        ChatSession session;
        session.session_id = GenerateSessionId();
        session.user_id = user_id;
        session.voice_profile = ""; // 默认无音色，可从用户配置读取
        session.create_time = TimeUtil::GetCurrentTimestamp();
        session.last_active_time = session.create_time;
        session.expire_time = session.create_time + 1800; // 30分钟有效期
        return session;
    }

    /**
     * 添加主动问候
     */
    static void AddGreeting(ChatSession& session) {
        std::vector<std::string> greetings = {
            "你好呀！今天天气不错，想聊点什么？",
            "您好！我在这里陪您聊天，有什么想说的吗？",
            "您好！很高兴见到您，今天心情怎么样？"
        };
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, greetings.size() - 1);
        
        ChatMessage greeting;
        greeting.role = "assistant";
        greeting.content = greetings[dis(gen)];
        greeting.timestamp = TimeUtil::GetCurrentTimestamp();
        session.messages.push_back(greeting);
    }

    /**
     * 调用文心一言API
     */
    static std::string CallWenxinAPI(const ChatSession& session) {
        try {
            // 1. 构造请求参数（文心一言ERNIE-Bot-turbo API格式）
            nlohmann::json req_params;
            req_params["messages"] = nlohmann::json::array();
            
            // 添加系统提示词（定义AI角色）
            nlohmann::json system_msg;
            system_msg["role"] = "user";
            system_msg["content"] = "你是一个温暖、耐心的老年人陪聊助手，名叫"老友"。你的任务是陪老年人聊天解闷，语气要亲切、简单易懂，避免使用网络用语。当老人提到家人时，要表达理解和关怀。";
            req_params["messages"].push_back(system_msg);

            // 添加历史对话
            for (const auto& msg : session.messages) {
                nlohmann::json json_msg;
                json_msg["role"] = msg.role;
                json_msg["content"] = msg.content;
                req_params["messages"].push_back(json_msg);
            }

            // 2. 获取access_token（实际需从配置读取并缓存）
            std::string access_token = GetWenxinAccessToken();
            if (access_token.empty()) {
                throw std::runtime_error("文心一言API未授权");
            }

            // 3. 发送HTTP请求
            std::string api_url = fmt::format(
                "https://aip.baidubce.com/rpc/2.0/ai_custom/v1/wenxinworkshop/chat/completions_pro?access_token={}",
                access_token);
            
            std::vector<std::pair<std::string, std::string>> headers = {
                {"Content-Type", "application/json"}
            };
            
            std::string response = HttpClient::Post(api_url, req_params.dump(), headers, true);
            
            // 4. 解析响应
            nlohmann::json res_json = JsonUtil::Parse(response);
            if (res_json.contains("result")) {
                return res_json["result"].get<std::string>();
            } else {
                SPDLOG_WARN("Wenxin API response invalid: {}", response);
                return GetFallbackReply(); // 降级回复
            }

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Call Wenxin API error: {}", e.what());
            return GetFallbackReply();
        }
    }

    /**
     * 获取文心一言access_token
     */
    static std::string GetWenxinAccessToken() {
        // 简化实现：实际需从配置读取API_KEY和SECRET_KEY，并缓存token
        // ConfigParser config("config/app.ini");
        // std::string api_key = config.GetString("wenxin", "api_key");
        // std::string secret_key = config.GetString("wenxin", "secret_key");
        
        // 调用鉴权API获取token（省略，返回示例token）
        static std::string cached_token = "24.xxxxx.xxxx"; // 需动态获取
        return cached_token;
    }

    /**
     * 降级回复：AI服务不可用时的备用回复
     */
    static std::string GetFallbackReply() {
        std::vector<std::string> fallback_replies = {
            "不好意思，我刚才走神了，您能再说一遍吗？",
            "让我想想......能再详细说说吗？",
            "这个问题有点难，我需要好好想想。"
        };
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, fallback_replies.size() - 1);
        return fallback_replies[dis(gen)];
    }

    /**
     * 生成会话ID
     */
    static std::string GenerateSessionId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(100000, 999999);
        return fmt::format("CHAT_SESSION{}{}", 
            TimeUtil::GetCurrentTimestamp(), dis(gen));
    }

    /**
     * 保存会话（简化实现：实际需存储到Redis）
     */
    static void SaveSession(const ChatSession& session) {
        // RedisClient::Set(session.session_id, JsonUtil::Serialize(session), 1800);
        SPDLOG_INFO("Save chat session: session_id={}, user_id={}", 
            session.session_id, session.user_id);
    }

    /**
     * 获取会话（简化实现：实际需从Redis读取）
     */
    static ChatSession GetSession(const std::string& session_id) {
        // std::string data = RedisClient::Get(session_id);
        // return JsonUtil::Deserialize<ChatSession>(data);
        
        // 简化实现：返回空会话
        ChatSession session;
        session.session_id = "";
        return session;
    }
};

#endif // CHAT_SERVICE_H
