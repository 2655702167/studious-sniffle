# 老友助手项目运行指南

本项目包括：

- Java 后端：`java-backend`（Spring Boot + MySQL）
- 微信小程序前端：`minicode-1`
- 语音识别：对接百度语音识别 API（短语音标准版）

---

## 一、需要安装的软件与账号

### 1. 必备软件

1. **JDK 17**
   - 下载地址：<https://www.oracle.com/java/technologies/downloads/>
   - 安装后在命令行输入 `java -version`，确认输出为 17 开头。

2. **Maven 3.8+**
   - 下载地址：<https://maven.apache.org/download.cgi>
   - 解压后配置环境变量 `MAVEN_HOME`，并把 `bin` 加入 `Path`。
   - 命令行输入 `mvn -v`，看到版本号说明安装成功。

3. **MySQL 8.x**
   - 下载地址：<https://dev.mysql.com/downloads/>
   - 安装时牢记：
     - 端口：默认 `3306`
     - 用户名：建议使用 `root`
     - 密码：和 `application.yml` 中保持一致（示例为 `Root@123456`）。

4. **微信开发者工具**
   - 下载地址：<https://developers.weixin.qq.com/miniprogram/dev/devtools/download.html>
   - 需要一个微信小程序 AppID（可以在微信公众平台申请测试小程序）。

5. **代码编辑器（任选其一）**
   - VS Code / IntelliJ IDEA / Eclipse 均可。

> 可选：Redis（目前项目中配置了 Redis，但不启用也能启动，若要完整使用会话功能，可安装 Redis 5+）。

---

## 二、后端环境准备（Java + 数据库）

### 1. 导入数据库

1. 启动 MySQL 服务。
2. 使用图形工具（如 Navicat、MySQL Workbench）或命令行执行：
   ```sql
   CREATE DATABASE elderly_assistant DEFAULT CHARSET utf8mb4;
   ```
3. 在项目目录中找到：
   ```text
   database/init_database.sql
   ```
4. 将 `init_database.sql` 整个脚本执行到 `elderly_assistant` 数据库中。
   - 其中会创建 `USER_BASE`、`PAYMENT_CONFIG` 等表并插入初始数据。

### 2. 配置后端 `application.yml`

文件路径：

```text
java-backend/src/main/resources/application.yml
```

需要特别检查和修改的地方：

1. **数据库连接**
   ```yaml
   spring:
     datasource:
       url: jdbc:mysql://localhost:3306/elderly_assistant?useUnicode=true&characterEncoding=utf8&useSSL=false&serverTimezone=Asia/Shanghai&allowPublicKeyRetrieval=true
       username: root
       password: Root@123456   # ← 改成你本机 MySQL 密码
   ```

2. **百度语音识别 Key**

   ```yaml
   api:
     baidu:
       voice:
         app-id: 你的APP_ID
         api-key: 你的API_KEY
         secret-key: 你的SECRET_KEY
   ```

   这三个值从百度智能云控制台复制：
   - 控制台地址：<https://console.bce.baidu.com/>
   - 产品：**语音技术 → 语音识别**
   - 找到已创建的应用，复制 `APP_ID / API_KEY / SECRET_KEY`。

3. **后端端口**

   ```yaml
   server:
     port: 8081
     servlet:
       context-path: /api
   ```

   若你本机 8081 被占用，可以改为其他端口，但要同步修改前端 `config.js` 中的端口号。

---

## 三、构建并启动后端服务

1. 打开命令行（CMD 或 PowerShell），进入后端目录：

   ```bash
   cd "C:\Users\28614\Desktop\作业\软工\old friend\java-backend"
   ```

2. 使用 Maven 打包：

   ```bash
   mvn clean package
   ```

   - 第一次会下载依赖，可能较慢。
   - 若看到 `BUILD SUCCESS` 表示打包成功。

3. 启动 Spring Boot 后端：

   ```bash
   java -jar target\assistant-backend-1.0.0.jar
   ```

4. 启动成功标志：
   - 控制台没有报错，最后类似输出：
     ```text
     Started ElderlyAssistantApplication in xx seconds
     ```
   - 浏览器访问：
     ```text
     http://localhost:8081/api/payment/unpaid-items?user_id=USER_123
     ```
     能看到 JSON 数据即为成功。

> 注意：后端启动窗口不要关闭，小程序访问接口需要这个服务一直运行。

---

## 四、配置并运行微信小程序前端

### 1. 打开项目

1. 启动 **微信开发者工具**。
2. 选择“**小程序**”。
3. AppID：填你自己的小程序 AppID（可以在测试号里创建）。
4. 选择项目目录：

   ```text
   minicode-1
   ```

5. 点击“确定”打开项目。

### 2. 配置前端后端地址 `config.js`

文件路径：

```text
minicode-1/utils/config.js
```

开发调试时（你自己的电脑 + 模拟器）：

```js
// 开发环境配置
const DEV_CONFIG = {
  // 本地或局域网后端地址
  apiBaseUrl: 'http://你的IP:8081/api',
  timeout: 10000,
  useMock: false
};
```

- 如果只在**模拟器**中调试，可以写：
  ```js
  apiBaseUrl: 'http://localhost:8081/api'
  ```
- 如果要在**真机上连你电脑**：
  1. 确保手机和电脑在同一 Wi‑Fi 或手机热点。
  2. 在电脑上运行 `ipconfig`，找到当前网络的 **IPv4 地址**，例如：
     ```text
     IPv4 地址 . . . . . . . . . . . : 192.168.171.233
     ```
  3. 将 `apiBaseUrl` 改为：
     ```js
     apiBaseUrl: 'http://192.168.171.233:8081/api'
     ```

### 3. 小程序权限与调试设置

1. 在 `app.json` 中已配置录音权限：

   ```json
   "permission": {
     "scope.record": {
       "desc": "需要使用您的麦克风进行语音识别"
     }
   }
   ```

2. 在微信开发者工具中：
   - 点击顶部“**详情 → 本地设置**”：
     - 勾选：**不校验合法域名、Web-view（业务域名）、TLS 版本以及 HTTPS 证书**
   - 防止因域名未备案而无法请求本地后端。

3. 在手机端第一次使用麦克风时，点击“允许”。

---

## 五、运行与测试流程

### 1. 在模拟器中测试

1. 保证后端命令行窗口正在运行。
2. 在微信开发者工具点击顶部 **“编译”**。
3. 在小程序中分别测试：
   - 首页语音按钮（点击说话）
   - 生活缴费页面语音识别按钮
   - 打车页面麦克风图标
   - 医院挂号页面搜索框右侧麦克风图标
   - 老友陪聊页面“按住说话”按钮
4. 观察是否能：
   - 录音状态正常切换（显示“录音中...”）
   - 识别成功后弹出结果或填入文本框
   - 后端控制台日志无错误（`err_no: 0` 即为百度识别成功）。

### 2. 在真机上测试（推荐演示用）

1. **手机开热点，电脑连接手机热点。**
2. 在电脑上用 `ipconfig` 查看当前网络的 IPv4 地址（一般是 `192.168.xxx.xxx`），填入 `config.js` 的 `apiBaseUrl`。
3. 在微信开发者工具点击 **“编译”**。
4. 点击 **“预览”**，手机微信扫码二维码打开小程序。
5. 确保手机连接的就是你开的热点，然后重复以上语音测试步骤。

---

## 六、常见问题排查

1. **小程序提示“无法连接服务器 / 请求失败”**
   - 检查：
     - 后端是否启动成功，命令行窗口是否还在。
     - `config.js` 中的 `apiBaseUrl` 是否写对 IP 和端口。
     - 手机和电脑是否在同一网络（同一 Wi‑Fi 或同一热点）。
     - 微信开发者工具是否勾选了“不校验合法域名”。

2. **百度返回 `err_no: 3312, err_msg: param format invalid.`**
   - 确认前端录音格式为 `pcm`，采样率 `16000`。
   - 后端请求参数中 `format` 是否为 `pcm`，`rate` 是否为 `16000`。

3. **百度返回成功，但 `result` 为空数组或 `[""]`**
   - 提醒用户：
     - 说话声音要足够大、清晰。
     - 按住录音至少 2 秒以上。
     - 在相对安静的环境下测试。

4. **真机可以打开小程序，但语音功能不工作**
   - 检查手机是否允许小程序使用“录音权限”。
   - 检查当前连接网络是否正确（是否连到了别的 Wi‑Fi）。

---

## 七、项目结构简要说明

```text
old friend/
├── java-backend/                # 后端 Spring Boot 项目
│   ├── src/main/java/com/elderly/assistant
│   │   ├── controller/          # 控制器（含语音控制器 VoiceController）
│   │   ├── service/             # 业务逻辑（含 VoiceService 调用百度语音）
│   │   └── ElderlyAssistantApplication.java
│   └── src/main/resources/
│       └── application.yml      # 配置文件
│
├── minicode-1/                  # 微信小程序前端
│   ├── pages/                   # 各个页面（首页、生活缴费、陪聊、挂号、打车等）
│   ├── utils/
│   │   ├── config.js            # 前端 API 基础地址配置
│   │   └── voice.js             # 录音与上传后端语音识别
│   └── app.json                 # 小程序全局配置
│
└── database/
    └── init_database.sql        # 初始化数据库脚本
```
