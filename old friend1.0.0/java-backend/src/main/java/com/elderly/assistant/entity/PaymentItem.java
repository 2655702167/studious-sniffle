package com.elderly.assistant.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.io.Serializable;
import java.math.BigDecimal;

/**
 * 缴费项目实体类
 * 对应C++的PaymentItem结构体
 *
 * @author 施汉霖
 */
@Data
@TableName("PAYMENT_CONFIG")
public class PaymentItem implements Serializable {

    private static final long serialVersionUID = 1L;

    /**
     * 缴费项目ID
     */
    @TableId(value = "config_id", type = IdType.INPUT)
    private String itemId;

    /**
     * 用户ID
     */
    @TableField("user_id")
    private String userId;

    /**
     * 缴费类型（水费/电费/网费/话费）
     */
    @TableField("payment_type")
    private String itemType;

    /**
     * 缴费账号（如水电表号）
     */
    @TableField("account_number")
    private String account;

    /**
     * 待缴金额
     */
    @TableField("default_amount")
    private BigDecimal amount;

    /**
     * 状态（欠费/已缴清）
     */
    @TableField("status")
    private String status;

    /**
     * 缴费截止日期（yyyy-MM-dd）
     */
    @TableField("due_date")
    private String dueDate;

    /**
     * 账单所属月份（yyyy-MM）
     */
    @TableField("bill_month")
    private String billMonth;

    /**
     * 备注
     */
    @TableField("remark")
    private String remark;

    /**
     * 创建时间
     */
    @TableField("create_time")
    private Long createTime;

    /**
     * 更新时间
     */
    @TableField("update_time")
    private Long updateTime;

    /**
     * 上次缴费时间
     */
    @TableField("last_pay_time")
    private Long lastPayTime;
}
