package com.elderly.assistant.service;

import cn.hutool.http.HttpRequest;
import cn.hutool.http.HttpResponse;
import cn.hutool.json.JSONObject;
import cn.hutool.json.JSONUtil;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import org.springframework.web.multipart.MultipartFile;

import java.io.File;
import java.io.IOException;
import java.util.Base64;

/**
 * 语音识别服务
 * 调用百度语音识别API
 * 
 * @author 施汉霖
 */
@Slf4j
@Service
public class VoiceService {

    @Value("${api.baidu.voice.app-id}")
    private String appId;

    @Value("${api.baidu.voice.api-key}")
    private String apiKey;

    @Value("${api.baidu.voice.secret-key}")
    private String secretKey;

    /**
     * 语音识别
     * @param audioFile 音频文件
     * @return 识别的文字
     */
    public String recognizeVoice(MultipartFile audioFile) {
        try {
            // 1. 获取访问令牌
            String accessToken = getAccessToken();
            
            // 2. 将音频文件转为 Base64
            byte[] audioData = audioFile.getBytes();
            String base64Audio = Base64.getEncoder().encodeToString(audioData);
            
            // 3. 调用百度语音识别 API
            String url = "https://vop.baidu.com/server_api";
            
            // 构建请求参数（百度标准版API支持pcm/wav/amr格式）
            JSONObject requestBody = new JSONObject();
            requestBody.set("format", "pcm");  // pcm格式（百度推荐）
            requestBody.set("rate", 16000);    // 采样率16000
            requestBody.set("channel", 1);     // 单声道
            requestBody.set("cuid", "elderly_assistant");
            requestBody.set("token", accessToken);
            requestBody.set("speech", base64Audio);
            requestBody.set("len", audioData.length);
            requestBody.set("dev_pid", 1537);  // 1537=普通话(支持简单的英文识别)
            
            log.info("调用百度语音识别API，文件大小: {} bytes", audioData.length);
            
            HttpResponse response = HttpRequest.post(url)
                    .header("Content-Type", "application/json")
                    .body(requestBody.toString())
                    .timeout(30000)
                    .execute();
            
            String responseBody = response.body();
            log.info("百度语音识别响应: {}", responseBody);
            
            JSONObject result = JSONUtil.parseObj(responseBody);
            
            // 4. 解析结果
            if (result.getInt("err_no") == 0) {
                return result.getJSONArray("result").getStr(0);
            } else {
                log.error("语音识别失败: {}", result.getStr("err_msg"));
                return "识别失败：" + result.getStr("err_msg");
            }
            
        } catch (Exception e) {
            log.error("语音识别异常", e);
            return "识别异常：" + e.getMessage();
        }
    }

    /**
     * 获取百度 API 访问令牌
     */
    private String getAccessToken() {
        String url = String.format(
            "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=%s&client_secret=%s",
            apiKey, secretKey
        );
        
        HttpResponse response = HttpRequest.post(url).execute();
        JSONObject result = JSONUtil.parseObj(response.body());
        
        return result.getStr("access_token");
    }

    /**
     * 根据文件名获取音频格式
     */
    private String getAudioFormat(String filename) {
        if (filename == null) return "wav";
        
        String lowerName = filename.toLowerCase();
        if (lowerName.endsWith(".mp3")) return "mp3";
        if (lowerName.endsWith(".wav")) return "wav";
        if (lowerName.endsWith(".pcm")) return "pcm";
        if (lowerName.endsWith(".amr")) return "amr";
        
        // 微信小程序录音默认格式
        return "mp3";
    }
}
