#ifndef VOICE_RECOGNITION_H
#define VOICE_RECOGNITION_H

#include <string>
#include <vector>
#include <regex>
#include <map>
#include "util/http_client.h"
#include "util/json_util.h"
#include "core/logger.h"

/**
 * 语音识别工具类：封装第三方语音识别API调用（如百度语音、讯飞等）
 * 适配老年人语音支付场景，支持方言识别、语义理解
 */
class VoiceRecognition {
public:
    /**
     * 语音识别结果结构体
     */
    struct RecognitionResult {
        bool success;                  // 识别是否成功
        std::string text;              // 识别出的文本
        double confidence;             // 识别置信度（0-1）
        std::string error_message;     // 错误信息（失败时）
    };

    /**
     * 语音转文字（主接口）
     * @param audio_data 音频数据（base64编码或二进制）
     * @param format 音频格式（pcm/wav/mp3）
     * @param rate 采样率（8000/16000）
     * @param language 语言类型（普通话/粤语/四川话等）
     * @return 识别结果
     */
    static RecognitionResult SpeechToText(
        const std::string& audio_data,
        const std::string& format = "wav",
        int rate = 16000,
        const std::string& language = "zh") {
        
        RecognitionResult result;
        result.success = false;
        result.confidence = 0.0;

        try {
            // 1. 构造API请求（以百度语音识别为例）
            std::string api_url = GetAPIUrl();
            std::string access_token = GetAccessToken();
            
            if (access_token.empty()) {
                result.error_message = "语音识别服务未授权";
                return result;
            }

            // 2. 构造请求参数
            nlohmann::json req_params = {
                {"format", format},
                {"rate", rate},
                {"channel", 1},
                {"cuid", "elderly_assistant_app"},
                {"token", access_token},
                {"dev_pid", GetDevPidByLanguage(language)}, // 语言模型ID
                {"speech", audio_data},  // base64音频数据
                {"len", audio_data.size()}
            };

            // 3. 发送HTTP POST请求
            std::vector<std::pair<std::string, std::string>> headers = {
                {"Content-Type", "application/json"}
            };
            std::string response = HttpClient::Post(api_url, req_params.dump(), headers, true);
            
            // 4. 解析响应
            nlohmann::json res_json = JsonUtil::Parse(response);
            int err_no = res_json.value("err_no", -1);
            
            if (err_no == 0) {
                // 识别成功
                result.success = true;
                auto results_array = res_json["result"];
                if (!results_array.empty()) {
                    result.text = results_array[0].get<std::string>();
                }
                // 提取置信度（部分API提供）
                if (res_json.contains("confidence")) {
                    result.confidence = res_json["confidence"].get<double>();
                } else {
                    result.confidence = 0.95; // 默认高置信度
                }
                SPDLOG_INFO("语音识别成功: {}", result.text);
            } else {
                // 识别失败
                result.error_message = res_json.value("err_msg", "识别失败");
                SPDLOG_WARN("语音识别失败: err_no={}, msg={}", err_no, result.error_message);
            }

        } catch (const std::exception& e) {
            result.error_message = std::string("语音识别异常: ") + e.what();
            SPDLOG_ERROR("语音识别异常: {}", e.what());
        }

        return result;
    }

    /**
     * 从识别文本中提取支付意图
     * @param text 识别出的文本
     * @return 支付意图结构体
     */
    struct PaymentIntent {
        bool is_payment;               // 是否为支付意图
        std::string payment_type;      // 缴费类型（水费/电费/网费/话费）
        double amount;                 // 金额（0表示未提及）
        bool is_confirm;               // 是否为确认指令
        std::string original_text;     // 原始文本
    };

    static PaymentIntent ExtractPaymentIntent(const std::string& text) {
        PaymentIntent intent;
        intent.is_payment = false;
        intent.amount = 0.0;
        intent.is_confirm = false;
        intent.original_text = text;

        // 1. 判断是否为支付意图（关键词匹配）
        std::vector<std::string> payment_keywords = {
            "缴费", "支付", "交费", "付款", "付费", "缴纳"
        };
        for (const auto& keyword : payment_keywords) {
            if (text.find(keyword) != std::string::npos) {
                intent.is_payment = true;
                break;
            }
        }

        // 2. 识别缴费类型
        std::map<std::string, std::vector<std::string>> type_keywords = {
            {"水费", {"水费", "水电", "自来水"}},
            {"电费", {"电费", "水电", "电费账单"}},
            {"网费", {"网费", "宽带", "网络费", "上网费"}},
            {"话费", {"话费", "电话费", "手机费"}}
        };

        for (const auto& [type, keywords] : type_keywords) {
            for (const auto& keyword : keywords) {
                if (text.find(keyword) != std::string::npos) {
                    intent.payment_type = type;
                    break;
                }
            }
            if (!intent.payment_type.empty()) break;
        }

        // 3. 提取金额（正则匹配）
        std::regex amount_regex(R"((\d+\.?\d*)\s*元)");
        std::smatch match;
        if (std::regex_search(text, match, amount_regex)) {
            try {
                intent.amount = std::stod(match[1]);
            } catch (...) {
                intent.amount = 0.0;
            }
        }

        // 4. 判断是否为确认指令
        std::vector<std::string> confirm_keywords = {
            "确认", "确定", "好的", "是的", "对", "没错", "支付"
        };
        for (const auto& keyword : confirm_keywords) {
            if (text.find(keyword) != std::string::npos) {
                intent.is_confirm = true;
                break;
            }
        }

        return intent;
    }

private:
    /**
     * 获取语音识别API地址（可配置）
     */
    static std::string GetAPIUrl() {
        // 实际环境需从配置文件读取
        return "https://vop.baidu.com/server_api";
    }

    /**
     * 获取访问令牌（使用API Key和Secret Key换取）
     * 注：实际环境需实现令牌缓存机制，避免频繁请求
     */
    static std::string GetAccessToken() {
        // 简化实现：实际需从配置读取API Key，并调用鉴权接口
        // ConfigParser config("config/app.ini");
        // std::string api_key = config.GetString("baidu_voice", "api_key");
        // std::string secret_key = config.GetString("baidu_voice", "secret_key");
        
        // 占位实现，返回示例token（实际需调用鉴权API）
        static std::string cached_token = "24.xxxxx.xxxx"; // 实际环境需动态获取
        return cached_token;
    }

    /**
     * 根据语言类型获取语言模型ID
     */
    static int GetDevPidByLanguage(const std::string& language) {
        static std::map<std::string, int> language_map = {
            {"zh", 1537},          // 普通话（支持简单的英文识别）
            {"cantonese", 1637},   // 粤语
            {"sichuan", 1837},     // 四川话
            {"henan", 1936}        // 河南话
        };
        auto it = language_map.find(language);
        return (it != language_map.end()) ? it->second : 1537; // 默认普通话
    }
};

#endif // VOICE_RECOGNITION_H
