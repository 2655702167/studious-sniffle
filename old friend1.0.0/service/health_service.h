#ifndef HEALTH_SERVICE_H
#define HEALTH_SERVICE_H

#include <string>
#include <vector>
#include "dao/health_dao.h"
#include "dao/device_dao.h"
#include "dao/user_dao.h"
#include "util/time_util.h"
#include "core/logger.h"
#include <fmt/core.h>

/**
 * 健康服务：管理智能手环数据同步、健康指标监控、异常预警
 * 支持心率、血压、步数等数据记录，并在指标异常时通知紧急联系人
 */
class HealthService {
public:
    /**
     * 健康日志结构体
     */
    struct HealthLog {
        std::string log_id;          // 日志ID
        std::string user_id;         // 用户ID
        std::string device_sn;       // 设备序列号
        int heart_rate;              // 心率（次/分）
        std::string blood_pressure;  // 血压（格式：120/80）
        int step_count;              // 步数
        std::string log_type;        // 日志类型：auto/manual
        int64_t log_time;            // 记录时间
    };

    /**
     * 设备绑定结构体
     */
    struct DeviceBind {
        std::string bind_id;         // 绑定ID
        std::string user_id;         // 用户ID
        std::string device_type;     // 设备类型：watch/band
        std::string device_brand;    // 设备品牌
        std::string device_sn;       // 设备序列号
        int64_t bind_time;           // 绑定时间
        int64_t last_sync_time;      // 最后同步时间
    };

    /**
     * 健康数据DTO
     */
    struct HealthDataDTO {
        std::string log_id;
        int heart_rate;
        std::string blood_pressure;
        int step_count;
        std::string log_time;
        std::string status;          // 正常/偏高/偏低
        std::string alert_message;   // 预警信息
    };

    /**
     * 健康预警结构体
     */
    struct HealthAlert {
        bool has_alert;              // 是否有预警
        std::string alert_type;      // 预警类型：heart_rate/blood_pressure
        std::string alert_level;     // 预警级别：warning/danger
        std::string alert_message;   // 预警文本
        std::vector<std::string> notify_contacts; // 需要通知的联系人
    };

    /**
     * 同步设备健康数据（智能手环上传数据）
     * @param user_id 用户ID
     * @param device_sn 设备序列号
     * @param heart_rate 心率
     * @param blood_pressure 血压
     * @param step_count 步数
     * @return 健康预警信息
     */
    static HealthAlert SyncHealthData(
        const std::string& user_id,
        const std::string& device_sn,
        int heart_rate,
        const std::string& blood_pressure,
        int step_count) {
        
        HealthAlert alert;
        alert.has_alert = false;

        try {
            // 1. 校验设备绑定关系
            DeviceBind device = DeviceDao::QueryDeviceByUserAndSn(user_id, device_sn);
            if (device.bind_id.empty()) {
                throw std::runtime_error("设备未绑定或绑定关系已失效");
            }

            // 2. 创建健康日志
            HealthLog log;
            log.log_id = GenerateLogId();
            log.user_id = user_id;
            log.device_sn = device_sn;
            log.heart_rate = heart_rate;
            log.blood_pressure = blood_pressure;
            log.step_count = step_count;
            log.log_type = "auto";
            log.log_time = TimeUtil::GetCurrentTimestamp();

            // 3. 保存到数据库
            bool save_ok = HealthDao::SaveHealthLog(log);
            if (!save_ok) {
                throw std::runtime_error("健康数据保存失败");
            }

            // 4. 更新设备最后同步时间
            DeviceDao::UpdateLastSyncTime(device.bind_id, log.log_time);

            // 5. 检测健康异常（心率、血压）
            alert = DetectHealthAbnormality(user_id, heart_rate, blood_pressure);

            // 6. 如果有异常，通知紧急联系人
            if (alert.has_alert) {
                NotifyEmergencyContacts(user_id, alert);
            }

            SPDLOG_INFO("Sync health data success: user_id={}, hr={}, bp={}, alert={}", 
                user_id, heart_rate, blood_pressure, alert.has_alert);
            return alert;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Sync health data error: user_id={}, error={}", 
                user_id, e.what());
            throw;
        }
    }

    /**
     * 查询用户健康数据历史
     * @param user_id 用户ID
     * @param days 查询天数（默认7天）
     * @return 健康数据列表（按时间倒序）
     */
    static std::vector<HealthDataDTO> GetHealthHistory(
        const std::string& user_id, 
        int days = 7) {
        
        try {
            if (user_id.empty()) {
                throw std::invalid_argument("用户ID不能为空");
            }

            int64_t start_time = TimeUtil::GetCurrentTimestamp() - (days * 24 * 3600);
            std::vector<HealthLog> logs = HealthDao::QueryHealthLogsByTime(
                user_id, start_time, TimeUtil::GetCurrentTimestamp());

            std::vector<HealthDataDTO> result;
            for (const auto& log : logs) {
                HealthDataDTO dto;
                dto.log_id = log.log_id;
                dto.heart_rate = log.heart_rate;
                dto.blood_pressure = log.blood_pressure;
                dto.step_count = log.step_count;
                dto.log_time = TimeUtil::TimestampToStr(log.log_time);
                dto.status = EvaluateHealthStatus(log.heart_rate, log.blood_pressure);
                result.push_back(dto);
            }

            return result;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Get health history error: user_id={}, error={}", 
                user_id, e.what());
            throw;
        }
    }

    /**
     * 绑定设备
     * @param user_id 用户ID
     * @param device_type 设备类型
     * @param device_brand 设备品牌
     * @param device_sn 设备序列号
     * @return 绑定ID
     */
    static std::string BindDevice(
        const std::string& user_id,
        const std::string& device_type,
        const std::string& device_brand,
        const std::string& device_sn) {
        
        try {
            // 参数校验
            if (user_id.empty() || device_sn.empty()) {
                throw std::invalid_argument("用户ID或设备序列号不能为空");
            }

            // 检查设备是否已被其他用户绑定
            DeviceBind existing = DeviceDao::QueryDeviceBySn(device_sn);
            if (!existing.bind_id.empty() && existing.user_id != user_id) {
                throw std::runtime_error("该设备已被其他用户绑定");
            }

            // 检查用户是否已绑定该设备
            if (!existing.bind_id.empty() && existing.user_id == user_id) {
                return existing.bind_id; // 已绑定，返回现有绑定ID
            }

            // 创建新绑定
            DeviceBind bind;
            bind.bind_id = GenerateBindId();
            bind.user_id = user_id;
            bind.device_type = device_type;
            bind.device_brand = device_brand;
            bind.device_sn = device_sn;
            bind.bind_time = TimeUtil::GetCurrentTimestamp();
            bind.last_sync_time = 0;

            // 保存到数据库
            bool save_ok = DeviceDao::SaveDeviceBind(bind);
            if (!save_ok) {
                throw std::runtime_error("设备绑定失败");
            }

            SPDLOG_INFO("Bind device success: user_id={}, device_sn={}", 
                user_id, device_sn);
            return bind.bind_id;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Bind device error: user_id={}, error={}", 
                user_id, e.what());
            throw;
        }
    }

    /**
     * 解绑设备
     * @param user_id 用户ID
     * @param bind_id 绑定ID
     * @return 是否成功
     */
    static bool UnbindDevice(const std::string& user_id, const std::string& bind_id) {
        try {
            DeviceBind bind = DeviceDao::QueryDeviceBindById(bind_id);
            if (bind.bind_id.empty()) {
                throw std::runtime_error("绑定记录不存在");
            }

            if (bind.user_id != user_id) {
                throw std::runtime_error("无权解绑该设备");
            }

            bool delete_ok = DeviceDao::DeleteDeviceBind(bind_id);
            if (!delete_ok) {
                throw std::runtime_error("设备解绑失败");
            }

            SPDLOG_INFO("Unbind device success: user_id={}, bind_id={}", 
                user_id, bind_id);
            return true;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Unbind device error: {}", e.what());
            return false;
        }
    }

    /**
     * 查询用户绑定的设备列表
     * @param user_id 用户ID
     * @return 设备列表
     */
    static std::vector<DeviceBindDTO> GetUserDevices(const std::string& user_id) {
        try {
            std::vector<DeviceBind> devices = DeviceDao::QueryDevicesByUserId(user_id);
            std::vector<DeviceBindDTO> result;

            for (const auto& device : devices) {
                DeviceBindDTO dto;
                dto.bind_id = device.bind_id;
                dto.device_type = device.device_type;
                dto.device_brand = device.device_brand;
                dto.device_sn = device.device_sn;
                dto.bind_time = TimeUtil::TimestampToStr(device.bind_time);
                dto.last_sync_time = (device.last_sync_time > 0) 
                    ? TimeUtil::TimestampToStr(device.last_sync_time) 
                    : "从未同步";
                result.push_back(dto);
            }

            return result;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Get user devices error: user_id={}, error={}", 
                user_id, e.what());
            throw;
        }
    }

private:
    /**
     * 检测健康异常
     */
    static HealthAlert DetectHealthAbnormality(
        const std::string& user_id,
        int heart_rate,
        const std::string& blood_pressure) {
        
        HealthAlert alert;
        alert.has_alert = false;

        // 1. 检测心率异常（正常范围：60-100次/分）
        if (heart_rate >= 105) {
            alert.has_alert = true;
            alert.alert_type = "heart_rate";
            alert.alert_level = (heart_rate >= 120) ? "danger" : "warning";
            alert.alert_message = fmt::format(
                "您的心率偏高（{}次/分），请注意休息，如有不适请及时就医", 
                heart_rate);
        } else if (heart_rate <= 55 && heart_rate > 0) {
            alert.has_alert = true;
            alert.alert_type = "heart_rate";
            alert.alert_level = (heart_rate <= 45) ? "danger" : "warning";
            alert.alert_message = fmt::format(
                "您的心率偏低（{}次/分），请注意身体状况", 
                heart_rate);
        }

        // 2. 检测血压异常（正常范围：收缩压90-140，舒张压60-90）
        auto bp_values = ParseBloodPressure(blood_pressure);
        if (bp_values.first > 0 && bp_values.second > 0) {
            if (bp_values.first >= 140 || bp_values.second >= 90) {
                alert.has_alert = true;
                alert.alert_type = "blood_pressure";
                alert.alert_level = (bp_values.first >= 160) ? "danger" : "warning";
                alert.alert_message = fmt::format(
                    "您的血压偏高（{}），请注意休息，避免剧烈运动", 
                    blood_pressure);
            }
        }

        // 3. 如果有异常，获取需要通知的紧急联系人
        if (alert.has_alert) {
            alert.notify_contacts = GetPrimaryEmergencyContacts(user_id);
        }

        return alert;
    }

    /**
     * 评估健康状态
     */
    static std::string EvaluateHealthStatus(int heart_rate, const std::string& blood_pressure) {
        // 心率正常范围：60-100
        bool hr_normal = (heart_rate >= 60 && heart_rate <= 100);
        
        // 血压正常范围：收缩压90-140，舒张压60-90
        auto bp = ParseBloodPressure(blood_pressure);
        bool bp_normal = (bp.first >= 90 && bp.first <= 140) && 
                        (bp.second >= 60 && bp.second <= 90);

        if (hr_normal && bp_normal) {
            return "正常";
        } else if (!hr_normal && bp_normal) {
            return "心率异常";
        } else if (hr_normal && !bp_normal) {
            return "血压异常";
        } else {
            return "多项异常";
        }
    }

    /**
     * 解析血压字符串（120/80 -> {120, 80}）
     */
    static std::pair<int, int> ParseBloodPressure(const std::string& bp) {
        size_t pos = bp.find('/');
        if (pos == std::string::npos) {
            return {0, 0};
        }
        try {
            int systolic = std::stoi(bp.substr(0, pos));
            int diastolic = std::stoi(bp.substr(pos + 1));
            return {systolic, diastolic};
        } catch (...) {
            return {0, 0};
        }
    }

    /**
     * 获取主要紧急联系人（用于健康预警通知）
     */
    static std::vector<std::string> GetPrimaryEmergencyContacts(const std::string& user_id) {
        std::vector<std::string> contacts;
        // 从USER_FAMILY表查询主联系人
        // auto family = UserDao::QueryPrimaryFamily(user_id);
        // if (!family.phone.empty()) {
        //     contacts.push_back(family.phone);
        // }
        return contacts;
    }

    /**
     * 通知紧急联系人（健康异常时）
     */
    static void NotifyEmergencyContacts(const std::string& user_id, const HealthAlert& alert) {
        // 实际环境：通过微信模板消息或短信通知家属
        for (const auto& contact : alert.notify_contacts) {
            SPDLOG_INFO("Notify emergency contact: user_id={}, contact={}, msg={}", 
                user_id, contact, alert.alert_message);
        }
    }

    /**
     * 生成日志ID
     */
    static std::string GenerateLogId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        return fmt::format("HEALTH_LOG{}{}", TimeUtil::GetCurrentTimestamp(), dis(gen));
    }

    /**
     * 生成绑定ID
     */
    static std::string GenerateBindId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        return fmt::format("DEVICE_BIND{}{}", TimeUtil::GetCurrentTimestamp(), dis(gen));
    }
};

/**
 * 设备绑定DTO
 */
struct DeviceBindDTO {
    std::string bind_id;
    std::string device_type;
    std::string device_brand;
    std::string device_sn;
    std::string bind_time;
    std::string last_sync_time;
};

#endif // HEALTH_SERVICE_H
