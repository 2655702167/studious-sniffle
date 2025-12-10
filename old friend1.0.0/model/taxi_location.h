#ifndef TAXI_LOCATION_MODEL_H
#define TAXI_LOCATION_MODEL_H

#include <string>
#include <cmath>
#define M_PI 3.14
/**
 * 地理位置模型：存储经纬度、详细地址，提供距离计算辅助函数
 */
struct TaxiLocation {
    // 经纬度（核心定位字段，支持高德/百度地图坐标系）
    double latitude = 0.0;  // 纬度（如 31.230416，上海地区范围：30.90~31.95）
    double longitude = 0.0; // 经度（如 121.473701，上海地区范围：120.85~122.10）

    // 详细地址（冗余存储，便于老年人理解，如 "上海市黄浦区南京东路123号"）
    std::string address;
    std::string province;   // 省份（如 "上海市"）
    std::string city;       // 城市（如 "上海市"）
    std::string district;   // 区县（如 "黄浦区"）
    std::string detail;     // 详细地址（如 "南京东路123号 小区3号楼"）

    /**
     * 辅助函数：计算与目标位置的直线距离（单位：km，Haversine公式）
     * @param target 目标位置
     * @return 直线距离（保留1位小数）
     */
    double CalculateDistanceTo(const TaxiLocation& target) const {
        const double R = 6371.0; // 地球半径（km）
        double dLat = (target.latitude - latitude) * M_PI / 180.0;
        double dLon = (target.longitude - longitude) * M_PI / 180.0;
        
        double a = sin(dLat / 2) * sin(dLat / 2) +
                   cos(latitude * M_PI / 180.0) * cos(target.latitude * M_PI / 180.0) *
                   sin(dLon / 2) * sin(dLon / 2);
        double c = 2 * atan2(sqrt(a), sqrt(1 - a));
        return round(R * c * 10) / 10; // 保留1位小数，适配老年人认知习惯
    }

    /**
     * 辅助函数：判断位置是否有效（经纬度在合理范围）
     */
    bool IsValid() const {
        // 中国经纬度大致范围：纬度 4°~53°，经度 73°~135°
        return (latitude >= 4.0 && latitude <= 53.0) &&
               (longitude >= 73.0 && longitude <= 135.0) &&
               !address.empty();
    }
};

#endif // TAXI_LOCATION_MODEL_H