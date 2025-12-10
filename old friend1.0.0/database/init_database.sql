-- =====================================================
-- 老友助手小程序 - 数据库初始化脚本
-- 数据库类型: MySQL 8.0+
-- 创建时间: 2025-11-27
-- =====================================================

-- 创建数据库
CREATE DATABASE IF NOT EXISTS elderly_assistant DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE elderly_assistant;

-- =====================================================
-- 1. 用户相关表
-- =====================================================

-- 用户基础表
DROP TABLE IF EXISTS USER_BASE;
CREATE TABLE USER_BASE (
    user_id VARCHAR(50) PRIMARY KEY COMMENT '用户ID',
    user_name VARCHAR(50) NOT NULL COMMENT '姓名',
    user_age INT DEFAULT 0 COMMENT '年龄',
    phone VARCHAR(200) NOT NULL COMMENT '手机号（AES加密存储）',
    dialect_type VARCHAR(20) DEFAULT 'zh' COMMENT '方言类型（zh/cantonese/sichuan）',
    avatar_url VARCHAR(200) COMMENT '头像URL',
    create_time BIGINT NOT NULL COMMENT '注册时间（时间戳）',
    update_time BIGINT DEFAULT 0 COMMENT '更新时间',
    INDEX idx_phone(phone(50)),
    INDEX idx_create_time(create_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户基础表';

-- 用户家属表
DROP TABLE IF EXISTS USER_FAMILY;
CREATE TABLE USER_FAMILY (
    family_id VARCHAR(50) PRIMARY KEY COMMENT '家属ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    family_name VARCHAR(50) NOT NULL COMMENT '家属姓名',
    family_phone VARCHAR(200) NOT NULL COMMENT '家属手机号（AES加密）',
    relation VARCHAR(20) NOT NULL COMMENT '关系（儿子/女儿/配偶等）',
    is_default_pay BOOLEAN DEFAULT FALSE COMMENT '是否默认代缴人',
    is_primary BOOLEAN DEFAULT FALSE COMMENT '是否主联系人',
    priority INT DEFAULT 5 COMMENT '优先级（1最高）',
    create_time BIGINT NOT NULL COMMENT '添加时间',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id),
    INDEX idx_priority(priority)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户家属表';

-- 用户地址表
DROP TABLE IF EXISTS USER_ADDRESS;
CREATE TABLE USER_ADDRESS (
    address_id VARCHAR(50) PRIMARY KEY COMMENT '地址ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    address_name VARCHAR(50) NOT NULL COMMENT '地址名称（家/公司/医院）',
    province VARCHAR(20) COMMENT '省',
    city VARCHAR(20) COMMENT '市',
    detail_address VARCHAR(200) NOT NULL COMMENT '详细地址',
    longitude DOUBLE COMMENT '经度',
    latitude DOUBLE COMMENT '纬度',
    is_default BOOLEAN DEFAULT FALSE COMMENT '是否默认地址',
    create_time BIGINT NOT NULL COMMENT '创建时间',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id),
    INDEX idx_is_default(is_default)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户地址表';

-- 用户设置表
DROP TABLE IF EXISTS USER_SETTINGS;
CREATE TABLE USER_SETTINGS (
    setting_id VARCHAR(50) PRIMARY KEY COMMENT '设置ID',
    user_id VARCHAR(50) NOT NULL UNIQUE COMMENT '用户ID',
    font_size VARCHAR(20) DEFAULT 'extra_large' COMMENT '字体大小（standard/large/extra_large）',
    voice_volume INT DEFAULT 80 COMMENT '语音音量（0-100）',
    dialect_type VARCHAR(20) DEFAULT 'zh' COMMENT '方言类型',
    voice_profile VARCHAR(200) COMMENT '亲人音色URL',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户设置表';

-- =====================================================
-- 2. 缴费相关表
-- =====================================================

-- 缴费配置表
DROP TABLE IF EXISTS PAYMENT_CONFIG;
CREATE TABLE PAYMENT_CONFIG (
    config_id VARCHAR(50) PRIMARY KEY COMMENT '配置ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    payment_type VARCHAR(20) NOT NULL COMMENT '缴费类型（水费/电费/网费/话费）',
    account_number VARCHAR(200) NOT NULL COMMENT '缴费账号（AES加密）',
    family_id VARCHAR(50) COMMENT '代缴家属ID',
    amount DECIMAL(10,2) DEFAULT 0 COMMENT '待缴金额',
    due_date VARCHAR(20) COMMENT '缴费截止日期（yyyy-MM-dd）',
    status VARCHAR(20) DEFAULT 'pending' COMMENT '状态（pending/paid）',
    create_time BIGINT NOT NULL COMMENT '创建时间',
    update_time BIGINT DEFAULT 0 COMMENT '更新时间',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    FOREIGN KEY (family_id) REFERENCES USER_FAMILY(family_id) ON DELETE SET NULL,
    INDEX idx_user_id(user_id),
    INDEX idx_status(status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='缴费配置表';

-- 缴费记录表
DROP TABLE IF EXISTS PAYMENT_RECORD;
CREATE TABLE PAYMENT_RECORD (
    record_id VARCHAR(50) PRIMARY KEY COMMENT '记录ID',
    config_id VARCHAR(50) NOT NULL COMMENT '配置ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    payment_type VARCHAR(20) NOT NULL COMMENT '缴费类型',
    payment_amount DECIMAL(10,2) NOT NULL COMMENT '缴费金额',
    payment_time BIGINT NOT NULL COMMENT '缴费时间',
    payment_status VARCHAR(20) NOT NULL COMMENT '支付状态（success/failed/pending）',
    trade_no VARCHAR(100) COMMENT '微信交易号',
    FOREIGN KEY (config_id) REFERENCES PAYMENT_CONFIG(config_id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id),
    INDEX idx_payment_time(payment_time),
    INDEX idx_trade_no(trade_no)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='缴费记录表';

-- =====================================================
-- 3. 打车相关表
-- =====================================================

-- 常用地址表
DROP TABLE IF EXISTS TAXI_COMMON_ADDRESS;
CREATE TABLE TAXI_COMMON_ADDRESS (
    addr_id VARCHAR(50) PRIMARY KEY COMMENT '地址ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    display_name VARCHAR(50) NOT NULL COMMENT '显示名称',
    address VARCHAR(200) NOT NULL COMMENT '详细地址',
    longitude DOUBLE NOT NULL COMMENT '经度',
    latitude DOUBLE NOT NULL COMMENT '纬度',
    tag VARCHAR(20) COMMENT '标签（HOME/HOSPITAL/RELATIVE/SHOPPING/OTHER）',
    is_default BOOLEAN DEFAULT FALSE COMMENT '是否默认地址',
    priority INT DEFAULT 5 COMMENT '优先级',
    note VARCHAR(100) COMMENT '备注',
    last_use_time BIGINT DEFAULT 0 COMMENT '最后使用时间',
    create_time BIGINT NOT NULL COMMENT '创建时间',
    update_time BIGINT DEFAULT 0 COMMENT '更新时间',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id),
    INDEX idx_is_default(is_default),
    INDEX idx_priority(priority)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='常用地址表';

-- 打车订单表
DROP TABLE IF EXISTS TAXI_ORDER;
CREATE TABLE TAXI_ORDER (
    order_id VARCHAR(50) PRIMARY KEY COMMENT '订单ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    driver_id VARCHAR(50) COMMENT '司机ID',
    start_address VARCHAR(200) NOT NULL COMMENT '起点地址',
    start_longitude DOUBLE NOT NULL COMMENT '起点经度',
    start_latitude DOUBLE NOT NULL COMMENT '起点纬度',
    end_address VARCHAR(200) NOT NULL COMMENT '终点地址',
    end_longitude DOUBLE NOT NULL COMMENT '终点经度',
    end_latitude DOUBLE NOT NULL COMMENT '终点纬度',
    start_time VARCHAR(30) COMMENT '出发时间',
    status VARCHAR(30) DEFAULT 'PENDING_DISPATCH' COMMENT '订单状态',
    distance DOUBLE DEFAULT 0 COMMENT '行驶距离（km）',
    duration INT DEFAULT 0 COMMENT '行驶时长（分钟）',
    base_fee DECIMAL(10,2) DEFAULT 0 COMMENT '起步价',
    distance_fee DECIMAL(10,2) DEFAULT 0 COMMENT '里程费',
    time_fee DECIMAL(10,2) DEFAULT 0 COMMENT '时长费',
    extra_fee DECIMAL(10,2) DEFAULT 0 COMMENT '附加费',
    discount_fee DECIMAL(10,2) DEFAULT 0 COMMENT '优惠金额',
    total_fee DECIMAL(10,2) DEFAULT 0 COMMENT '总费用',
    pay_status VARCHAR(20) DEFAULT '未支付' COMMENT '支付状态',
    elderly_note VARCHAR(200) COMMENT '老年人备注',
    create_time BIGINT NOT NULL COMMENT '创建时间',
    update_time BIGINT DEFAULT 0 COMMENT '更新时间',
    expire_time BIGINT COMMENT '过期时间',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id),
    INDEX idx_status(status),
    INDEX idx_create_time(create_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='打车订单表';

-- =====================================================
-- 4. 医院挂号相关表
-- =====================================================

-- 医院信息表
DROP TABLE IF EXISTS HOSPITAL_INFO;
CREATE TABLE HOSPITAL_INFO (
    hospital_id VARCHAR(50) PRIMARY KEY COMMENT '医院ID',
    hospital_name VARCHAR(100) NOT NULL COMMENT '医院名称',
    hospital_level VARCHAR(20) COMMENT '医院等级',
    hospital_type VARCHAR(20) COMMENT '医院类型',
    address VARCHAR(200) NOT NULL COMMENT '地址',
    phone VARCHAR(20) COMMENT '电话',
    emergency_phone VARCHAR(20) COMMENT '急诊电话',
    longitude DOUBLE NOT NULL COMMENT '经度',
    latitude DOUBLE NOT NULL COMMENT '纬度',
    departments TEXT COMMENT '科室列表（JSON数组）',
    daily_quota INT DEFAULT 100 COMMENT '每日总配额',
    available_quota INT DEFAULT 100 COMMENT '剩余配额',
    opening_hours VARCHAR(100) COMMENT '门诊时间',
    status VARCHAR(20) DEFAULT 'enabled' COMMENT '服务状态',
    INDEX idx_status(status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='医院信息表';

-- 预约挂号表
DROP TABLE IF EXISTS RESERVE_ORDER;
CREATE TABLE RESERVE_ORDER (
    order_id VARCHAR(50) PRIMARY KEY COMMENT '订单ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    hospital_id VARCHAR(50) NOT NULL COMMENT '医院ID',
    hospital_name VARCHAR(100) NOT NULL COMMENT '医院名称',
    department VARCHAR(50) NOT NULL COMMENT '科室',
    reserve_date VARCHAR(20) NOT NULL COMMENT '预约日期（yyyy-MM-dd）',
    status VARCHAR(20) DEFAULT '已预约' COMMENT '状态',
    create_time BIGINT NOT NULL COMMENT '创建时间',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    FOREIGN KEY (hospital_id) REFERENCES HOSPITAL_INFO(hospital_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id),
    INDEX idx_hospital_id(hospital_id),
    INDEX idx_reserve_date(reserve_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='预约挂号表';

-- =====================================================
-- 5. 健康监控相关表
-- =====================================================

-- 设备绑定表
DROP TABLE IF EXISTS DEVICE_BIND;
CREATE TABLE DEVICE_BIND (
    bind_id VARCHAR(50) PRIMARY KEY COMMENT '绑定ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    device_type VARCHAR(20) NOT NULL COMMENT '设备类型（watch/band）',
    device_brand VARCHAR(50) COMMENT '设备品牌',
    device_sn VARCHAR(50) NOT NULL UNIQUE COMMENT '设备序列号',
    bind_time BIGINT NOT NULL COMMENT '绑定时间',
    last_sync_time BIGINT DEFAULT 0 COMMENT '最后同步时间',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id),
    INDEX idx_device_sn(device_sn)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='设备绑定表';

-- 健康日志表
DROP TABLE IF EXISTS HEALTH_LOG;
CREATE TABLE HEALTH_LOG (
    log_id VARCHAR(50) PRIMARY KEY COMMENT '日志ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    device_sn VARCHAR(50) NOT NULL COMMENT '设备序列号',
    heart_rate INT DEFAULT 0 COMMENT '心率（次/分）',
    blood_pressure VARCHAR(20) COMMENT '血压（120/80）',
    step_count INT DEFAULT 0 COMMENT '步数',
    log_type VARCHAR(20) DEFAULT 'auto' COMMENT '日志类型（auto/manual）',
    log_time BIGINT NOT NULL COMMENT '记录时间',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id),
    INDEX idx_log_time(log_time),
    INDEX idx_device_sn(device_sn)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='健康日志表';

-- =====================================================
-- 6. 紧急呼叫相关表
-- =====================================================

-- 呼叫日志表
DROP TABLE IF EXISTS CALL_LOG;
CREATE TABLE CALL_LOG (
    log_id VARCHAR(50) PRIMARY KEY COMMENT '日志ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    callee_type VARCHAR(20) NOT NULL COMMENT '呼叫对象类型（family/120/110）',
    callee_name VARCHAR(50) NOT NULL COMMENT '呼叫对象姓名',
    callee_phone VARCHAR(20) NOT NULL COMMENT '呼叫电话',
    call_status VARCHAR(20) NOT NULL COMMENT '呼叫状态（initiated/connected/failed/cancelled）',
    call_time BIGINT NOT NULL COMMENT '呼叫时间',
    call_duration INT DEFAULT 0 COMMENT '通话时长（秒）',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id),
    INDEX idx_call_time(call_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='呼叫日志表';

-- =====================================================
-- 7. 智能体相关表
-- =====================================================

-- 智能体操作日志表
DROP TABLE IF EXISTS AGENT_LOG;
CREATE TABLE AGENT_LOG (
    log_id VARCHAR(50) PRIMARY KEY COMMENT '日志ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    user_query TEXT NOT NULL COMMENT '用户查询',
    agent_response TEXT NOT NULL COMMENT '智能体回复',
    intent_type VARCHAR(20) COMMENT '意图类型（taxi/payment/chat等）',
    confidence DOUBLE DEFAULT 0 COMMENT '置信度',
    interaction_time BIGINT NOT NULL COMMENT '交互时间',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id),
    INDEX idx_interaction_time(interaction_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='智能体操作日志表';

-- 聊天会话表
DROP TABLE IF EXISTS CHAT_SESSION;
CREATE TABLE CHAT_SESSION (
    session_id VARCHAR(50) PRIMARY KEY COMMENT '会话ID',
    user_id VARCHAR(50) NOT NULL COMMENT '用户ID',
    messages TEXT COMMENT '历史消息（JSON数组）',
    voice_profile VARCHAR(200) COMMENT '音色配置',
    create_time BIGINT NOT NULL COMMENT '创建时间',
    last_active_time BIGINT NOT NULL COMMENT '最后活跃时间',
    expire_time BIGINT NOT NULL COMMENT '过期时间',
    FOREIGN KEY (user_id) REFERENCES USER_BASE(user_id) ON DELETE CASCADE,
    INDEX idx_user_id(user_id),
    INDEX idx_expire_time(expire_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='聊天会话表';

-- =====================================================
-- 8. 插入示例数据
-- =====================================================

-- 插入测试用户
INSERT INTO USER_BASE (user_id, user_name, user_age, phone, dialect_type, create_time) VALUES
('USER_102301524', '张大爷', 68, 'ENCRYPTED_13812345678', 'zh', UNIX_TIMESTAMP() * 1000),
('USER_102301525', '李奶奶', 72, 'ENCRYPTED_13987654321', 'zh', UNIX_TIMESTAMP() * 1000);

-- 插入家属信息
INSERT INTO USER_FAMILY (family_id, user_id, family_name, family_phone, relation, is_default_pay, is_primary, priority, create_time) VALUES
('FAMILY_001', 'USER_102301524', '张明', 'ENCRYPTED_13611112222', '儿子', TRUE, TRUE, 1, UNIX_TIMESTAMP() * 1000),
('FAMILY_002', 'USER_102301524', '张丽', 'ENCRYPTED_13622223333', '女儿', FALSE, FALSE, 2, UNIX_TIMESTAMP() * 1000);

-- 插入医院信息
INSERT INTO HOSPITAL_INFO (hospital_id, hospital_name, hospital_level, hospital_type, address, phone, longitude, latitude, departments, daily_quota, available_quota, opening_hours, status) VALUES
('H001', '市第一人民医院', '三级甲等', '综合医院', '市中心区人民路123号', '0571-88888888', 120.15, 30.28, '["内科","外科","骨科","眼科"]', 100, 95, '周一至周五 8:00-17:00', 'enabled'),
('H002', '中医院', '三级乙等', '中医医院', '市西湖区西湖路456号', '0571-99999999', 120.12, 30.25, '["内科","针灸科","康复科"]', 80, 75, '周一至周六 8:00-17:00', 'enabled');

-- 插入常用地址
INSERT INTO TAXI_COMMON_ADDRESS (addr_id, user_id, display_name, address, longitude, latitude, tag, is_default, priority, create_time) VALUES
('ADDR_001', 'USER_102301524', '我家', '杭州市西湖区文一路XXX号', 120.13, 30.27, 'HOME', TRUE, 1, UNIX_TIMESTAMP() * 1000),
('ADDR_002', 'USER_102301524', '市第一人民医院', '市中心区人民路123号', 120.15, 30.28, 'HOSPITAL', FALSE, 2, UNIX_TIMESTAMP() * 1000);

-- 插入缴费配置
INSERT INTO PAYMENT_CONFIG (config_id, user_id, payment_type, account_number, amount, due_date, status, create_time) VALUES
('PAY_001', 'USER_102301524', '电费', 'ENCRYPTED_330100123456', 126.30, '2025-12-05', 'pending', UNIX_TIMESTAMP() * 1000),
('PAY_002', 'USER_102301524', '水费', 'ENCRYPTED_330100654321', 85.60, '2025-12-10', 'pending', UNIX_TIMESTAMP() * 1000);

COMMIT;

-- =====================================================
-- 说明：
-- 1. 所有时间字段统一使用BIGINT存储时间戳（毫秒）
-- 2. 敏感字段（手机号、账号）使用AES加密，示例数据用ENCRYPTED_前缀标识
-- 3. 所有表都有索引优化查询性能
-- 4. 使用外键确保数据一致性
-- 5. 支持级联删除（ON DELETE CASCADE）
-- =====================================================
