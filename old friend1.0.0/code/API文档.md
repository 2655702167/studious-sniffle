# 语音支付API接口文档

---

## 接口概览

| 接口名称 | 方法 | 路径 | 说明 |
|---------|------|------|------|
| 语音支付 | POST | /api/payment/voice-pay | 处理用户语音支付请求 |
| 语音查询待缴费 | POST | /api/payment/voice-query-unpaid | 语音查询用户待缴费项目 |

---

## 1. 语音支付接口

### 基本信息

- **接口地址**: `/api/payment/voice-pay`
- **请求方法**: `POST`
- **Content-Type**: `application/json`
- **认证方式**: 用户Token（Header: Authorization）

### 请求参数

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | string | 是 | 用户唯一标识 |
| audio_data | string | 是 | base64编码的音频数据 |
| session_id | string | 否 | 会话ID（多轮对话时传入） |
| audio_format | string | 否 | 音频格式（默认wav，支持mp3/pcm） |
| sample_rate | int | 否 | 采样率（默认16000） |

### 请求示例

```json
{
  "user_id": "USER_102301524",
  "audio_data": "//uQZAAAAAAAAAAAAAAAAAAAAAAAWGluZwAAAA8AAAACAAAFsABVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV...",
  "session_id": "",
  "audio_format": "mp3",
  "sample_rate": 16000
}
```

### 响应参数

| 参数名 | 类型 | 说明 |
|--------|------|------|
| success | boolean | 请求是否成功 |
| reply_text | string | 语音回复文本（供TTS播报） |
| session_id | string | 会话ID（多轮对话用） |
| next_action | string | 下一步操作（continue/complete/error） |
| payment_order | object | 支付订单信息（支付成功时返回） |
| recognition_text | string | 识别出的原始文本 |
| confidence | float | 识别置信度（0-1） |

### 响应示例

#### 场景1：首次交互（未指定缴费类型）

```json
{
  "success": true,
  "reply_text": "我可以帮您缴纳水费、电费、网费、话费。请问您要缴哪一项？",
  "session_id": "VOICE_PAY_SESSION1732435678123",
  "next_action": "continue",
  "recognition_text": "我要缴费",
  "confidence": 0.96
}
```

#### 场景2：指定缴费类型（等待确认）

```json
{
  "success": true,
  "reply_text": "您要缴纳水费，金额85.60元。请说"确认"继续支付，或说"取消"放弃",
  "session_id": "VOICE_PAY_SESSION1732435678123",
  "next_action": "continue",
  "recognition_text": "我要缴水费",
  "confidence": 0.98
}
```

#### 场景3：确认支付（支付成功）

```json
{
  "success": true,
  "reply_text": "已为您发起水费支付，金额85.60元，请在微信中完成支付",
  "session_id": "VOICE_PAY_SESSION1732435678123",
  "next_action": "complete",
  "payment_order": {
    "order_id": "PAY17324356781234567",
    "out_trade_no": "2024112417324356781234567",
    "item_type": "水费",
    "amount": 85.60,
    "pay_type": "wechat",
    "status": "未支付",
    "create_time": "2024-11-24 16:47:58",
    "expire_time": "2024-11-24 16:52:58",
    "pay_params": {
      "appid": "wx1234567890abcdef",
      "prepay_id": "wx241124164758123456789",
      "time_stamp": "1732435678",
      "nonce_str": "a1b2c3d4",
      "sign_type": "MD5",
      "pay_sign": "9A0A8659F005D6984697E2CA0A9CF3B7"
    }
  },
  "recognition_text": "确认",
  "confidence": 0.99
}
```

#### 场景4：识别失败

```json
{
  "success": false,
  "reply_text": "抱歉，没有听清您说的话，请再说一遍",
  "session_id": "",
  "next_action": "error",
  "recognition_text": "",
  "confidence": 0.32
}
```

### 错误码说明

| 错误码 | 说明 | 处理建议 |
|--------|------|---------|
| 40001 | 用户ID无效 | 检查user_id参数 |
| 40002 | 音频数据为空 | 检查audio_data参数 |
| 40003 | 音频格式不支持 | 使用wav/mp3/pcm格式 |
| 40004 | 会话已过期 | 重新发起支付 |
| 50001 | 语音识别服务异常 | 稍后重试 |
| 50002 | 缴费项目不存在 | 确认用户有待缴费项目 |
| 50003 | 支付订单创建失败 | 检查后端服务状态 |

---

## 2. 语音查询待缴费接口

### 基本信息

- **接口地址**: `/api/payment/voice-query-unpaid`
- **请求方法**: `POST`
- **Content-Type**: `application/json`
- **认证方式**: 用户Token

### 请求参数

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | string | 是 | 用户唯一标识 |

### 请求示例

```json
{
  "user_id": "USER_102301524"
}
```

### 响应参数

| 参数名 | 类型 | 说明 |
|--------|------|------|
| success | boolean | 请求是否成功 |
| reply_text | string | 语音回复文本 |
| unpaid_items | array | 待缴费项目列表 |

### 响应示例

#### 场景1：有待缴费项目

```json
{
  "success": true,
  "reply_text": "您有3项待缴费用：1. 水费，金额85.60元；2. 电费，金额126.30元；3. 网费，金额99.00元；请问您要缴哪一项？",
  "unpaid_items": [
    {
      "item_id": "PAY_ITEM1732435000001",
      "item_type": "水费",
      "amount": 85.60,
      "due_date": "2024-12-10"
    },
    {
      "item_id": "PAY_ITEM1732435000002",
      "item_type": "电费",
      "amount": 126.30,
      "due_date": "2024-12-15"
    },
    {
      "item_id": "PAY_ITEM1732435000003",
      "item_type": "网费",
      "amount": 99.00,
      "due_date": "2024-12-20"
    }
  ]
}
```

#### 场景2：无待缴费项目

```json
{
  "success": true,
  "reply_text": "您当前没有待缴费用，真棒！",
  "unpaid_items": []
}
```

---

## 调用示例

### cURL 示例

```bash
# 语音支付
curl -X POST https://your-api.com/api/payment/voice-pay \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "user_id": "USER_102301524",
    "audio_data": "base64_encoded_audio_data",
    "session_id": ""
  }'

# 语音查询
curl -X POST https://your-api.com/api/payment/voice-query-unpaid \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "user_id": "USER_102301524"
  }'
```

### JavaScript 示例（微信小程序）

```javascript
// 语音支付
wx.request({
  url: 'https://your-api.com/api/payment/voice-pay',
  method: 'POST',
  header: {
    'Content-Type': 'application/json',
    'Authorization': 'Bearer ' + wx.getStorageSync('token')
  },
  data: {
    user_id: wx.getStorageSync('userId'),
    audio_data: audioBase64,
    session_id: this.data.sessionId
  },
  success: (res) => {
    console.log('语音支付成功', res.data);
    // 处理响应
    this.handleVoicePaymentResponse(res.data);
  },
  fail: (err) => {
    console.error('语音支付失败', err);
  }
});
```

### Python 示例

```python
import requests
import base64

# 读取音频文件
with open('audio.mp3', 'rb') as f:
    audio_data = base64.b64encode(f.read()).decode('utf-8')

# 发送请求
response = requests.post(
    'https://your-api.com/api/payment/voice-pay',
    headers={
        'Content-Type': 'application/json',
        'Authorization': 'Bearer YOUR_TOKEN'
    },
    json={
        'user_id': 'USER_102301524',
        'audio_data': audio_data,
        'session_id': ''
    }
)

result = response.json()
print('语音回复:', result['reply_text'])
```

---

## 性能指标

| 指标 | 目标值 | 说明 |
|------|--------|------|
| 响应时间 | < 2秒 | 包含语音识别+业务处理 |
| 识别准确率 | > 90% | 普通话场景 |
| 方言识别准确率 | > 85% | 粤语、四川话等 |
| 并发处理能力 | > 1000 QPS | 高峰期 |
| 可用性 | 99.9% | 年均停机时间 < 8.76小时 |

---

## 注意事项

1. **音频格式要求**：
   - 支持格式：wav、mp3、pcm
   - 采样率：8000Hz 或 16000Hz（推荐）
   - 声道数：单声道
   - 最大时长：60秒

2. **会话管理**：
   - 会话有效期：3分钟
   - 过期后需重新发起
   - 会话ID由后端生成，前端需保存

3. **安全建议**：
   - 使用HTTPS加密传输
   - 音频数据脱敏处理
   - 支付前二次确认

4. **错误重试**：
   - 识别失败自动重试1次
   - 网络异常最多重试3次
   - 指数退避策略

---

## 更新日志

### v1.0 (2024-11-24)
- ✅ 初始版本发布
- ✅ 支持语音支付核心功能
- ✅ 支持多轮对话
- ✅ 支持方言识别

---
