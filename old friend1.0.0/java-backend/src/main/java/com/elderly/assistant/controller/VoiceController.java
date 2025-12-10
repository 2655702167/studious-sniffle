package com.elderly.assistant.controller;

import com.elderly.assistant.common.Result;
import com.elderly.assistant.service.VoiceService;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;

import java.util.HashMap;
import java.util.Map;

/**
 * 语音识别接口
 * 
 * @author 施汉霖
 */
@Slf4j
@RestController
@RequestMapping("/voice")
public class VoiceController {

    @Autowired
    private VoiceService voiceService;

    /**
     * 语音识别
     * @param file 音频文件
     * @return 识别结果
     */
    @PostMapping("/recognize")
    public Result<Map<String, String>> recognizeVoice(@RequestParam("file") MultipartFile file) {
        log.info("收到语音识别请求，文件名: {}, 大小: {} bytes", 
                 file.getOriginalFilename(), file.getSize());
        
        try {
            // 调用语音识别服务
            String text = voiceService.recognizeVoice(file);
            
            Map<String, String> data = new HashMap<>();
            data.put("text", text);
            
            log.info("语音识别成功: {}", text);
            return Result.success(data);
            
        } catch (Exception e) {
            log.error("语音识别失败", e);
            return Result.error("语音识别失败：" + e.getMessage());
        }
    }
}
