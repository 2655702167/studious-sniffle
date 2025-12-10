#ifndef TAXI_SERVICE_H
#define TAXI_SERVICE_H

#include "dao/taxi_dao.h"
#include "dao/user_dao.h"
#include "model/taxi_order.h"
#include "model/taxi.h"
#include "util/time_util.h"
#include "config/config_parser.h"
#include "core/logger.h"
#include <stdexcept>
#include <fmt/core.h>
#include <random>
#include <algorithm>
#include <vector>

// 打车服务常量定义
constexpr int TAXI_ORDER_EXPIRE_SECONDS = 600; // 订单超时未派单自动取消（10分钟）
constexpr int RESERVE_ORDER_MIN_ADVANCE_MINUTES = 30; // 预约单最小提前时间（30分钟）
constexpr double MAX_DISPATCH_DISTANCE = 5.0; // 司机匹配最大距离（5km，可配置）
constexpr double BASE_FEE = 10.0; // 起步价（元）
constexpr double DISTANCE_FEE_PER_KM = 2.3; // 里程费（元/km）
constexpr double TIME_FEE_PER_MIN = 0.5; // 时长费（元/分钟）
constexpr double ELDERLY_SERVICE_EXTRA_FEE = 5.0; // 老年专项服务费（元，可选）

/**
 * 打车服务层：封装打车全流程业务逻辑，适配老年人快捷出行需求
 */
class TaxiService {
public:
    // 依赖注入：通过抽象DAO和工具类隔离依赖，便于测试
    TaxiService(ITaxiDao* taxi_dao, IUserDao* user_dao, ITimeUtil* time_util)
        : taxi_dao_(taxi_dao), user_dao_(user_dao), time_util_(time_util) {}

    // ====================== 地址管理相关（常用地址） ======================
    /**
     * 添加用户常用地址（如家庭、医院）
     */
    std::string AddCommonAddress(const std::string& user_id, const TaxiCommonAddress& addr) {
        // 1. 参数校验
        if (user_id.empty()) {
            throw std::invalid_argument("用户ID不能为空");
        }
        if (addr.display_name.empty()) {
            throw std::invalid_argument("地址显示名称不能为空（如'我家'）");
        }
        if (!addr.location.IsValid()) {
            throw std::invalid_argument("地址位置无效，请选择正确的地址");
        }

        // 2. 构造完整地址信息
        TaxiCommonAddress new_addr = addr;
        new_addr.addr_id = GenerateCommonAddrId();
        new_addr.user_id = user_id;
        new_addr.create_time = time_util_->GetCurrentTimestamp();
        new_addr.update_time = new_addr.create_time;
        new_addr.last_use_time = new_addr.create_time;

        // 3. 若设置为默认地址，取消其他默认地址
        if (new_addr.is_default) {
            taxi_dao_->CancelOtherDefaultCommonAddress(user_id);
        }

        // 4. 保存到DAO
        bool save_ok = taxi_dao_->SaveCommonAddress(new_addr);
        if (!save_ok) {
            throw std::runtime_error("常用地址添加失败，请重试");
        }

        SPDLOG_INFO("用户添加常用地址成功，user_id={}, addr_id={}, name={}", 
            user_id, new_addr.addr_id, new_addr.display_name);
        return new_addr.addr_id;
    }

    /**
     * 查询用户常用地址列表（按优先级+最近使用排序）
     */
    std::vector<TaxiCommonAddressDTO> QueryUserCommonAddresses(const std::string& user_id) {
        if (user_id.empty()) {
            throw std::invalid_argument("用户ID不能为空");
        }

        std::vector<TaxiCommonAddress> addrs = taxi_dao_->QueryCommonAddressesByUserId(user_id);
        // 排序：默认地址 → 优先级（1最高）→ 最近使用时间（倒序）
        std::sort(addrs.begin(), addrs.end(), [](const TaxiCommonAddress& a, const TaxiCommonAddress& b) {
            if (a.is_default != b.is_default) return a.is_default;
            if (a.priority != b.priority) return a.priority < b.priority;
            return a.last_use_time > b.last_use_time;
        });

        // 转换为DTO（脱敏+简化字段，返回给前端）
        std::vector<TaxiCommonAddressDTO> dto_list;
        for (const auto& addr : addrs) {
            TaxiCommonAddressDTO dto;
            dto.addr_id = addr.addr_id;
            dto.display_name = addr.display_name;
            dto.tag = AddressTagToString(addr.tag);
            dto.detailed_address = addr.location.address;
            dto.is_default = addr.is_default;
            dto.note = addr.note;
            dto_list.push_back(dto);
        }

        return dto_list;
    }

    // ====================== 订单相关（下单→派单→接驾→完成） ======================
    /**
     * 创建打车订单（支持常用地址/快捷目的地/手动输入地址）
     * @param input 下单输入参数（包含起点/终点类型：常用地址ID/快捷目的地ID/手动地址）
     * @return 订单DTO（含订单号、状态）
     */
    TaxiOrderDTO CreateTaxiOrder(const TaxiOrderCreateInput& input) {
        // 1. 基础参数校验
        if (input.user_id.empty()) {
            throw std::invalid_argument("用户ID不能为空");
        }
        if (input.start_type.empty() || input.end_type.empty()) {
            throw std::invalid_argument("起点和终点类型不能为空");
        }

        // 2. 解析起点和终点位置
        TaxiLocation start_loc = ResolveLocation(input.user_id, input.start_type, input.start_id, input.start_location);
        TaxiLocation end_loc = ResolveLocation(input.user_id, input.end_type, input.end_id, input.end_location);


        // 4. 构造订单信息
        TaxiOrder order;
        order.order_id = GenerateOrderId();
        order.user_id = input.user_id;
        order.start_location = start_loc;
        order.end_location = end_loc;
        order.start_time = input.start_time;
        order.status = TaxiOrder::OrderStatus::PENDING_DISPATCH;
        order.create_time = time_util_->GetCurrentTimestamp();
        order.expire_time = order.create_time + TAXI_ORDER_EXPIRE_SECONDS;

        // 5. 预计算费用（预估距离）
        double estimate_distance = start_loc.CalculateDistanceTo(end_loc);
        CalculateEstimateFee(order, estimate_distance);

        // 6. 保存订单
        bool save_ok = taxi_dao_->SaveTaxiOrder(order);
        if (!save_ok) {
            throw std::runtime_error("订单创建失败，请重试");
        }

        DispatchOrder(order.order_id);

        SPDLOG_INFO("创建打车订单成功，order_id={}, user_id={}, start={}, end={}", 
            order.order_id, input.user_id, start_loc.address, end_loc.address);
        return ConvertOrderToDTO(order);
    

    
        // 3. 更新订单状态
        order.status = TaxiOrder::OrderStatus::DRIVER_ACCEPTED;
        order.accept_time = time_util_->GetCurrentTimestamp();
        order.update_time = order.accept_time;
}

    /**
     * 确认送达（到达目的地，结束行程）
     */
    TaxiOrderDTO ConfirmDropOff(const std::string& driver_id, const std::string& order_id, double actual_distance, int actual_duration) {
        // 1. 校验并更新订单状态
        TaxiOrder order = taxi_dao_->QueryTaxiOrderById(order_id);
        if (order.order_id.empty()) {
            throw std::runtime_error("订单不存在");
        }
        if (order.driver_id != driver_id) {
            throw std::runtime_error("无权操作该订单（非接单司机）");
        }
        if (order.status != TaxiOrder::OrderStatus::PICKED_UP) {
            throw std::runtime_error(fmt::format("订单当前状态{}，无法确认送达", OrderStatusToString(order.status)));
        }

        // 2. 计算实际费用
        order.distance = actual_distance;
        order.duration = actual_duration;
        CalculateActualFee(order);

        // 3. 更新订单状态
        order.status = TaxiOrder::OrderStatus::COMPLETED;
        order.complete_time = time_util_->GetCurrentTimestamp();
        order.update_time = order.complete_time;
        order.pay_status = "未支付"; // 触发支付流程

      

        // 4. 原子更新
        bool update_order_ok = taxi_dao_->UpdateTaxiOrder(order);
        bool update_driver_ok = taxi_dao_->UpdateDriver(driver);
        if (!update_order_ok || !update_driver_ok) {
            throw std::runtime_error("确认送达失败，请重试");
        }

        SPDLOG_INFO("订单完成，order_id={}, actual_distance={}km, total_fee={}元", 
            order_id, actual_distance, order.total_fee);
        return ConvertOrderToDTO(order);

    }
private:
    // ====================== 辅助函数 ======================
    /**
     * 解析位置（支持常用地址ID/快捷目的地ID/手动地址）
     */
    TaxiLocation ResolveLocation(const std::string& user_id, const std::string& type, const std::string& id, const TaxiLocation& manual_loc) {
        if (type == "common_addr") {
            // 常用地址ID
            if (id.empty()) {
                throw std::invalid_argument("常用地址ID不能为空");
            }
            TaxiCommonAddress addr = taxi_dao_->QueryCommonAddressById(user_id, id);
            if (addr.addr_id.empty()) {
                throw std::runtime_error("常用地址不存在");
            }
            // 更新最近使用时间
            addr.last_use_time = time_util_->GetCurrentTimestamp();
            addr.update_time = addr.last_use_time;
            taxi_dao_->UpdateCommonAddress(addr);
            return addr.location;
        } else if (type == "quick_dest") {
            // 快捷目的地ID
            if (id.empty()) {
                throw std::invalid_argument("快捷目的地ID不能为空");
            }
            TaxiQuickDestination dest = taxi_dao_->QueryQuickDestinationById(id);
            if (dest.dest_id.empty()) {
                throw std::runtime_error("快捷目的地不存在");
            }
            // 若为用户收藏，更新最近使用时间
            if (!user_id.empty() && dest.is_user_collect) {
                dest.user_last_use_time = time_util_->GetCurrentTimestamp();
                taxi_dao_->UpdateUserCollectedQuickDestination(dest);
            }
            return dest.location;
        } else if (type == "manual") {
            // 手动输入地址
            if (!manual_loc.IsValid()) {
                throw std::invalid_argument("手动输入地址无效，请选择正确的地址");
            }
            return manual_loc;
        } else {
            throw std::invalid_argument("位置类型无效（支持常用地址/快捷目的地/手动输入）");
        }
    }

    /**
     * 生成常用地址ID
     */
    std::string GenerateCommonAddrId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        return fmt::format("COMMON_ADDR{}{}", time_util_->GetCurrentTimestamp(), dis(gen));
    }

    /**
     * 生成订单ID
     */
    std::string GenerateOrderId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(100000, 999999);
        return fmt::format("TAXI_ORDER{}{}", time_util_->GetCurrentTimestamp(), dis(gen));
    }

    /**
     * 预计算订单费用（基于预估距离）
     */
    void CalculateEstimateFee(TaxiOrder& order, double estimate_distance) {
        order.base_fee = BASE_FEE;
        order.distance_fee = estimate_distance * DISTANCE_FEE_PER_KM;
        order.time_fee = 10 * TIME_FEE_PER_MIN; // 预估时长10分钟（可优化为基于距离和路况）
        order.extra_fee = order.need_elderly_service ? ELDERLY_SERVICE_EXTRA_FEE : 0.0;
        order.discount_fee = 0.0; // 优惠逻辑可扩展
        order.total_fee = round((order.base_fee + order.distance_fee + order.time_fee + order.extra_fee - order.discount_fee) * 100) / 100;
    }

    /**
     * 计算实际订单费用（基于实际距离和时长）
     */
    void CalculateActualFee(TaxiOrder& order) {
        order.base_fee = BASE_FEE;
        order.distance_fee = order.distance * DISTANCE_FEE_PER_KM;
        order.time_fee = order.duration * TIME_FEE_PER_MIN;
        order.extra_fee = order.need_elderly_service ? ELDERLY_SERVICE_EXTRA_FEE : 0.0;
        // 优惠逻辑（如老年优惠、优惠券等，可扩展）
        order.discount_fee = CalculateDiscount(order);
        order.total_fee = round((order.base_fee + order.distance_fee + order.time_fee + order.extra_fee - order.discount_fee) * 100) / 100;
    }

    /**
     * 计算优惠金额（示例：老年人固定优惠3元）
     */
    double CalculateDiscount(const TaxiOrder& order) {
        // 假设老年人用户享受固定优惠，可通过用户中心接口查询用户类型
        bool is_elderly = user_dao_->IsElderlyUser(order.user_id);
        return is_elderly ? 3.0 : 0.0;
    }

    /**
     * 通用订单状态更新函数（减少重复代码）
     */
    bool UpdateOrderStatus(
        const std::string& order_id,
        const std::string& operator_id,
        TaxiOrder::OrderStatus expect_status,
        TaxiOrder::OrderStatus target_status,
        const std::string& operation_name,
        std::function<void(TaxiOrder&)> update_func) {
        TaxiOrder order = taxi_dao_->QueryTaxiOrderById(order_id);
        if (order.order_id.empty()) {
            throw std::runtime_error("订单不存在");
        }
        if (order.status != expect_status) {
            throw std::runtime_error(fmt::format("订单当前状态{}，无法{}", OrderStatusToString(order.status), operation_name));
        }

        // 执行自定义更新逻辑
        update_func(order);

        bool update_ok = taxi_dao_->UpdateTaxiOrder(order);
        if (!update_ok) {
            throw std::runtime_error(fmt::format("{}失败，请重试", operation_name));
        }

        SPDLOG_INFO("订单{}成功，order_id={}, operator_id={}", operation_name, order_id, operator_id);
        return true;
    }

    /**
     * 转换TaxiOrder为DTO（返回给前端）
     */
    TaxiOrderDTO ConvertOrderToDTO(const TaxiOrder& order) {
        TaxiOrderDTO dto;
        dto.order_id = order.order_id;
        dto.user_id = order.user_id;
        dto.start_address = order.start_location.address;
        dto.end_address = order.end_location.address;
        dto.start_time = order.start_time;
        dto.status = OrderStatusToString(order.status);
        dto.elderly_note = order.elderly_note;
        dto.base_fee = order.base_fee;
        dto.distance_fee = order.distance_fee;
        dto.time_fee = order.time_fee;
        dto.extra_fee = order.extra_fee;
        dto.discount_fee = order.discount_fee;
        dto.total_fee = order.total_fee;
        dto.pay_status = order.pay_status;
        dto.create_time = time_util_->TimestampToStr(order.create_time);
        return dto;
    }

    /**
     * 手机号脱敏（138****1234）
     */
    std::string DesensitizePhone(const std::string& phone) {
        if (phone.size() != 11) return phone;
        return phone.substr(0, 3) + "****" + phone.substr(7);
    }

private:
    ITaxiDao* taxi_dao_;    // 打车模块DAO抽象接口
    IUserDao* user_dao_;    // 用户模块DAO抽象接口
    ITimeUtil* time_util_; 
  // 时间工具抽象接口
};

// ====================== 数据传输对象（DTO）和输入参数结构体 ======================
/**
 * 常用地址DTO（返回给前端）
 */
struct TaxiCommonAddressDTO {
    std::string addr_id;        // 常用地址ID
    std::string display_name;   // 显示名称
    std::string tag;            // 地址标签（字符串形式）
    std::string detailed_address; // 详细地址
    bool is_default;            // 是否默认地址
    std::string note;           // 备注
};


/**
 * 下单输入参数结构体
 */
struct TaxiOrderCreateInput {
    std::string user_id;                // 用户ID
    std::string start_type;             // 起点类型（common_addr/quick_dest/manual）
    std::string start_id;               // 起点ID（常用地址ID/快捷目的地ID，手动输入时为空）
    TaxiLocation start_location;        // 手动输入起点（start_type=manual时必填）
    std::string end_type;               // 终点类型（common_addr/quick_dest/manual）
    std::string end_id;                 // 终点ID（常用地址ID/快捷目的地ID，手动输入时为空）
    TaxiLocation end_location;          // 手动输入终点（end_type=manual时必填）
    std::string start_time;             // 出发时间
};

/**
 * 打车订单DTO（返回给前端）
 */
struct TaxiOrderDTO {
    std::string order_id;               // 订单ID
    std::string user_id;                // 用户ID
    std::string start_address;          // 起点地址
    std::string end_address;            // 终点地址
    bool is_reserve_order;              // 是否预约单
    std::string start_time;             // 出发时间
    std::string status;                 // 订单状态（字符串形式）
    double base_fee;                    // 起步价
    double distance_fee;                // 里程费
    double time_fee;                    // 时长费
    double extra_fee;                   // 附加费
    double discount_fee;                // 优惠金额
    double total_fee;                   // 总费用
    std::string pay_status;             // 支付状态
    std::string create_time;            // 创建时间（yyyy-MM-dd HH:mm:ss）
};

#endif // TAXI_SERVICE_H