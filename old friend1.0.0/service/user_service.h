#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include <string>
#include <vector>
#include "dao/user_dao.h"
#include "dao/address_dao.h"
#include "util/time_util.h"
#include "util/crypto_util.h"
#include "core/logger.h"
#include <fmt/core.h>

/**
 * 用户服务：管理用户基础信息、地址、设置等个人中心功能
 */
class UserService {
public:
    /**
     * 用户基础信息结构体
     */
    struct UserInfo {
        std::string user_id;         // 用户ID
        std::string user_name;       // 姓名
        int user_age;                // 年龄
        std::string phone;           // 手机号（加密存储）
        std::string dialect_type;    // 方言类型
        int64_t create_time;         // 注册时间
    };

    /**
     * 用户地址结构体
     */
    struct UserAddress {
        std::string address_id;      // 地址ID
        std::string user_id;         // 用户ID
        std::string address_name;    // 地址名称（家/公司/医院等）
        std::string province;        // 省
        std::string city;            // 市
        std::string detail_address;  // 详细地址
        double longitude;            // 经度
        double latitude;             // 纬度
        bool is_default;             // 是否默认地址
        int64_t create_time;         // 创建时间
    };

    /**
     * 用户设置结构体
     */
    struct UserSettings {
        std::string user_id;
        std::string font_size;       // 字体大小：standard/large/extra_large
        int voice_volume;            // 语音音量：0-100
        std::string dialect_type;    // 方言类型：zh/cantonese/sichuan
        std::string voice_profile;   // 亲人音色URL
    };

    /**
     * 用户信息DTO（返回给前端，手机号脱敏）
     */
    struct UserInfoDTO {
        std::string user_id;
        std::string user_name;
        int user_age;
        std::string phone_display;   // 脱敏手机号
        std::string dialect_type;
        std::string create_time;
    };

    /**
     * 地址DTO
     */
    struct UserAddressDTO {
        std::string address_id;
        std::string address_name;
        std::string full_address;    // 拼接后的完整地址
        bool is_default;
    };

    /**
     * 获取用户基础信息
     * @param user_id 用户ID
     * @return 用户信息DTO
     */
    static UserInfoDTO GetUserProfile(const std::string& user_id) {
        try {
            if (user_id.empty()) {
                throw std::invalid_argument("用户ID不能为空");
            }

            UserInfo user = UserDao::QueryUserById(user_id);
            if (user.user_id.empty()) {
                throw std::runtime_error("用户不存在");
            }

            // 转换为DTO
            UserInfoDTO dto;
            dto.user_id = user.user_id;
            dto.user_name = user.user_name;
            dto.user_age = user.user_age;
            dto.phone_display = DesensitizePhone(CryptoUtil::Decrypt(user.phone));
            dto.dialect_type = user.dialect_type;
            dto.create_time = TimeUtil::TimestampToStr(user.create_time);

            return dto;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Get user profile error: user_id={}, error={}", 
                user_id, e.what());
            throw;
        }
    }

    /**
     * 更新用户基础信息
     * @param user_id 用户ID
     * @param user_name 姓名
     * @param user_age 年龄
     * @param phone 手机号
     * @return 是否成功
     */
    static bool UpdateUserProfile(
        const std::string& user_id,
        const std::string& user_name,
        int user_age,
        const std::string& phone) {
        
        try {
            if (user_id.empty()) {
                throw std::invalid_argument("用户ID不能为空");
            }

            // 校验手机号格式
            if (!phone.empty() && !IsValidPhone(phone)) {
                throw std::invalid_argument("手机号格式不正确");
            }

            // 查询用户
            UserInfo user = UserDao::QueryUserById(user_id);
            if (user.user_id.empty()) {
                throw std::runtime_error("用户不存在");
            }

            // 更新字段
            if (!user_name.empty()) {
                user.user_name = user_name;
            }
            if (user_age > 0) {
                user.user_age = user_age;
            }
            if (!phone.empty()) {
                user.phone = CryptoUtil::Encrypt(phone); // 加密存储
            }

            // 保存到数据库
            bool update_ok = UserDao::UpdateUser(user);
            if (!update_ok) {
                throw std::runtime_error("更新用户信息失败");
            }

            SPDLOG_INFO("Update user profile success: user_id={}", user_id);
            return true;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Update user profile error: user_id={}, error={}", 
                user_id, e.what());
            return false;
        }
    }

    /**
     * 获取用户设置
     * @param user_id 用户ID
     * @return 用户设置
     */
    static UserSettings GetUserSettings(const std::string& user_id) {
        try {
            UserSettings settings = UserDao::QueryUserSettings(user_id);
            if (settings.user_id.empty()) {
                // 返回默认设置
                settings.user_id = user_id;
                settings.font_size = "extra_large"; // 默认超大号字体
                settings.voice_volume = 80;         // 默认音量80%
                settings.dialect_type = "zh";       // 默认普通话
                settings.voice_profile = "";
            }
            return settings;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Get user settings error: user_id={}, error={}", 
                user_id, e.what());
            throw;
        }
    }

    /**
     * 更新用户设置
     * @param settings 用户设置
     * @return 是否成功
     */
    static bool UpdateUserSettings(const UserSettings& settings) {
        try {
            if (settings.user_id.empty()) {
                throw std::invalid_argument("用户ID不能为空");
            }

            // 校验参数合法性
            if (!settings.font_size.empty()) {
                std::vector<std::string> valid_sizes = {"standard", "large", "extra_large"};
                bool valid = false;
                for (const auto& size : valid_sizes) {
                    if (settings.font_size == size) {
                        valid = true;
                        break;
                    }
                }
                if (!valid) {
                    throw std::invalid_argument("字体大小参数无效");
                }
            }

            if (settings.voice_volume < 0 || settings.voice_volume > 100) {
                throw std::invalid_argument("音量参数无效（0-100）");
            }

            // 保存到数据库
            bool update_ok = UserDao::UpdateUserSettings(settings);
            if (!update_ok) {
                throw std::runtime_error("更新用户设置失败");
            }

            SPDLOG_INFO("Update user settings success: user_id={}", settings.user_id);
            return true;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Update user settings error: user_id={}, error={}", 
                settings.user_id, e.what());
            return false;
        }
    }

    /**
     * 查询用户地址列表
     * @param user_id 用户ID
     * @return 地址列表
     */
    static std::vector<UserAddressDTO> ListUserAddresses(const std::string& user_id) {
        try {
            std::vector<UserAddress> addresses = AddressDao::QueryAddressesByUserId(user_id);
            std::vector<UserAddressDTO> result;

            // 排序：默认地址在前
            std::sort(addresses.begin(), addresses.end(), 
                [](const UserAddress& a, const UserAddress& b) {
                    if (a.is_default != b.is_default) return a.is_default;
                    return a.create_time > b.create_time;
                });

            for (const auto& addr : addresses) {
                UserAddressDTO dto;
                dto.address_id = addr.address_id;
                dto.address_name = addr.address_name;
                dto.full_address = fmt::format("{} {} {}", 
                    addr.province, addr.city, addr.detail_address);
                dto.is_default = addr.is_default;
                result.push_back(dto);
            }

            return result;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("List user addresses error: user_id={}, error={}", 
                user_id, e.what());
            throw;
        }
    }

    /**
     * 添加地址
     * @param address 地址信息
     * @return 地址ID
     */
    static std::string AddAddress(const UserAddress& address) {
        try {
            if (address.user_id.empty() || address.detail_address.empty()) {
                throw std::invalid_argument("必填参数不能为空");
            }

            // 创建地址
            UserAddress new_addr = address;
            new_addr.address_id = GenerateAddressId();
            new_addr.create_time = TimeUtil::GetCurrentTimestamp();

            // 如果设为默认地址，取消其他默认地址
            if (new_addr.is_default) {
                AddressDao::CancelDefaultAddresses(new_addr.user_id);
            }

            // 保存到数据库
            bool save_ok = AddressDao::SaveAddress(new_addr);
            if (!save_ok) {
                throw std::runtime_error("添加地址失败");
            }

            SPDLOG_INFO("Add address success: user_id={}, address_id={}", 
                address.user_id, new_addr.address_id);
            return new_addr.address_id;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Add address error: {}", e.what());
            throw;
        }
    }

    /**
     * 设置默认地址
     * @param user_id 用户ID
     * @param address_id 地址ID
     * @return 是否成功
     */
    static bool SetDefaultAddress(const std::string& user_id, const std::string& address_id) {
        try {
            // 1. 取消其他默认地址
            AddressDao::CancelDefaultAddresses(user_id);

            // 2. 设置新的默认地址
            bool update_ok = AddressDao::UpdateAddressDefault(address_id, true);
            if (!update_ok) {
                throw std::runtime_error("设置默认地址失败");
            }

            SPDLOG_INFO("Set default address success: user_id={}, address_id={}", 
                user_id, address_id);
            return true;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Set default address error: {}", e.what());
            return false;
        }
    }

    /**
     * 删除地址
     * @param user_id 用户ID
     * @param address_id 地址ID
     * @return 是否成功
     */
    static bool DeleteAddress(const std::string& user_id, const std::string& address_id) {
        try {
            UserAddress addr = AddressDao::QueryAddressById(address_id);
            if (addr.address_id.empty()) {
                throw std::runtime_error("地址不存在");
            }

            if (addr.user_id != user_id) {
                throw std::runtime_error("无权删除该地址");
            }

            bool delete_ok = AddressDao::DeleteAddress(address_id);
            if (!delete_ok) {
                throw std::runtime_error("删除地址失败");
            }

            SPDLOG_INFO("Delete address success: user_id={}, address_id={}", 
                user_id, address_id);
            return true;

        } catch (const std::exception& e) {
            SPDLOG_ERROR("Delete address error: {}", e.what());
            return false;
        }
    }

private:
    /**
     * 手机号脱敏（136****1234）
     */
    static std::string DesensitizePhone(const std::string& phone) {
        if (phone.size() != 11) return phone;
        return phone.substr(0, 3) + "****" + phone.substr(7);
    }

    /**
     * 校验手机号格式
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
     * 生成地址ID
     */
    static std::string GenerateAddressId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        return fmt::format("ADDR{}{}", TimeUtil::GetCurrentTimestamp(), dis(gen));
    }
};

#endif // USER_SERVICE_H
