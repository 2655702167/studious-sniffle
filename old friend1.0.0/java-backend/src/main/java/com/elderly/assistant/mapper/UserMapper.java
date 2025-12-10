package com.elderly.assistant.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.elderly.assistant.entity.UserBase;
import org.apache.ibatis.annotations.Mapper;

/**
 * 用户Mapper接口
 *
 * @author 施汉霖
 */
@Mapper
public interface UserMapper extends BaseMapper<UserBase> {
}
