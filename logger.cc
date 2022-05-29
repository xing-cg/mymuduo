#include"logger.h"
/* 获取日志唯一的实例对象 */
Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}
/* 设置日志级别 */
void Logger::setLogLevel(int level)
{
    m_logLevel = level;
}
/**
 * @brief 写日志到标准输出设备。
 * 输出格式：[级别信息] time: msg
 * 
 * @param msg 消息体
 */
void Logger::log(std::string msg)
{
    switch (m_logLevel)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }
    /* 打印时间和消息体 */
    std::cout << "time" << ": " << msg << std::endl;
};