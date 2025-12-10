package com.elderly.assistant;

import org.mybatis.spring.annotation.MapperScan;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

/**
 * 老友助手后端服务启动类
 * 
 */
@SpringBootApplication
@MapperScan("com.elderly.assistant.mapper")
public class ElderlyAssistantApplication {

    public static void main(String[] args) {
        SpringApplication.run(ElderlyAssistantApplication.class, args);
        System.out.println("\n========================================");
        System.out.println("老友助手后端服务启动成功！");
        System.out.println("访问地址: http://localhost:8080/api");
        System.out.println("========================================\n");
    }
}
