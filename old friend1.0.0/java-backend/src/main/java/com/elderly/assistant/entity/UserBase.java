package com.elderly.assistant.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.io.Serializable;

/**
 * 用户基础表实体类
 *
 * @author 施汉霖
 */
@Data
@TableName("USER_BASE")
public class UserBase implements Serializable {

    private static final long serialVersionUID = 1L;

    /**
     * 用户ID
     */
    @TableId(value = "user_id", type = IdType.INPUT)
    private String userId;

    /**
     * 姓名
     */
    @TableField("user_name")
    private String userName;

    /**
     * 年龄
     */
    @TableField("user_age")
    private Integer userAge;

    /**
     * 手机号（AES加密存储）
     */
    @TableField("phone")
    private String phone;

    /**
     * 方言类型（zh/cantonese/sichuan）
     */
    @TableField("dialect_type")
    private String dialectType;

    /**
     * 头像URL
     */
    @TableField("avatar_url")
    private String avatarUrl;

    /**
     * 注册时间（时间戳）
     */
    @TableField("create_time")
    private Long createTime;

    /**
     * 更新时间
     */
    @TableField("update_time")
    private Long updateTime;
}
