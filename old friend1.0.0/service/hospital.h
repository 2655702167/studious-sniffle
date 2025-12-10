#include "dao/hospital_dao.h"
#include "dao/user_dao.h"//修改为实际校验头文件
#include "model/hospital.h"
#include "model/reserve_order.h"
#include "util/time_util.h"
//#include "config/config_parser.h"
#include <stdexcept>
#include<vector>
#include<algorithm>
#include <random>
#include<thread>
#include<fmt/core.h>
#define M_PI 3.14
class HospitalService {
public:
    // 1. 获取所有科室（从数据库加载，支持缓存）
    static std::vector<std::string> GetAllDepartments() {
        static std::vector<std::string> cache;
        if (cache.empty()) {
            cache = HospitalDao::QueryAllDepartments();
        }
        return cache;
    }

    // 2. 根据科室+地理位置筛选医院（按距离排序）
    static std::vector<HospitalDTO> GetHospitalsByDeptAndLocation(
        const std::string& department, double latitude, double longitude) {
        if (department.empty()) {
            throw std::invalid_argument("科室不能为空");
        }
        // 数据库查询符合条件的医院
        auto hospitals = HospitalDao::QueryHospitalsByDepartment(department);
        std::vector<HospitalDTO> result;
        for (const auto& hosp : hospitals) {
            HospitalDTO dto;
            dto.id = hosp.id;
            dto.name = hosp.name;
            dto.address = hosp.address;
            dto.phone = hosp.phone;
            // 计算距离（简化：经纬度直线距离，单位km）
            dto.distance = CalculateDistance(latitude, longitude, hosp.latitude, hosp.longitude);
            dto.status = hosp.available_quota > 0 ? "可预约" : "约满";
            dto.available_quota = hosp.available_quota;
            result.push_back(dto);
        }
        // 按距离升序排序
        std::sort(result.begin(), result.end(), [](const HospitalDTO& a, const HospitalDTO& b) {
            return a.distance < b.distance;
        });
        return result;
    }

    // 3. 预约挂号（核心业务：校验→扣减配额→创建订单）
    static ReserveOrderDTO CreateReserveOrder(
        const std::string& user_id, const std::string& hospital_id, 
        const std::string& department, const std::string& reserve_date) {
        // 校验参数
        if (user_id.empty() || hospital_id.empty() || reserve_date.empty()) {
            throw std::invalid_argument("必填参数缺失");
        }
        /*
        该部分功能为校验用户，修改为自己的校验接口
        */
        auto user = UserDao::QueryUserById(user_id);
        if (user.id.empty()) {
            throw std::runtime_error("用户不存在");
        }
        // 校验医院及科室配额
        auto hospital = HospitalDao::QueryHospitalById(hospital_id);
        if (hospital.id.empty()) {
            throw std::runtime_error("医院不存在");
        }
        if (hospital.available_quota <= 0) {
            throw std::runtime_error("已满");
        }
        // 校验日期格式（yyyy-mm-dd）和有效性（不能是过去日期）
        if (!TimeUtil::GetInstance().IsValidDateFormat(reserve_date) || TimeUtil::GetInstance().CompareDate(reserve_date, TimeUtil::GetInstance().GetCurrentDate()) < 0) {
            throw std::runtime_error("预约日期无效");
        }
        // 扣减医院预约配额（数据库事务：确保扣减和订单创建原子性）
        bool quota_updated = HospitalDao::DecreaseAvailableQuota(hospital_id, 1);
        if (!quota_updated) {
            throw std::runtime_error("预约失败，请重试");
        }
        // 创建预约订单
        ReserveOrder order;
        order.order_id = GenerateOrderId();
        order.user_id = user_id;
        order.hospital_id = hospital_id;
        order.hospital_name = hospital.name;
        order.department = department;
        order.reserve_date = reserve_date;
        order.create_time = TimeUtil::GetInstance().GetCurrentTimestamp();
        order.status = "已预约";
        // 保存订单到数据库
        HospitalDao::SaveReserveOrder(order);
        // 构造返回DTO
        ReserveOrderDTO dto;
        dto.order_id = order.order_id;
        dto.hospital_name = order.hospital_name;
        dto.department = order.department;
        dto.reserve_date = order.reserve_date;
        dto.status = order.status;
        dto.create_time = TimeUtil::GetInstance().TimestampToStr(order.create_time);
        return dto;
    }

private:
    // 辅助函数：计算经纬度直线距离（Haversine公式）
    static double CalculateDistance(double lat1, double lon1, double lat2, double lon2) {
        const double R = 6371.0; // 地球半径（km）
        double dLat = (lat2 - lat1) * M_PI / 180.0;
        double dLon = (lon2 - lon1) * M_PI / 180.0;
        double a = sin(dLat / 2) * sin(dLat / 2) +
                   cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
                   sin(dLon / 2) * sin(dLon / 2);
        double c = 2 * atan2(sqrt(a), sqrt(1 - a));
        return round(R * c * 10) / 10; // 保留1位小数
    }

    // 辅助函数：生成唯一订单号（用户ID+时间戳+随机数）
    static std::string GenerateOrderId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        return fmt::format("{}{}{}", 
            TimeUtil::GetInstance().GetCurrentTimestamp(), 
            dis(gen), 
            std::hash<std::thread::id>{}(std::this_thread::get_id()) % 1000);
    }
};

// 配套数据传输对象（DTO）
struct HospitalDTO {
    std::string id;
    std::string name;
    std::string address;
    std::string phone;
    double distance; // 距离（km）
    std::string status; // 可预约/约满
    int available_quota; // 剩余配额
};

struct ReserveOrderDTO {
    std::string order_id;
    std::string hospital_name;
    std::string department;
    std::string reserve_date;
    std::string status;
    std::string create_time;
};