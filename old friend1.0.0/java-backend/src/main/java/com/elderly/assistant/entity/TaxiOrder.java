package com.elderly.assistant.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.io.Serializable;
import java.math.BigDecimal;

/**
 * 打车订单实体类
 * 对应C++的TaxiOrder结构体
 *
 * @author 施汉霖
 */
@Data
@TableName("TAXI_ORDER")
public class TaxiOrder implements Serializable {

    private static final long serialVersionUID = 1L;

    /**
     * 订单ID
     */
    @TableId(value = "order_id", type = IdType.INPUT)
    private String orderId;

    /**
     * 用户ID
     */
    @TableField("user_id")
    private String userId;

    /**
     * 支付订单号
     */
    @TableField("out_trade_no")
    private String outTradeNo;

    /**
     * 起点地址
     */
    @TableField("start_address")
    private String startAddress;

    /**
     * 起点经度
     */
    @TableField("start_longitude")
    private BigDecimal startLongitude;

    /**
     * 起点纬度
     */
    @TableField("start_latitude")
    private BigDecimal startLatitude;

    /**
     * 终点地址
     */
    @TableField("end_address")
    private String endAddress;

    /**
     * 终点经度
     */
    @TableField("end_longitude")
    private BigDecimal endLongitude;

    /**
     * 终点纬度
     */
    @TableField("end_latitude")
    private BigDecimal endLatitude;

    /**
     * 出发时间
     */
    @TableField("start_time")
    private String startTime;

    /**
     * 司机ID
     */
    @TableField("driver_id")
    private String driverId;

    /**
     * 司机姓名
     */
    @TableField("driver_name")
    private String driverName;

    /**
     * 车牌号
     */
    @TableField("license_plate")
    private String licensePlate;

    /**
     * 司机手机号
     */
    @TableField("driver_phone")
    private String driverPhone;

    /**
     * 订单状态（0-待派单,1-已派单,2-司机已接单,3-已接驾,4-已完成,5-已取消）
     */
    @TableField("status")
    private Integer status;

    /**
     * 实际行驶距离(km)
     */
    @TableField("distance")
    private BigDecimal distance;

    /**
     * 实际行驶时长(分钟)
     */
    @TableField("duration")
    private Integer duration;

    /**
     * 起步价
     */
    @TableField("base_fee")
    private BigDecimal baseFee;

    /**
     * 里程费
     */
    @TableField("distance_fee")
    private BigDecimal distanceFee;

    /**
     * 时长费
     */
    @TableField("time_fee")
    private BigDecimal timeFee;

    /**
     * 附加费
     */
    @TableField("extra_fee")
    private BigDecimal extraFee;

    /**
     * 优惠金额
     */
    @TableField("discount_fee")
    private BigDecimal discountFee;

    /**
     * 总费用
     */
    @TableField("total_fee")
    private BigDecimal totalFee;

    /**
     * 支付状态
     */
    @TableField("pay_status")
    private String payStatus;

    /**
     * 创建时间
     */
    @TableField("create_time")
    private Long createTime;

    /**
     * 派单时间
     */
    @TableField("dispatch_time")
    private Long dispatchTime;

    /**
     * 接单时间
     */
    @TableField("accept_time")
    private Long acceptTime;

    /**
     * 接驾时间
     */
    @TableField("pick_up_time")
    private Long pickUpTime;

    /**
     * 完成时间
     */
    @TableField("complete_time")
    private Long completeTime;

    /**
     * 取消时间
     */
    @TableField("cancel_time")
    private Long cancelTime;

    /**
     * 取消人
     */
    @TableField("cancelor")
    private String cancelor;

    /**
     * 取消原因
     */
    @TableField("cancel_reason")
    private String cancelReason;

    /**
     * 备注
     */
    @TableField("remark")
    private String remark;
}
