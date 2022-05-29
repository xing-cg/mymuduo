#pragma once

#include<iostream>

#include"noncopyable.h"

/**
 * 定义宏便于用户使用
 * 其中，DEBUG日志消息仅当用户声明#define NEED_DEBUG时才有效。
 * 否则LOG_DEBUG为空操作。
 */
#define LOG_INFO(logmsgFormat, ...) \
    do \
    {  \
       Logger & logger = logger::instance();            \
       logger.setLogLevel(INFO);                        \
       char buf[1024] = {0};                            \
       snprintf(buf, 1024, logmsgFormat, ##__VA__ARGS); \
       logger.log(buf);                                 \
    } while(0)
#define LOG_ERROR(logmsgFormat, ...) \
    do \
    {  \
       Logger & logger = logger::instance();            \
       logger.setLogLevel(ERROR);                        \
       char buf[1024] = {0};                            \
       snprintf(buf, 1024, logmsgFormat, ##__VA__ARGS); \
       logger.log(buf);                                 \
    } while(0)
#define LOG_FATAL(logmsgFormat, ...) \
    do \
    {  \
       Logger & logger = logger::instance();            \
       logger.setLogLevel(FATAL);                        \
       char buf[1024] = {0};                            \
       snprintf(buf, 1024, logmsgFormat, ##__VA__ARGS); \
       logger.log(buf);                                 \
    } while(0)
#ifndef NEED_DEBUG
    #define LOG_DEBUG(logmsgFormat, ...) \
        do \
        {  \
        Logger & logger = logger::instance();            \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                            \
        snprintf(buf, 1024, logmsgFormat, ##__VA__ARGS); \
        logger.log(buf);                                 \
        } while(0)
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif
/**
 * 日志的级别
 *  - INFO  - 普通信息
 *  - ERROR - 错误信息
 *  - FATAL - core信息
 *  - DEBUG - 调试信息
 */
enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};
/**
 * 单例模式
 * 禁止拷贝构造、赋值拷贝
 */
class Logger : noncopyable
{
public:
    /* 获取日志唯一的实例对象 */
    static Logger& instance();
    /* 设置日志级别 */
    void setLogLevel(int level);
    /* 写日志 */
    void log(std::string msg);
private:
    int m_logLevel;
    Logger(){}
};