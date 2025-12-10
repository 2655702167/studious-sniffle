#ifndef EMERGENCY_SERVICE_H
#define EMERGENCY_SERVICE_H

#include <string>
#include <vector>
#include "dao/user_dao.h"
#include "dao/emergency_dao.h"
#include "util/time_util.h"
#include "core/logger.h"
#include <fmt/core.h>

/**
 * 紧急呼叫服务：为老年人提供一键拨打紧急联系人、120、110功能
 * 并记录所有呼叫日志，便于家属追踪
 */
class EmergencyService {
public:
    /**
     * 紧急联系人结构体
     */
    struct EmergencyContact {
        std::string contact_id;     // 联系人ID
        std::string user_id;        // 所属用户ID
        std::string name;           // 姓名
        std::string phone;          // 电话号码
        std::string relation;       // 关系（儿子/女儿/配偶等）
        bool is_primary;            // 是否为主联系人
        int priority;               // 优先级（1最高）
        int64_t create_time;        // 添加时间
    };

    /**
     * 紧急联系人DTO（返回给前端，电话号码脱敏）
     */
    struct EmergencyContactDTO {
        std::string contact_id;
        std::string name;
        std::string phone_display;  // 脱敏电话（138****5678）
        std::string phone_raw;      // 原始电话（用于拨号）
        std::string relation;
        std::string contact_type;   // 类型：family/emergency_service
        bool is_primary;
    };

    /**
     * 呼叫日志结构体
     */
    struct CallLog {
        std::string log_id;         // 日志ID
        std::string user_id;        // 用户ID
        std::string callee_type;    // 呼叫对象类型：family/120/110
        std::string callee_name;    // 呼叫对象姓名
        std::string callee_phone;   // 呼叫电话
        std::string call_status;    // 呼叫状态：initiated/connected/failed/cancelled
        int64_t call_time;          // 呼叫时间
        int call_duration;          // 通话时长（秒）
    };

    /**
     * 获取紧急联系人列表（包含家属联系人+120+110）
     * @param user_id 用户ID
     * @return 紧急联系人列表（按优先级排序）
     */
    static std::vector<EmergencyContactDTO> GetEmergencyContacts(const std::string& user_id) {
        try {
            if (user_id.empty()) {
                throw std::invalid_argument("用户ID不能为空");
            }

            std::vector<EmergencyContactDTO> result;

            // 1. 查询用户家属联系人（从USER_FAMILY表）
            std::vector<EmergencyContact> family_contacts = EmergencyDao::QueryFamilyContacts(user_id);
            
            // 按优先级排序（主联系人优先）
            std::sort(family_contacts.begin(), family_contacts.end(), 
                [](const EmergencyContact& a, const EmergencyContact& b) {
                    if (a.is_primary != b.is_primary) return a.is_primary;
                    return a.priority < b.priority;
                });

            // 转换为DTO
            for (const auto& contact : family_contacts) {
                EmergencyContactDTO dto;
                dto.contact_id = contact.contact_id;
                dto.name = fmt::format("{}：{}", contact.relation, contact.name);
                dto.phone_display = DesensitizePhone(contact.phone);
                dto.phone_raw = contact.phone;
                dto.relation = contact.relation;
                dto.contact_type = "family";
                dto.is_primary = contact.is_primary;
                result.push_back(dto);
            }

            // 2. 添加120急救中心
            EmergencyContactDTO dto_120;
            dto_120.contact_id = "EMERGENCY_120";
            dto_120.name = "120急救中心";
            dto_120.phone_display = "120";
            dto_120.phone_raw = "120";
            dto_120.relation = "急救服务";
            dto_120.contact_type = "emergency_service";
            dto_120.is_primary = false;
            result.push_back(dto_120);

            // 3. 添加110报警中心
            EmergencyContactDTO dto_110;
            dto_110.contact_id = "EMERGENCY_110";
            dto_110.name = "110报警中心";
            dto_110.phone_display = "110";
            dto_110.phone_raw = "110";
            dto_110.relation = "报警服务";
            dto_110.contact_type = "emergency_service";
            dto_110.is_primary = false;
            result.push_back(dto_110);

            SPDLOG_INFO("Get emergency contacts success: user_id={}, count={}", 
                user_id, result.size());
            return result;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Get emergency contacts error: user_id={}, error={}", 
                user_id, e.what());
            throw;
        }
    }

    /**
     * 记录呼叫日志
     * @param user_id 用户ID
     * @param callee_type 呼叫对象类型（family/120/110）
     * @param callee_name 呼叫对象姓名
     * @param callee_phone 呼叫电话
     * @param call_status 呼叫状态
     * @param call_duration 通话时长（秒）
     * @return 日志ID
     */
    static std::string LogEmergencyCall(
        const std::string& user_id,
        const std::string& callee_type,
        const std::string& callee_name,
        const std::string& callee_phone,
        const std::string& call_status,
        int call_duration = 0) {
        
        try {
            if (user_id.empty()) {
                throw std::invalid_argument("用户ID不能为空");
            }

            // 创建呼叫日志
            CallLog log;
            log.log_id = GenerateLogId();
            log.user_id = user_id;
            log.callee_type = callee_type;
            log.callee_name = callee_name;
            log.callee_phone = callee_phone;
            log.call_status = call_status;
            log.call_time = TimeUtil::GetCurrentTimestamp();
            log.call_duration = call_duration;

            // 保存到数据库
            bool save_ok = EmergencyDao::SaveCallLog(log);
            if (!save_ok) {
                throw std::runtime_error("呼叫日志保存失败");
            }

            // 如果是家属联系人且状态为已接通，通知家属（可选功能）
            if (callee_type == "family" && call_status == "connected") {
                NotifyFamilyMember(user_id, callee_name, "老人主动拨打了您的电话");
            }

            SPDLOG_INFO("Emergency call logged: user_id={}, callee={}, status={}", 
                user_id, callee_name, call_status);
            return log.log_id;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Log emergency call error: user_id={}, error={}", 
                user_id, e.what());
            throw;
        }
    }

    /**
     * 查询呼叫历史记录
     * @param user_id 用户ID
     * @param limit 返回条数（默认20条）
     * @return 呼叫日志列表（按时间倒序）
     */
    static std::vector<CallLogDTO> GetCallHistory(const std::string& user_id, int limit = 20) {
        try {
            if (user_id.empty()) {
                throw std::invalid_argument("用户ID不能为空");
            }

            std::vector<CallLog> logs = EmergencyDao::QueryCallLogsByUserId(user_id, limit);
            std::vector<CallLogDTO> result;

            for (const auto& log : logs) {
                CallLogDTO dto;
                dto.log_id = log.log_id;
                dto.callee_name = log.callee_name;
                dto.callee_phone = DesensitizePhone(log.callee_phone);
                dto.call_status = TranslateCallStatus(log.call_status);
                dto.call_time = TimeUtil::TimestampToStr(log.call_time);
                dto.call_duration = FormatDuration(log.call_duration);
                result.push_back(dto);
            }

            return result;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Get call history error: user_id={}, error={}", 
                user_id, e.what());
            throw;
        }
    }

    /**
     * 添加家属联系人
     * @param user_id 用户ID
     * @param name 姓名
     * @param phone 电话
     * @param relation 关系
     * @return 联系人ID
     */
    static std::string AddFamilyContact(
        const std::string& user_id,
        const std::string& name,
        const std::string& phone,
        const std::string& relation) {
        
        try {
            // 参数校验
            if (user_id.empty() || name.empty() || phone.empty()) {
                throw std::invalid_argument("必填参数不能为空");
            }

            if (!IsValidPhone(phone)) {
                throw std::invalid_argument("电话号码格式不正确");
            }

            // 创建联系人
            EmergencyContact contact;
            contact.contact_id = GenerateContactId();
            contact.user_id = user_id;
            contact.name = name;
            contact.phone = phone;
            contact.relation = relation;
            contact.is_primary = false; // 默认非主联系人
            contact.priority = 5; // 默认优先级最低
            contact.create_time = TimeUtil::GetCurrentTimestamp();

            // 保存到数据库
            bool save_ok = EmergencyDao::SaveFamilyContact(contact);
            if (!save_ok) {
                throw std::runtime_error("联系人添加失败");
            }

            SPDLOG_INFO("Add family contact success: user_id={}, contact_id={}, name={}", 
                user_id, contact.contact_id, name);
            return contact.contact_id;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Add family contact error: user_id={}, error={}", 
                user_id, e.what());
            throw;
        }
    }

    /**
     * 设置主联系人
     * @param user_id 用户ID
     * @param contact_id 联系人ID
     * @return 是否成功
     */
    static bool SetPrimaryContact(const std::string& user_id, const std::string& contact_id) {
        try {
            // 1. 取消其他主联系人
            EmergencyDao::CancelPrimaryContacts(user_id);

            // 2. 设置新的主联系人
            bool update_ok = EmergencyDao::UpdateContactPrimary(contact_id, true);
            if (!update_ok) {
                throw std::runtime_error("设置主联系人失败");
            }

            SPDLOG_INFO("Set primary contact success: user_id={}, contact_id={}", 
                user_id, contact_id);
            return true;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Set primary contact error: {}", e.what());
            return false;
        }
    }

private:
    /**
     * 电话号码脱敏（138****5678）
     */
    static std::string DesensitizePhone(const std::string& phone) {
        if (phone.size() != 11) return phone;
        return phone.substr(0, 3) + "****" + phone.substr(7);
    }

    /**
     * 校验电话号码格式
     */
    static bool IsValidPhone(const std::string& phone) {
        if (phone.size() != 11) return false;
        if (phone[0] != '1') return false;
        for (char c : phone) {
            if (!isdigit(c)) return false;
        }
        return true;
    }

    /**
     * 翻译呼叫状态
     */
    static std::string TranslateCallStatus(const std::string& status) {
        static std::map<std::string, std::string> status_map = {
            {"initiated", "已拨打"},
            {"connected", "已接通"},
            {"failed", "拨打失败"},
            {"cancelled", "已取消"}
        };
        auto it = status_map.find(status);
        return (it != status_map.end()) ? it->second : "未知状态";
    }

    /**
     * 格式化通话时长
     */
    static std::string FormatDuration(int seconds) {
        if (seconds == 0) return "未接通";
        int minutes = seconds / 60;
        int secs = seconds % 60;
        if (minutes > 0) {
            return fmt::format("{}分{}秒", minutes, secs);
        }
        return fmt::format("{}秒", secs);
    }

    /**
     * 生成日志ID
     */
    static std::string GenerateLogId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        return fmt::format("CALL_LOG{}{}", TimeUtil::GetCurrentTimestamp(), dis(gen));
    }

    /**
     * 生成联系人ID
     */
    static std::string GenerateContactId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        return fmt::format("CONTACT{}{}", TimeUtil::GetCurrentTimestamp(), dis(gen));
    }

    /**
     * 通知家属成员（可选功能：发送模板消息）
     */
    static void NotifyFamilyMember(
        const std::string& user_id,
        const std::string& family_name,
        const std::string& message) {
        
        // 实际环境可通过微信模板消息或短信通知家属
        SPDLOG_INFO("Notify family: user_id={}, family={}, msg={}", 
            user_id, family_name, message);
    }
};

/**
 * 呼叫日志DTO
 */
struct CallLogDTO {
    std::string log_id;
    std::string callee_name;
    std::string callee_phone;
    std::string call_status;
    std::string call_time;
    std::string call_duration;
};

#endif // EMERGENCY_SERVICE_H
