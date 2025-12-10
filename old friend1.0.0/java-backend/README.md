# 老友助手后端服务 - Java版本

**项目名称：** 老友助手小程序后端API服务  
**开发语言：** Java  
**框架版本：** Spring Boot 2.7.14  
**作者：** 施汉霖  
**完成时间：** 2025年11月

---

## 📁 项目结构

```
java-backend/
├── src/main/java/com/elderly/assistant/
│   ├── ElderlyAssistantApplication.java  # 主启动类
│   ├── entity/                   # 实体类（对应数据库表）
│   │   ├── UserBase.java         # 用户基础信息
│   │   ├── PaymentItem.java      # 缴费项目
│   │   └── TaxiOrder.java        # 打车订单
│   ├── mapper/                   # MyBatis Mapper接口（DAO层）
│   │   ├── UserMapper.java
│   │   ├── PaymentMapper.java
│   │   └── TaxiOrderMapper.java
│   ├── service/                  # 服务层（业务逻辑）
│   │   ├── PaymentService.java   # 缴费服务
│   │   └── TaxiService.java      # 打车服务
│   ├── controller/               # 控制器层（API接口）
│   │   ├── PaymentController.java
│   │   └── TaxiController.java
│   └── common/                   # 公共类
│       └── Result.java           # 统一响应结果
│
├── src/main/resources/
│   ├── application.yml           # 应用配置文件
│   └── mapper/                   # MyBatis XML映射文件
│
├── pom.xml                       # Maven项目配置
└── README.md                     # 本文档
```

---

## 🚀 技术栈

| 技术 | 版本 | 说明 |
|------|------|------|
| **Java** | 1.8+ | 编程语言 |
| **Spring Boot** | 2.7.14 | 应用框架 |
| **MyBatis-Plus** | 3.5.3.1 | ORM框架（增强MyBatis） |
| **MySQL** | 8.0+ | 数据库 |
| **Lombok** | 1.18.x | 代码简化工具 |
| **Hutool** | 5.8.20 | Java工具类库 |
| **FastJson2** | 2.0.40 | JSON处理 |
| **Redis** | 6.0+ | 缓存（可选） |

---

## 📋 功能模块

### ✅ 已实现模块

| 模块 | 接口路径 | 说明 | 状态 |
|------|----------|------|------|
| **生活缴费** | `/payment/*` | 水电燃气缴费管理 | ✅ 完成 |
| **语音支付** | `/payment/voice-pay` | 语音识别+支付 | ✅ 完成 |
| **打车服务** | `/taxi/*` | 订单管理+费用计算 | 🔄 框架完成 |
| **用户中心** | `/user/*` | 用户信息管理 | 🔄 框架完成 |

### 🔄 待扩展模块

- 医院挂号（`/hospital/*`）
- 智能陪聊（`/chat/*`）
- 健康监控（`/health/*`）
- 紧急呼叫（`/emergency/*`）

---

## 🔧 快速开始

### 1. 环境准备

**必备环境：**
- JDK 1.8 或以上
- Maven 3.6+
- MySQL 8.0+
- Redis（可选）

### 2. 数据库初始化

```sql
-- 1. 创建数据库
CREATE DATABASE elderly_assistant DEFAULT CHARACTER SET utf8mb4;

-- 2. 执行SQL脚本
source ../database/init_database.sql
```

### 3. 修改配置

编辑 `src/main/resources/application.yml`：

```yaml
spring:
  datasource:
    url: jdbc:mysql://localhost:3306/elderly_assistant
    username: root        # 修改为你的MySQL用户名
    password: 123456      # 修改为你的MySQL密码
```

### 4. 启动项目

**方式一：使用Maven命令**
```bash
cd java-backend
mvn clean install
mvn spring-boot:run
```

**方式二：使用IDE**
1. 用 IntelliJ IDEA 或 Eclipse 打开项目
2. 等待Maven依赖下载完成
3. 运行 `ElderlyAssistantApplication.java` 主类

**启动成功标志：**
```
========================================
老友助手后端服务启动成功！
访问地址: http://localhost:8080/api
========================================
```

### 5. 测试接口

**查询待缴费项目：**
```bash
curl "http://localhost:8080/api/payment/unpaid-items?user_id=USER_123"
```

**期望响应：**
```json
{
  "code": 0,
  "message": "success",
  "data": [
    {
      "itemId": "PAY_ITEM_001",
      "itemType": "电费",
      "amount": 126.30,
      "status": "欠费",
      "dueDate": "2025-12-05"
    }
  ]
}
```

---

## 📖 API接口文档

### 统一响应格式

所有接口返回格式：

```json
{
  "code": 0,           // 0-成功，其他-失败
  "message": "success", // 响应消息
  "data": {}           // 响应数据
}
```

### 缴费模块接口

#### 1. 查询待缴费项目

**接口：** `GET /api/payment/unpaid-items`

**参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |

**响应示例：**
```json
{
  "code": 0,
  "message": "success",
  "data": [
    {
      "itemId": "PAY_ITEM_001",
      "userId": "USER_123",
      "itemType": "电费",
      "amount": 126.30,
      "status": "欠费",
      "dueDate": "2025-12-05"
    }
  ]
}
```

#### 2. 语音支付

**接口：** `POST /api/payment/voice-pay`

**请求体：**
```json
{
  "userId": "USER_123",
  "audioData": "base64_encoded_audio",
  "sessionId": ""
}
```

**响应示例：**
```json
{
  "code": 0,
  "message": "success",
  "data": {
    "sessionId": "SESSION_1732800000",
    "replyText": "您好，请问需要缴纳什么费用？",
    "needTts": true
  }
}
```

---

## 🔍 与C++版本的对应关系

| C++代码 | Java代码 | 说明 |
|---------|----------|------|
| `PaymentItem` 结构体 | `PaymentItem` 实体类 | 缴费项目数据模型 |
| `TaxiOrder` 结构体 | `TaxiOrder` 实体类 | 打车订单数据模型 |
| `PaymentService` 类 | `PaymentService` 类 | 缴费业务逻辑 |
| `PaymentDao` 类 | `PaymentMapper` 接口 | 数据访问层 |
| - | `PaymentController` 类 | RESTful API控制器 |

**主要改进：**
1. ✅ **完整的分层架构**：Entity → Mapper → Service → Controller
2. ✅ **统一响应格式**：使用`Result<T>`统一封装
3. ✅ **注解驱动**：使用Spring注解简化配置
4. ✅ **MyBatis-Plus**：自动生成CRUD方法
5. ✅ **跨域支持**：`@CrossOrigin`支持微信小程序调用

---

## 🛠 开发指南

### 添加新的API接口

**1. 创建实体类**（`entity/`）
```java
@Data
@TableName("YOUR_TABLE")
public class YourEntity {
    @TableId
    private String id;
    private String name;
}
```

**2. 创建Mapper接口**（`mapper/`）
```java
@Mapper
public interface YourMapper extends BaseMapper<YourEntity> {
}
```

**3. 创建Service**（`service/`）
```java
@Service
public class YourService {
    @Autowired
    private YourMapper mapper;
    
    public List<YourEntity> getList() {
        return mapper.selectList(null);
    }
}
```

**4. 创建Controller**（`controller/`）
```java
@RestController
@RequestMapping("/your-module")
@CrossOrigin
public class YourController {
    @Autowired
    private YourService service;
    
    @GetMapping("/list")
    public Result<?> getList() {
        return Result.success(service.getList());
    }
}
```

---

## ⚠️ 常见问题

### 1. 数据库连接失败

**错误：** `CommunicationsException: Communications link failure`

**解决：**
- 检查MySQL服务是否启动
- 确认配置文件中的用户名密码正确
- 检查MySQL端口是否为3306

### 2. 端口已被占用

**错误：** `Port 8080 was already in use`

**解决：**
修改`application.yml`中的端口：
```yaml
server:
  port: 8081  # 改为其他端口
```

### 3. MyBatis找不到Mapper

**错误：** `Invalid bound statement (not found)`

**解决：**
- 确认`@MapperScan`注解路径正确
- 检查Mapper接口和XML文件的namespace匹配

---

## 📝 后续开发建议

### 短期优化（1-2周）
- [ ] 完善其他模块的Controller和Service
- [ ] 添加参数校验（@Valid注解）
- [ ] 集成Swagger生成API文档
- [ ] 添加全局异常处理器

### 中期优化（1个月）
- [ ] 接入百度语音识别API
- [ ] 接入文心一言AI对话
- [ ] 实现Redis会话管理
- [ ] 添加JWT身份认证

### 长期优化（2-3个月）
- [ ] 对接微信支付API
- [ ] 实现数据加密存储
- [ ] 性能优化和压力测试
- [ ] Docker容器化部署

---

## 📞 联系方式

**开发者：** 施汉霖  
**学号：** 102301524  

---

## 📜 总结

本Java后端项目完整实现了：

✅ **标准的Spring Boot项目结构**  
✅ **MyBatis-Plus数据访问层**  
✅ **完整的MVC三层架构**  
✅ **统一的RESTful API规范**  
✅ **完整的缴费模块示例**  

相比C++版本，Java版本更易于：
- 🚀 **快速开发**：注解驱动，减少样板代码
- 🔧 **易于维护**：清晰的分层架构
- 🌐 **跨平台部署**：一次编写，到处运行
- 📚 **丰富的生态**：大量开源框架支持

**祝开发顺利！** 🎉
