

const recorderManager = wx.getRecorderManager();
const innerAudioContext = wx.createInnerAudioContext();

Page({
  data: {
    isRecording: false,           // 是否正在录音
    isPlaying: false,             // 是否正在播放回复
    replyText: '',                // 语音回复文本
    sessionId: '',                // 会话ID（多轮对话）
    unpaidItems: [],              // 待缴费项目列表
    currentOrder: null,           // 当前支付订单
    recordingTime: 0,             // 录音时长
    recognitionStatus: 'idle'     // 识别状态（idle/recording/recognizing/success/error）
  },

  /**
   * 页面加载时初始化
   */
  onLoad: function() {
    this.initRecorder();
    this.loadUnpaidItems();
  },

  /**
   * 初始化录音管理器
   */
  initRecorder: function() {
    const that = this;
    
    // 录音停止事件
    recorderManager.onStop((res) => {
      console.log('录音停止', res);
      const { tempFilePath, duration } = res;
      that.setData({ recordingTime: Math.floor(duration / 1000) });
      
      // 将录音文件转为base64并发送到后端
      that.uploadAndRecognize(tempFilePath);
    });

    // 录音错误事件
    recorderManager.onError((err) => {
      console.error('录音失败', err);
      that.setData({ 
        recognitionStatus: 'error',
        replyText: '录音失败，请检查麦克风权限'
      });
      wx.showToast({
        title: '录音失败',
        icon: 'none'
      });
    });
  },

  /**
   * 加载用户待缴费项目
   */
  loadUnpaidItems: function() {
    const that = this;
    wx.request({
      url: 'https://your-api.com/api/payment/unpaid-items',
      method: 'GET',
      data: { user_id: wx.getStorageSync('userId') },
      success: (res) => {
        if (res.data.success) {
          that.setData({ unpaidItems: res.data.data });
        }
      },
      fail: (err) => {
        console.error('加载待缴费项目失败', err);
      }
    });
  },

  /**
   * 按下开始录音（适老化大按钮设计）
   */
  startRecord: function() {
    const that = this;
    
    // 检查麦克风权限
    wx.authorize({
      scope: 'scope.record',
      success: () => {
        that.setData({ 
          isRecording: true,
          recognitionStatus: 'recording',
          replyText: '请说话...'
        });
        
        // 开始录音（最长60秒）
        recorderManager.start({
          duration: 60000,
          sampleRate: 16000,
          numberOfChannels: 1,
          encodeBitRate: 48000,
          format: 'mp3'
        });

        // 震动反馈
        wx.vibrateShort();
      },
      fail: () => {
        wx.showModal({
          title: '需要麦克风权限',
          content: '请在设置中开启麦克风权限，以便使用语音支付功能',
          confirmText: '去设置',
          success: (res) => {
            if (res.confirm) {
              wx.openSetting();
            }
          }
        });
      }
    });
  },

  /**
   * 松开停止录音
   */
  stopRecord: function() {
    const that = this;
    that.setData({ 
      isRecording: false,
      recognitionStatus: 'recognizing',
      replyText: '正在识别...'
    });
    recorderManager.stop();
  },

  /**
   * 上传录音并进行语音识别
   */
  uploadAndRecognize: function(filePath) {
    const that = this;
    
    // 读取文件为base64
    wx.getFileSystemManager().readFile({
      filePath: filePath,
      encoding: 'base64',
      success: (res) => {
        const audioBase64 = res.data;
        
        // 调用后端语音支付接口
        wx.request({
          url: 'https://your-api.com/api/payment/voice-pay',
          method: 'POST',
          data: {
            user_id: wx.getStorageSync('userId'),
            audio_data: audioBase64,
            session_id: that.data.sessionId
          },
          success: (response) => {
            console.log('语音支付响应', response.data);
            that.handleVoicePaymentResponse(response.data);
          },
          fail: (err) => {
            console.error('语音支付请求失败', err);
            that.setData({
              recognitionStatus: 'error',
              replyText: '网络连接失败，请稍后重试'
            });
          }
        });
      },
      fail: (err) => {
        console.error('读取录音文件失败', err);
        that.setData({
          recognitionStatus: 'error',
          replyText: '文件读取失败'
        });
      }
    });
  },

  /**
   * 处理语音支付响应
   */
  handleVoicePaymentResponse: function(data) {
    const that = this;
    
    if (data.success) {
      that.setData({
        recognitionStatus: 'success',
        replyText: data.reply_text,
        sessionId: data.session_id || ''
      });

      // 播放语音回复（TTS）
      that.playTTSReply(data.reply_text);

      // 如果支付完成，处理支付订单
      if (data.next_action === 'complete' && data.payment_order) {
        that.setData({ currentOrder: data.payment_order });
        
        // 跳转到支付页面
        setTimeout(() => {
          wx.navigateTo({
            url: `/pages/payment/confirm?orderId=${data.payment_order.order_id}`
          });
        }, 2000);
      }

    } else {
      that.setData({
        recognitionStatus: 'error',
        replyText: data.reply_text || '处理失败，请重试'
      });
      that.playTTSReply(data.reply_text);
    }
  },

  /**
   * 播放TTS语音回复（调用微信同声传译或第三方TTS）
   */
  playTTSReply: function(text) {
    const that = this;
    
    // 方案1：使用微信插件语音合成（需要在app.json中引入插件）
    const plugin = requirePlugin("WechatSI");
    plugin.textToSpeech({
      lang: "zh_CN",
      tts: true,
      content: text,
      success: (res) => {
        console.log("TTS合成成功", res);
        that.setData({ isPlaying: true });
        
        innerAudioContext.src = res.filename;
        innerAudioContext.play();
        
        innerAudioContext.onEnded(() => {
          that.setData({ isPlaying: false });
        });
      },
      fail: (err) => {
        console.error("TTS合成失败", err);
        // 降级方案：仅显示文本
        wx.showToast({
          title: text,
          icon: 'none',
          duration: 3000
        });
      }
    });

    // 方案2：调用后端TTS接口（备选）
    // wx.request({
    //   url: 'https://your-api.com/api/tts/synthesize',
    //   method: 'POST',
    //   data: { text: text },
    //   success: (res) => {
    //     innerAudioContext.src = res.data.audio_url;
    //     innerAudioContext.play();
    //   }
    // });
  },

  /**
   * 语音查询待缴费项目
   */
  queryUnpaidByVoice: function() {
    const that = this;
    wx.request({
      url: 'https://your-api.com/api/payment/voice-query-unpaid',
      method: 'POST',
      data: { user_id: wx.getStorageSync('userId') },
      success: (res) => {
        if (res.data.success) {
          that.setData({ replyText: res.data.reply_text });
          that.playTTSReply(res.data.reply_text);
        }
      }
    });
  },

  /**
   * 重置会话（重新开始）
   */
  resetSession: function() {
    this.setData({
      sessionId: '',
      replyText: '',
      recognitionStatus: 'idle',
      currentOrder: null
    });
  },

  /**
   * 页面卸载时清理资源
   */
  onUnload: function() {
    recorderManager.stop();
    innerAudioContext.stop();
  }
});
