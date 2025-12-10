package com.elderly.assistant.controller;

import com.elderly.assistant.common.Result;
import com.elderly.assistant.entity.PaymentItem;
import com.elderly.assistant.service.PaymentService;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.List;

/**
 * 生活缴费API控制器
 * 对应前端API调用指南中的 /payment 接口
 *
 * @author 施汉霖
 */
@Slf4j
@RestController
@RequestMapping("/payment")
@CrossOrigin
public class PaymentController {

    @Autowired
    private PaymentService paymentService;

    /**
     * 查询用户待缴费项目
     * GET /payment/unpaid-items?user_id=xxx
     *
     * @param userId 用户ID
     * @return 缴费项目列表
     */
    @GetMapping("/unpaid-items")
    public Result<List<PaymentItem>> getUnpaidItems(@RequestParam("user_id") String userId) {
        log.info("查询待缴费项目，userId: {}", userId);
        try {
            List<PaymentItem> items = paymentService.getUserPaymentItems(userId);
            return Result.success(items);
        } catch (Exception e) {
            log.error("查询待缴费项目失败", e);
            return Result.error(e.getMessage());
        }
    }

    /**
     * 查询缴费历史
     * GET /payment/history?user_id=xxx
     *
     * @param userId 用户ID
     * @return 缴费历史列表
     */
    @GetMapping("/history")
    public Result<List<PaymentItem>> getPaymentHistory(@RequestParam("user_id") String userId) {
        log.info("查询缴费历史，userId: {}", userId);
        try {
            List<PaymentItem> items = paymentService.getUserPaymentItems(userId);
            return Result.success(items);
        } catch (Exception e) {
            log.error("查询缴费历史失败", e);
            return Result.error(e.getMessage());
        }
    }

    /**
     * 语音支付（返回Mock数据）
     * POST /payment/voice-pay
     *
     * @param request 请求参数
     * @return 支付结果
     */
    @PostMapping("/voice-pay")
    public Result<?> voicePay(@RequestBody VoicePayRequest request) {
        log.info("语音支付，userId: {}, sessionId: {}", request.getUserId(), request.getSessionId());
        
        try {
            // Mock返回数据
            VoicePayResponse response = new VoicePayResponse();
            response.setSessionId("SESSION_" + System.currentTimeMillis());
            response.setReplyText("您好，请问需要缴纳什么费用？");
            response.setNeedTts(true);
            
            return Result.success(response);
        } catch (Exception e) {
            log.error("语音支付失败", e);
            return Result.error(e.getMessage());
        }
    }

    /**
     * 标记缴费项目为已支付
     * POST /payment/mark-paid
     *
     * @param itemId 缴费项目ID
     * @return 操作结果
     */
    @PostMapping("/mark-paid")
    public Result<?> markAsPaid(@RequestParam("item_id") String itemId) {
        log.info("标记为已支付，itemId: {}", itemId);
        try {
            boolean success = paymentService.markAsPaid(itemId);
            return success ? Result.success() : Result.error("标记失败");
        } catch (Exception e) {
            log.error("标记为已支付失败", e);
            return Result.error(e.getMessage());
        }
    }

    // ==================== 内部类：请求响应对象 ====================

    @lombok.Data
    static class VoicePayRequest {
        private String userId;
        private String audioData;
        private String sessionId;
    }

    @lombok.Data
    static class VoicePayResponse {
        private String sessionId;
        private String replyText;
        private Boolean needTts;
        private PaymentOrder paymentOrder;
    }

    @lombok.Data
    static class PaymentOrder {
        private String timeStamp;
        private String nonceStr;
        private String packageStr;
        private String paySign;
    }
}
