# 老友助手小程序 - 前端API调用指南

## 📝 接口调用说明

### 1. 统一配置

在 `config.js` 中配置后端API地址：

```javascript
// config/config.js
const config = {
  // 后端API基础地址（根据实际部署修改）
  apiBaseUrl: 'https://your-backend-server.com/api',
  
  // 微信云开发环境ID（如果使用云开发）
  cloudEnvId: 'your-cloud-env-id',
  
  // 超时配置
  timeout: 10000
}

module.exports = config
```

### 2. 统一请求封装

创建 `utils/request.js`：

```javascript
// utils/request.js
const config = require('../config/config.js')

/**
 * 统一API请求方法
 */
function request(options) {
  return new Promise((resolve, reject) => {
    wx.request({
      url: config.apiBaseUrl + options.url,
      method: options.method || 'GET',
      data: options.data || {},
      header: {
        'Content-Type': 'application/json',
        ...options.header
      },
      timeout: options.timeout || config.timeout,
      success: (res) => {
        if (res.statusCode === 200) {
          if (res.data.code === 0) {
            resolve(res.data.data)
          } else {
            wx.showToast({
              title: res.data.message || '请求失败',
              icon: 'none'
            })
            reject(res.data)
          }
        } else {
          wx.showToast({
            title: '网络错误',
            icon: 'none'
          })
          reject(res)
        }
      },
      fail: (err) => {
        wx.showToast({
          title: '网络请求失败',
          icon: 'none'
        })
        reject(err)
      }
    })
  })
}

module.exports = { request }
```

---

## 🚀 核心功能API调用示例

### 一、语音支付模块

#### 1. 发起语音支付

```javascript
// pages/Living_payment/Living_payment.js
const { request } = require('../../utils/request.js')

Page({
  // 录音完成后发起支付
  async onVoicePayment() {
    const recorderManager = wx.getRecorderManager()
    
    // 开始录音
    recorderManager.start({
      duration: 60000,
      sampleRate: 16000,
      numberOfChannels: 1,
      encodeBitRate: 48000,
      format: 'mp3'
    })
    
    // 停止录音后上传
    recorderManager.onStop(async (res) => {
      const tempFilePath = res.tempFilePath
      
      // 将音频转为base64
      const fileManager = wx.getFileSystemManager()
      const audioData = fileManager.readFileSync(tempFilePath, 'base64')
      
      try {
        // 调用语音支付API
        const result = await request({
          url: '/payment/voice-pay',
          method: 'POST',
          data: {
            user_id: this.data.userId,
            audio_data: audioData,
            session_id: this.data.sessionId || ''
          }
        })
        
        // 保存会话ID
        this.setData({
          sessionId: result.session_id,
          replyText: result.reply_text
        })
        
        // 语音播报回复
        this.playVoice(result.reply_text)
        
        // 如果需要支付，调起微信支付
        if (result.payment_order) {
          this.wxPay(result.payment_order)
        }
      } catch (err) {
        console.error('语音支付失败:', err)
      }
    })
  },
  
  // 播放语音回复（TTS）
  playVoice(text) {
    const innerAudioContext = wx.createInnerAudioContext()
    innerAudioContext.src = `https://tts-api.com/synthesize?text=${encodeURIComponent(text)}`
    innerAudioContext.play()
  },
  
  // 调起微信支付
  wxPay(orderInfo) {
    wx.requestPayment({
      timeStamp: orderInfo.timeStamp,
      nonceStr: orderInfo.nonceStr,
      package: orderInfo.package,
      signType: 'MD5',
      paySign: orderInfo.paySign,
      success: () => {
        wx.showToast({ title: '支付成功', icon: 'success' })
      },
      fail: () => {
        wx.showToast({ title: '支付取消', icon: 'none' })
      }
    })
  }
})
```

#### 2. 查询待缴费项目

```javascript
async queryUnpaidItems() {
  try {
    const items = await request({
      url: '/payment/unpaid-items',
      method: 'GET',
      data: {
        user_id: this.data.userId
      }
    })
    
    this.setData({ paymentItems: items })
  } catch (err) {
    console.error('查询失败:', err)
  }
}
```

---

### 二、智能陪聊模块

#### 1. 发送聊天消息

```javascript
// pages/Olds_chatting/Olds_chatting.js
async sendMessage(message) {
  try {
    const result = await request({
      url: '/chat/message',
      method: 'POST',
      data: {
        user_id: this.data.userId,
        message: message,
        session_id: this.data.sessionId || ''
      }
    })
    
    // 更新会话ID
    this.setData({
      sessionId: result.session_id
    })
    
    // 添加AI回复到消息列表
    this.data.messages.push({
      role: 'assistant',
      content: result.reply_text,
      time: new Date().toLocaleTimeString()
    })
    
    this.setData({ messages: this.data.messages })
    
    // 语音播报（如果需要）
    if (result.need_tts) {
      this.playVoice(result.reply_text)
    }
  } catch (err) {
    console.error('发送消息失败:', err)
  }
}
```

#### 2. 语音意图识别

```javascript
async parseVoiceIntent(text) {
  try {
    const intent = await request({
      url: '/chat/parse-intent',
      method: 'POST',
      data: {
        user_id: this.data.userId,
        text: text
      }
    })
    
    // 根据意图类型跳转页面
    if (intent.intent_type === 'taxi') {
      wx.navigateTo({
        url: '/pages/Voice_taxi/Voice_taxi'
      })
    } else if (intent.intent_type === 'payment') {
      wx.navigateTo({
        url: '/pages/Living_payment/Living_payment'
      })
    } else if (intent.intent_type === 'register') {
      wx.navigateTo({
        url: '/pages/Hospital_registration/Hospital_registration'
      })
    }
  } catch (err) {
    console.error('意图识别失败:', err)
  }
}
```

---

### 三、打车模块

#### 1. 创建打车订单

```javascript
// pages/Voice_taxi/Voice_taxi.js
async createTaxiOrder(startLocation, endLocation) {
  try {
    const order = await request({
      url: '/taxi/create-order',
      method: 'POST',
      data: {
        user_id: this.data.userId,
        start_type: 'manual',
        start_location: {
          address: startLocation.address,
          longitude: startLocation.longitude,
          latitude: startLocation.latitude
        },
        end_type: 'manual',
        end_location: {
          address: endLocation.address,
          longitude: endLocation.longitude,
          latitude: endLocation.latitude
        }
      }
    })
    
    this.setData({ orderInfo: order })
    wx.showToast({ title: '订单创建成功', icon: 'success' })
  } catch (err) {
    console.error('创建订单失败:', err)
  }
}
```

#### 2. 查询常用地址

```javascript
async queryCommonAddresses() {
  try {
    const addresses = await request({
      url: '/taxi/common-addresses',
      method: 'GET',
      data: {
        user_id: this.data.userId
      }
    })
    
    this.setData({ commonAddresses: addresses })
  } catch (err) {
    console.error('查询常用地址失败:', err)
  }
}
```

---

### 四、医院挂号模块

#### 1. 查询附近医院

```javascript
// pages/Hospital_registration/Hospital_registration.js
async queryNearbyHospitals(department) {
  // 获取当前位置
  wx.getLocation({
    type: 'gcj02',
    success: async (res) => {
      try {
        const hospitals = await request({
          url: '/hospital/nearby',
          method: 'GET',
          data: {
            department: department,
            latitude: res.latitude,
            longitude: res.longitude
          }
        })
        
        this.setData({ hospitals: hospitals })
      } catch (err) {
        console.error('查询医院失败:', err)
      }
    }
  })
}
```

#### 2. 创建预约订单

```javascript
async createReserveOrder(hospitalId, department, date) {
  try {
    const order = await request({
      url: '/hospital/reserve',
      method: 'POST',
      data: {
        user_id: this.data.userId,
        hospital_id: hospitalId,
        department: department,
        reserve_date: date
      }
    })
    
    wx.showToast({ title: '预约成功', icon: 'success' })
    setTimeout(() => {
      wx.navigateBack()
    }, 1500)
  } catch (err) {
    console.error('预约失败:', err)
  }
}
```

---

### 五、健康监控模块

#### 1. 同步健康数据

```javascript
// pages/Setting/Setting.js
async syncHealthData(healthData) {
  try {
    const alert = await request({
      url: '/health/sync',
      method: 'POST',
      data: {
        user_id: this.data.userId,
        device_sn: this.data.deviceSn,
        heart_rate: healthData.heartRate,
        blood_pressure: healthData.bloodPressure,
        step_count: healthData.stepCount
      }
    })
    
    // 如果有健康预警
    if (alert.has_alert) {
      wx.showModal({
        title: '健康提醒',
        content: alert.alert_message,
        showCancel: false
      })
    }
  } catch (err) {
    console.error('同步健康数据失败:', err)
  }
}
```

#### 2. 查询健康历史

```javascript
async queryHealthHistory(days = 7) {
  try {
    const history = await request({
      url: '/health/history',
      method: 'GET',
      data: {
        user_id: this.data.userId,
        days: days
      }
    })
    
    this.setData({ healthHistory: history })
  } catch (err) {
    console.error('查询健康历史失败:', err)
  }
}
```

---

### 六、紧急呼叫模块

#### 1. 获取紧急联系人

```javascript
// pages/Emergency_safety/Emergency_safety.js
async getEmergencyContacts() {
  try {
    const contacts = await request({
      url: '/emergency/contacts',
      method: 'GET',
      data: {
        user_id: this.data.userId
      }
    })
    
    this.setData({ emergencyContacts: contacts })
  } catch (err) {
    console.error('获取紧急联系人失败:', err)
  }
}
```

#### 2. 拨打电话并记录日志

```javascript
makeCall(contact) {
  wx.makePhoneCall({
    phoneNumber: contact.phone_raw,
    success: () => {
      // 记录呼叫日志
      this.logEmergencyCall(contact, 'initiated')
    },
    fail: () => {
      wx.showToast({ title: '拨打失败', icon: 'none' })
      this.logEmergencyCall(contact, 'failed')
    }
  })
}

async logEmergencyCall(contact, status) {
  try {
    await request({
      url: '/emergency/log',
      method: 'POST',
      data: {
        user_id: this.data.userId,
        callee_type: contact.contact_type,
        callee_name: contact.name,
        callee_phone: contact.phone_raw,
        call_status: status
      }
    })
  } catch (err) {
    console.error('记录呼叫日志失败:', err)
  }
}
```

---

### 七、用户中心模块

#### 1. 获取用户信息

```javascript
// pages/profile/profile.js
async getUserProfile() {
  try {
    const profile = await request({
      url: '/user/profile',
      method: 'GET',
      data: {
        user_id: this.data.userId
      }
    })
    
    this.setData({ userProfile: profile })
  } catch (err) {
    console.error('获取用户信息失败:', err)
  }
}
```

#### 2. 更新用户设置

```javascript
async updateUserSettings(settings) {
  try {
    await request({
      url: '/user/settings',
      method: 'POST',
      data: {
        user_id: this.data.userId,
        font_size: settings.fontSize,
        voice_volume: settings.voiceVolume,
        dialect_type: settings.dialectType
      }
    })
    
    wx.showToast({ title: '保存成功', icon: 'success' })
  } catch (err) {
    console.error('更新设置失败:', err)
  }
}
```

---

## 🔧 调试技巧

### 1. 开启调试模式

在 `app.js` 中：

```javascript
App({
  globalData: {
    debug: true,  // 开发环境设为true
    userId: 'USER_102301524'  // 测试用户ID
  }
})
```

### 2. Mock数据（后端未就绪时）

```javascript
// utils/mock.js
const mockData = {
  '/payment/unpaid-items': {
    code: 0,
    data: [
      { item_id: '1', type: '电费', amount: 126.30, due_date: '2025-12-05' },
      { item_id: '2', type: '水费', amount: 85.60, due_date: '2025-12-10' }
    ]
  }
}

function getMockData(url) {
  return mockData[url] || { code: 404, message: 'Mock数据不存在' }
}

module.exports = { getMockData }
```

### 3. 错误处理

```javascript
// 统一错误处理
function handleError(err, showToast = true) {
  console.error('API Error:', err)
  
  if (showToast) {
    wx.showToast({
      title: err.message || '操作失败',
      icon: 'none',
      duration: 2000
    })
  }
  
  // 上报错误日志
  if (getApp().globalData.debug) {
    console.log('Error Stack:', err.stack)
  }
}
```

---

## 📦 完整API列表

| 模块 | 接口路径 | 方法 | 说明 |
|------|----------|------|------|
| **语音支付** | `/payment/voice-pay` | POST | 语音支付 |
| | `/payment/unpaid-items` | GET | 查询待缴费 |
| | `/payment/history` | GET | 缴费历史 |
| **智能陪聊** | `/chat/message` | POST | 发送消息 |
| | `/chat/parse-intent` | POST | 意图识别 |
| **打车** | `/taxi/create-order` | POST | 创建订单 |
| | `/taxi/common-addresses` | GET | 常用地址 |
| | `/taxi/order-status` | GET | 订单状态 |
| **医院挂号** | `/hospital/nearby` | GET | 附近医院 |
| | `/hospital/reserve` | POST | 创建预约 |
| | `/hospital/orders` | GET | 预约记录 |
| **健康监控** | `/health/sync` | POST | 同步数据 |
| | `/health/history` | GET | 健康历史 |
| | `/health/bind-device` | POST | 绑定设备 |
| **紧急呼叫** | `/emergency/contacts` | GET | 紧急联系人 |
| | `/emergency/log` | POST | 记录呼叫 |
| | `/emergency/add-contact` | POST | 添加联系人 |
| **用户中心** | `/user/profile` | GET | 用户信息 |
| | `/user/settings` | POST | 更新设置 |
| | `/user/addresses` | GET | 地址列表 |

---

## 🚀 快速开始

1. **配置后端地址**
   ```javascript
   // config/config.js
   const config = {
     apiBaseUrl: 'https://your-backend-server.com/api'
   }
   ```

2. **引入request工具**
   ```javascript
   const { request } = require('../../utils/request.js')
   ```

3. **调用API**
   ```javascript
   const result = await request({
     url: '/payment/voice-pay',
     method: 'POST',
     data: { ... }
   })
   ```

---

## 📋 接口参数详细说明

### 一、语音支付模块参数

#### 1. 发起语音支付 `POST /payment/voice-pay`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |
| audio_data | String | ✅ | 音频文件的base64编码 |
| session_id | String | ❌ | 会话ID（首次为空，后续传入） |

**返回参数：**
| 参数名 | 类型 | 说明 |
|--------|------|------|
| session_id | String | 会话ID，用于保持上下文 |
| reply_text | String | 语音回复文本内容 |
| payment_order | Object | 支付订单信息（需要支付时才有） |
| ├─ timeStamp | String | 微信支付时间戳 |
| ├─ nonceStr | String | 微信支付随机字符串 |
| ├─ package | String | 微信支付预支付ID |
| └─ paySign | String | 微信支付签名 |

**调用示例：**
```javascript
const result = await request({
  url: '/payment/voice-pay',
  method: 'POST',
  data: {
    user_id: 'USER_102301524',
    audio_data: 'base64_encoded_audio_data',
    session_id: 'SESSION_123456'
  }
})
```

#### 2. 查询待缴费项目 `GET /payment/unpaid-items`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |

**返回参数：**
```javascript
[
  {
    item_id: "缴费项ID",
    type: "电费/水费/燃气费",
    amount: 126.30,
    due_date: "2025-12-05",
    account_number: "账号（脱敏显示）"
  }
]
```

---

### 二、智能陪聊模块参数

#### 1. 发送聊天消息 `POST /chat/message`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |
| message | String | ✅ | 用户消息文本内容 |
| session_id | String | ❌ | 会话ID（首次为空） |

**返回参数：**
| 参数名 | 类型 | 说明 |
|--------|------|------|
| session_id | String | 会话ID |
| reply_text | String | AI回复文本 |
| need_tts | Boolean | 是否需要语音播报 |
| intent_type | String | 识别到的意图类型（可选） |

#### 2. 语音意图识别 `POST /chat/parse-intent`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |
| text | String | ✅ | 要识别的文本内容 |

**返回参数：**
| 参数名 | 类型 | 说明 |
|--------|------|------|
| intent_type | String | 意图类型：taxi/payment/register/chat |
| confidence | Float | 置信度（0-1之间） |
| slots | Object | 提取的槽位信息 |
| ├─ payment_type | String | 缴费类型（电费/水费等） |
| ├─ amount | Float | 金额 |
| ├─ destination | String | 目的地 |
| └─ department | String | 科室名称 |

---

### 三、打车模块参数

#### 1. 创建打车订单 `POST /taxi/create-order`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |
| start_type | String | ✅ | 出发地类型：manual（手动）/saved（常用地址） |
| start_location | Object | ✅ | 出发地位置信息 |
| ├─ address | String | ✅ | 详细地址 |
| ├─ longitude | Float | ✅ | 经度（GCJ-02坐标系） |
| └─ latitude | Float | ✅ | 纬度（GCJ-02坐标系） |
| end_type | String | ✅ | 目的地类型：manual/saved |
| end_location | Object | ✅ | 目的地位置信息 |
| ├─ address | String | ✅ | 详细地址 |
| ├─ longitude | Float | ✅ | 经度 |
| └─ latitude | Float | ✅ | 纬度 |

**返回参数：**
| 参数名 | 类型 | 说明 |
|--------|------|------|
| order_id | String | 订单ID |
| estimated_fee | Float | 预估费用（元） |
| estimated_time | Integer | 预估时长（分钟） |
| distance | Float | 距离（公里） |
| status | String | 订单状态 |

#### 2. 查询常用地址 `GET /taxi/common-addresses`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |

**返回参数：**
```javascript
[
  {
    address_id: "地址ID",
    name: "家/医院/超市",
    address: "详细地址",
    longitude: 116.397128,
    latitude: 39.916527,
    is_default: true
  }
]
```

---

### 四、医院挂号模块参数

#### 1. 查询附近医院 `GET /hospital/nearby`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| department | String | ❌ | 科室名称（不传则返回所有医院） |
| latitude | Float | ✅ | 当前纬度 |
| longitude | Float | ✅ | 当前经度 |
| radius | Integer | ❌ | 搜索半径（公里，默认5） |

**返回参数：**
```javascript
[
  {
    hospital_id: "医院ID",
    name: "北京协和医院",
    address: "东城区帅府园1号",
    distance: 1.2,
    available_quota: 5,
    departments: ["内科", "外科", "骨科"]
  }
]
```

#### 2. 创建预约订单 `POST /hospital/reserve`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |
| hospital_id | String | ✅ | 医院ID |
| department | String | ✅ | 科室名称 |
| reserve_date | String | ✅ | 预约日期（YYYY-MM-DD） |
| reserve_time | String | ❌ | 预约时间段（上午/下午） |

**返回参数：**
| 参数名 | 类型 | 说明 |
|--------|------|------|
| order_id | String | 预约订单ID |
| reserve_number | String | 预约号 |
| hospital_name | String | 医院名称 |
| department | String | 科室 |
| reserve_datetime | String | 预约日期时间 |

---

### 五、健康监控模块参数

#### 1. 同步健康数据 `POST /health/sync`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |
| device_sn | String | ✅ | 设备序列号 |
| heart_rate | Integer | ✅ | 心率（次/分钟） |
| blood_pressure | String | ❌ | 血压（格式：120/80） |
| step_count | Integer | ❌ | 步数 |
| temperature | Float | ❌ | 体温（℃） |
| blood_oxygen | Integer | ❌ | 血氧饱和度（%） |

**返回参数：**
| 参数名 | 类型 | 说明 |
|--------|------|------|
| has_alert | Boolean | 是否有健康预警 |
| alert_level | String | 预警级别：warning（警告）/danger（危险） |
| alert_message | String | 预警消息内容 |
| notify_family | Boolean | 是否已通知家属 |

**健康预警阈值：**
- **心率**：正常范围 60-100，≥105 或 ≤55 触发预警
- **血压**：收缩压 90-140，舒张压 60-90
- **血氧**：正常 ≥95%，<95% 触发预警

#### 2. 查询健康历史 `GET /health/history`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |
| days | Integer | ❌ | 查询最近N天的数据（默认7天） |

**返回参数：**
```javascript
[
  {
    log_time: "2025-11-28 10:00:00",
    heart_rate: 75,
    blood_pressure: "120/80",
    step_count: 8500,
    has_alert: false
  }
]
```

---

### 六、紧急呼叫模块参数

#### 1. 获取紧急联系人 `GET /emergency/contacts`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |

**返回参数：**
```javascript
[
  {
    contact_id: "联系人ID",
    name: "李女士（女儿）",
    phone_raw: "13912345678",        // 完整号码（用于拨打）
    phone_display: "139****5678",    // 脱敏显示
    contact_type: "family/emergency", // 联系人类型
    is_primary: true,                 // 是否主联系人
    relation: "女儿"                  // 关系
  }
]
```

#### 2. 记录呼叫日志 `POST /emergency/log`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |
| callee_type | String | ✅ | 被叫类型：family/emergency |
| callee_name | String | ❌ | 被叫姓名 |
| callee_phone | String | ✅ | 被叫电话 |
| call_status | String | ✅ | 呼叫状态：initiated（已发起）/failed（失败） |

#### 3. 添加紧急联系人 `POST /emergency/add-contact`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |
| name | String | ✅ | 联系人姓名 |
| phone | String | ✅ | 联系人电话 |
| contact_type | String | ✅ | 类型：family/emergency |
| relation | String | ❌ | 关系（女儿/儿子等） |
| is_primary | Boolean | ❌ | 是否设为主联系人（默认false） |

---

### 七、用户中心模块参数

#### 1. 获取用户信息 `GET /user/profile`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |

**返回参数：**
| 参数名 | 类型 | 说明 |
|--------|------|------|
| user_name | String | 用户姓名 |
| age | Integer | 年龄 |
| phone_display | String | 手机号（脱敏显示） |
| avatar_url | String | 头像URL |
| register_time | String | 注册时间 |

#### 2. 更新用户设置 `POST /user/settings`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |
| font_size | String | ❌ | 字体大小：small/medium/large |
| voice_volume | Integer | ❌ | 音量（0-100） |
| dialect_type | String | ❌ | 方言类型：mandarin（普通话）/cantonese（粤语）/sichuan（四川话） |

#### 3. 地址列表 `GET /user/addresses`

**请求参数：**
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| user_id | String | ✅ | 用户ID |

**返回参数：**
```javascript
[
  {
    address_id: "地址ID",
    name: "家/公司/其他",
    address: "详细地址",
    is_default: true
  }
]
```

---

## 📊 所有接口参数汇总表

### 请求参数汇总

| 接口路径 | 方法 | 必填参数 | 可选参数 |
|----------|------|----------|----------|
| `/payment/voice-pay` | POST | user_id, audio_data | session_id |
| `/payment/unpaid-items` | GET | user_id | - |
| `/payment/history` | GET | user_id | - |
| `/chat/message` | POST | user_id, message | session_id |
| `/chat/parse-intent` | POST | user_id, text | - |
| `/taxi/create-order` | POST | user_id, start_location, end_location | start_type, end_type |
| `/taxi/common-addresses` | GET | user_id | - |
| `/taxi/order-status` | GET | user_id, order_id | - |
| `/hospital/nearby` | GET | latitude, longitude | department, radius |
| `/hospital/reserve` | POST | user_id, hospital_id, department, reserve_date | reserve_time |
| `/hospital/orders` | GET | user_id | - |
| `/health/sync` | POST | user_id, device_sn, heart_rate | blood_pressure, step_count, temperature, blood_oxygen |
| `/health/history` | GET | user_id | days |
| `/health/bind-device` | POST | user_id, device_sn, device_type | - |
| `/emergency/contacts` | GET | user_id | - |
| `/emergency/log` | POST | user_id, callee_type, callee_phone, call_status | callee_name |
| `/emergency/add-contact` | POST | user_id, name, phone, contact_type | relation, is_primary |
| `/user/profile` | GET | user_id | - |
| `/user/settings` | POST | user_id | font_size, voice_volume, dialect_type |
| `/user/addresses` | GET | user_id | - |

---

## 🎯 通用参数说明

### 1. 用户身份参数
- **user_id**（必填）- 所有接口都需要，格式：USER_xxxxxxxx

### 2. 会话管理参数
- **session_id**（可选）- 用于保持对话上下文，首次调用为空，后续传入返回的session_id

### 3. 音频参数规范
- **格式**：mp3
- **采样率**：16000Hz
- **声道数**：1（单声道）
- **码率**：48000bps
- **编码**：base64

### 4. 位置参数规范
- **坐标系**：GCJ-02（火星坐标系）
- **latitude**：纬度，范围 -90 到 90
- **longitude**：经度，范围 -180 到 180

### 5. 日期时间格式
- **日期**：YYYY-MM-DD（如：2025-12-10）
- **时间**：HH:mm:ss（如：14:30:00）
- **日期时间**：YYYY-MM-DD HH:mm:ss

### 6. 返回码说明
- **code: 0** - 成功
- **code: 400** - 参数错误
- **code: 401** - 未授权
- **code: 404** - 资源不存在
- **code: 500** - 服务器错误

---

## 🔍 参数验证规则

### 字符串参数
- **user_id**: 长度6-32位
- **phone**: 11位数字
- **session_id**: 32位字符串

### 数值参数
- **heart_rate**: 30-200（次/分钟）
- **voice_volume**: 0-100
- **age**: 1-150

### 数组参数
- **最大长度**: 通常不超过100条记录

---

## 💡 最佳实践

### 1. 错误处理
```javascript
try {
  const result = await request({ ... })
} catch (err) {
  if (err.code === 400) {
    console.error('参数错误:', err.message)
  } else if (err.code === 401) {
    // 跳转到登录页
  }
}
```

### 2. 超时处理
```javascript
const result = await request({
  url: '/payment/voice-pay',
  method: 'POST',
  data: { ... },
  timeout: 15000  // 语音接口建议15秒超时
})
```

### 3. 重试机制
```javascript
async function requestWithRetry(options, maxRetries = 3) {
  for (let i = 0; i < maxRetries; i++) {
    try {
      return await request(options)
    } catch (err) {
      if (i === maxRetries - 1) throw err
      await sleep(1000 * (i + 1))  // 递增延迟
    }
  }
}
```

---

**祝开发顺利！有问题随时查阅本文档 📖**
