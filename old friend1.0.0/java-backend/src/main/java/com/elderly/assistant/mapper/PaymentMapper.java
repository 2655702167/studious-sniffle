package com.elderly.assistant.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.elderly.assistant.entity.PaymentItem;
import org.apache.ibatis.annotations.Mapper;

/**
 * 缴费项目Mapper接口
 *
 * @author 施汉霖
 */
@Mapper
public interface PaymentMapper extends BaseMapper<PaymentItem> {
}
