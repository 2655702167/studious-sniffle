#ifndef HELP_SERVICE_H
#define HELP_SERVICE_H

#include <string>
#include <vector>
#include <map>
#include "core/logger.h"

/**
 * 帮助服务：提供使用教程、语音操作指南等辅助功能
 * 帮助老年人快速上手小程序各项功能
 */
class HelpService {
public:
    /**
     * 视频教程结构体
     */
    struct VideoTutorial {
        std::string video_id;        // 视频ID
        std::string title;           // 标题
        std::string description;     // 描述
        std::string video_url;       // 视频URL
        int duration;                // 时长（秒）
        std::string cover_url;       // 封面URL
        std::string category;        // 分类：taxi/payment/register
    };

    /**
     * 语音操作指南结构体
     */
    struct VoiceGuide {
        std::string guide_id;        // 指南ID
        std::string title;           // 标题
        std::string description;     // 描述
        std::vector<std::string> examples; // 示例话术
    };

    /**
     * 常见问题结构体
     */
    struct FAQ {
        std::string faq_id;          // 问题ID
        std::string question;        // 问题
        std::string answer;          // 答案
        std::string category;        // 分类
    };

    /**
     * 获取所有视频教程
     * @return 视频教程列表
     */
    static std::vector<VideoTutorial> GetAllVideoTutorials() {
        std::vector<VideoTutorial> tutorials;

        // 1. 打车教程
        VideoTutorial taxi_video;
        taxi_video.video_id = "VIDEO_TAXI_001";
        taxi_video.title = "如何语音打车";
        taxi_video.description = "教您如何使用语音快速叫车，包括说出目的地、选择快捷地址等操作";
        taxi_video.video_url = "https://cdn.example.com/videos/taxi_tutorial.mp4";
        taxi_video.duration = 120; // 2分钟
        taxi_video.cover_url = "https://cdn.example.com/covers/taxi.jpg";
        taxi_video.category = "taxi";
        tutorials.push_back(taxi_video);

        // 2. 缴费教程
        VideoTutorial payment_video;
        payment_video.video_id = "VIDEO_PAYMENT_001";
        payment_video.title = "如何语音缴费";
        payment_video.description = "教您如何使用语音缴纳水费、电费、网费，简单几步完成支付";
        payment_video.video_url = "https://cdn.example.com/videos/payment_tutorial.mp4";
        payment_video.duration = 150; // 2分30秒
        payment_video.cover_url = "https://cdn.example.com/covers/payment.jpg";
        payment_video.category = "payment";
        tutorials.push_back(payment_video);

        // 3. 挂号教程
        VideoTutorial register_video;
        register_video.video_id = "VIDEO_REGISTER_001";
        register_video.title = "如何预约挂号";
        register_video.description = "教您如何在线预约医院挂号，选择科室、医院，快速完成预约";
        register_video.video_url = "https://cdn.example.com/videos/register_tutorial.mp4";
        register_video.duration = 180; // 3分钟
        register_video.cover_url = "https://cdn.example.com/covers/register.jpg";
        register_video.category = "register";
        tutorials.push_back(register_video);

        SPDLOG_INFO("Get video tutorials success, count={}", tutorials.size());
        return tutorials;
    }

    /**
     * 根据分类获取视频教程
     * @param category 分类：taxi/payment/register
     * @return 视频教程
     */
    static VideoTutorial GetVideoTutorialByCategory(const std::string& category) {
        auto tutorials = GetAllVideoTutorials();
        for (const auto& tutorial : tutorials) {
            if (tutorial.category == category) {
                return tutorial;
            }
        }
        return VideoTutorial(); // 返回空对象
    }

    /**
     * 获取语音操作指南
     * @return 语音指南列表
     */
    static std::vector<VoiceGuide> GetVoiceGuides() {
        std::vector<VoiceGuide> guides;

        // 1. 基础语音操作
        VoiceGuide basic_guide;
        basic_guide.guide_id = "GUIDE_BASIC_001";
        basic_guide.title = "语音操作基础";
        basic_guide.description = "按住说话，松开发送。说话要清楚，慢慢说";
        basic_guide.examples = {
            "请在安静的环境下说话",
            "每次说话尽量不超过30秒",
            "说完一句话后稍作停顿"
        };
        guides.push_back(basic_guide);

        // 2. 打车语音指令
        VoiceGuide taxi_guide;
        taxi_guide.guide_id = "GUIDE_TAXI_001";
        taxi_guide.title = "打车语音指令";
        taxi_guide.description = "如何用语音叫车";
        taxi_guide.examples = {
            "我要打车去市第一人民医院",
            "我要去超市",
            "我要回家",
            "帮我叫辆车去儿子家"
        };
        guides.push_back(taxi_guide);

        // 3. 缴费语音指令
        VoiceGuide payment_guide;
        payment_guide.guide_id = "GUIDE_PAYMENT_001";
        payment_guide.title = "缴费语音指令";
        payment_guide.description = "如何用语音缴纳各种费用";
        payment_guide.examples = {
            "我要缴费",
            "我要交电费",
            "帮我交水费",
            "我要缴纳网费"
        };
        guides.push_back(payment_guide);

        // 4. 挂号语音指令
        VoiceGuide register_guide;
        register_guide.guide_id = "GUIDE_REGISTER_001";
        register_guide.title = "挂号语音指令";
        register_guide.description = "如何用语音预约挂号";
        register_guide.examples = {
            "我要挂号",
            "我想挂内科",
            "我要看骨科",
            "帮我预约市一医院"
        };
        guides.push_back(register_guide);

        SPDLOG_INFO("Get voice guides success, count={}", guides.size());
        return guides;
    }

    /**
     * 获取常见问题
     * @param category 分类（可选）
     * @return 常见问题列表
     */
    static std::vector<FAQ> GetFAQs(const std::string& category = "") {
        std::vector<FAQ> faqs;

        // 通用问题
        FAQ faq1;
        faq1.faq_id = "FAQ_GENERAL_001";
        faq1.question = "如何调整字体大小？";
        faq1.answer = "进入"我的"->"设置"->"字体大小"，可以选择标准、大号或超大号字体";
        faq1.category = "general";
        faqs.push_back(faq1);

        FAQ faq2;
        faq2.faq_id = "FAQ_GENERAL_002";
        faq2.question = "语音识别不准确怎么办？";
        faq2.answer = "请在安静环境下说话，吐字清晰，语速适中。如果仍然不准确，可以尝试切换到您的方言模式";
        faq2.category = "general";
        faqs.push_back(faq2);

        // 打车问题
        FAQ faq3;
        faq3.faq_id = "FAQ_TAXI_001";
        faq3.question = "打车需要多久到？";
        faq3.answer = "一般5-10分钟内会有司机接单，具体时间取决于您所在位置附近的司机数量";
        faq3.category = "taxi";
        faqs.push_back(faq3);

        FAQ faq4;
        faq4.faq_id = "FAQ_TAXI_002";
        faq4.question = "如何取消订单？";
        faq4.answer = "在订单页面点击"取消订单"按钮，或对着手机说"取消打车"即可";
        faq4.category = "taxi";
        faqs.push_back(faq4);

        // 缴费问题
        FAQ faq5;
        faq5.faq_id = "FAQ_PAYMENT_001";
        faq5.question = "缴费是否安全？";
        faq5.answer = "非常安全。我们使用微信支付，所有交易都经过加密保护，您的资金安全有保障";
        faq5.category = "payment";
        faqs.push_back(faq5);

        FAQ faq6;
        faq6.faq_id = "FAQ_PAYMENT_002";
        faq6.question = "缴费失败怎么办？";
        faq6.answer = "请检查网络连接和支付账户余额。如仍失败，请联系客服或家人帮忙处理";
        faq6.category = "payment";
        faqs.push_back(faq6);

        // 挂号问题
        FAQ faq7;
        faq7.faq_id = "FAQ_REGISTER_001";
        faq7.question = "可以预约多久之后的号？";
        faq7.answer = "一般可以预约7天内的号源，具体以医院实际情况为准";
        faq7.category = "register";
        faqs.push_back(faq7);

        FAQ faq8;
        faq8.faq_id = "FAQ_REGISTER_002";
        faq8.question = "挂号成功后怎么取号？";
        faq8.answer = "到医院后，凭您的身份证或预约短信在自助机上取号，也可以到窗口人工取号";
        faq8.category = "register";
        faqs.push_back(faq8);

        // 根据分类过滤
        if (!category.empty()) {
            std::vector<FAQ> filtered;
            for (const auto& faq : faqs) {
                if (faq.category == category) {
                    filtered.push_back(faq);
                }
            }
            return filtered;
        }

        SPDLOG_INFO("Get FAQs success, category={}, count={}", 
            category.empty() ? "all" : category, faqs.size());
        return faqs;
    }

    /**
     * 搜索帮助内容
     * @param keyword 关键词
     * @return 搜索结果（FAQ列表）
     */
    static std::vector<FAQ> SearchHelp(const std::string& keyword) {
        std::vector<FAQ> all_faqs = GetFAQs();
        std::vector<FAQ> results;

        for (const auto& faq : all_faqs) {
            if (faq.question.find(keyword) != std::string::npos ||
                faq.answer.find(keyword) != std::string::npos) {
                results.push_back(faq);
            }
        }

        SPDLOG_INFO("Search help success, keyword={}, count={}", 
            keyword, results.size());
        return results;
    }

    /**
     * 获取紧急情况处理指南
     * @return 紧急指南列表
     */
    static std::vector<std::string> GetEmergencyGuides() {
        return {
            "如遇紧急情况，请点击首页右下角"紧急呼叫"按钮",
            "可以快速拨打120急救中心或110报警中心",
            "也可以直接呼叫您设置的紧急联系人（如儿子、女儿）",
            "请确保您已在"我的"->"紧急联系人"中添加家人电话",
            "如果身体不适，请第一时间拨打120或联系家人"
        };
    }

    /**
     * 获取功能快捷入口说明
     * @return 功能说明列表
     */
    static std::map<std::string, std::string> GetFeatureDescriptions() {
        return {
            {"taxi", "语音打车：说出您要去的地方，系统会自动为您叫车"},
            {"payment", "生活缴费：快速缴纳水费、电费、网费等各种费用"},
            {"register", "医院挂号：在线预约挂号，不用去医院排队"},
            {"chat", "老友陪聊：有什么心事都可以和我聊聊天"},
            {"emergency", "紧急呼叫：一键拨打急救电话或联系家人"},
            {"help", "使用帮助：查看视频教程和操作指南"}
        };
    }
};

#endif // HELP_SERVICE_H
