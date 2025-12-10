package com.elderly.assistant.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.elderly.assistant.entity.TaxiOrder;
import org.apache.ibatis.annotations.Mapper;

/**
 * 打车订单Mapper接口
 *
 * @author 施汉霖
 */
@Mapper
public interface TaxiOrderMapper extends BaseMapper<TaxiOrder> {
}
