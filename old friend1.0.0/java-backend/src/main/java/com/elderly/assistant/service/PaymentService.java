package com.elderly.assistant.service;

import cn.hutool.core.util.IdUtil;
import cn.hutool.core.util.StrUtil;
import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.elderly.assistant.entity.PaymentItem;
import com.elderly.assistant.mapper.PaymentMapper;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.math.BigDecimal;
import java.util.List;

/**
 * 生活缴费服务层
 * 对应C++的PaymentService类
 *
 * @author 施汉霖
 */
@Slf4j
@Service
public class PaymentService {

    @Autowired
    private PaymentMapper paymentMapper;

    /**
     * 获取用户待缴费项目
     *
     * @param userId 用户ID
     * @return 缴费项目列表
     */
    public List<PaymentItem> getUserPaymentItems(String userId) {
        log.info("查询用户缴费项目，userId: {}", userId);
        
        if (StrUtil.isEmpty(userId)) {
            throw new IllegalArgumentException("用户ID不能为空");
        }

        LambdaQueryWrapper<PaymentItem> queryWrapper = new LambdaQueryWrapper<>();
        queryWrapper.eq(PaymentItem::getUserId, userId)
                   .orderByDesc(PaymentItem::getCreateTime);

        List<PaymentItem> items = paymentMapper.selectList(queryWrapper);
        log.info("查询到{}条缴费项目", items.size());
        
        return items;
    }

    /**
     * 根据ID查询缴费项目
     *
     * @param itemId 缴费项目ID
     * @return 缴费项目
     */
    public PaymentItem getPaymentItemById(String itemId) {
        if (StrUtil.isEmpty(itemId)) {
            throw new IllegalArgumentException("缴费项目ID不能为空");
        }
        
        return paymentMapper.selectById(itemId);
    }

    /**
     * 创建缴费项目
     *
     * @param paymentItem 缴费项目信息
     * @return 是否成功
     */
    public boolean createPaymentItem(PaymentItem paymentItem) {
        // 生成ID
        if (StrUtil.isEmpty(paymentItem.getItemId())) {
            paymentItem.setItemId("PAY_ITEM_" + IdUtil.getSnowflakeNextIdStr());
        }

        // 设置创建时间
        paymentItem.setCreateTime(System.currentTimeMillis());
        paymentItem.setUpdateTime(System.currentTimeMillis());

        // 默认状态
        if (StrUtil.isEmpty(paymentItem.getStatus())) {
            paymentItem.setStatus(paymentItem.getAmount().compareTo(BigDecimal.ZERO) > 0 ? "欠费" : "已缴清");
        }

        int result = paymentMapper.insert(paymentItem);
        return result > 0;
    }

    /**
     * 更新缴费项目
     *
     * @param paymentItem 缴费项目信息
     * @return 是否成功
     */
    public boolean updatePaymentItem(PaymentItem paymentItem) {
        paymentItem.setUpdateTime(System.currentTimeMillis());
        int result = paymentMapper.updateById(paymentItem);
        return result > 0;
    }

    /**
     * 标记为已缴清
     *
     * @param itemId 缴费项目ID
     * @return 是否成功
     */
    public boolean markAsPaid(String itemId) {
        PaymentItem item = paymentMapper.selectById(itemId);
        if (item == null) {
            throw new RuntimeException("缴费项目不存在");
        }

        item.setStatus("已缴清");
        item.setAmount(BigDecimal.ZERO);
        item.setLastPayTime(System.currentTimeMillis());
        item.setUpdateTime(System.currentTimeMillis());

        int result = paymentMapper.updateById(item);
        return result > 0;
    }

    /**
     * 删除缴费项目
     *
     * @param itemId 缴费项目ID
     * @return 是否成功
     */
    public boolean deletePaymentItem(String itemId) {
        int result = paymentMapper.deleteById(itemId);
        return result > 0;
    }
}
