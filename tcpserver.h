#pragma once
#include"eventloop.h"
#include"acceptor.h"
#include"inetaddress.h"
#include"noncopyable.h"
#include"eventloopthreadpool.h"
#include"callbacks.h"
#include<functional>
#include<memory>
#include<unordered_map>
/**
 * @brief 对外提供的tcp服务器编程的类
 */
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    /* 是否对端口可重用 */
    enum Option
    {
        kNoReusePort,
        kReusePort
    };
public:
    TcpServer(EventLoop *loop, const InetAddress &listenAddr,
              const std::string &nameArg, Option option = kNoReusePort);
    ~TcpServer();
    void setThreadInitCallback(const ThreadInitCallback &cb)
    {
        m_threadInitCallback = cb;
    }
    void setConnectionCallback(const ConnectionCallback &cb)
    {
        m_connectionCallback = cb;
    }
    void setMessageCallback(const MessageCallback &cb)
    {
        m_messageCallback = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        m_writeCompleteCallback = cb;
    }

    /* 设置底层subLoop个数 */
    void setThreadNum(int numThreads);
    /* 开启服务器监听 */
    void start();
private:
    void newConnection(int sockfd, const InetAddress & peerAddr);
    void removeConnection(const TcpConnectionPtr & conn);
    void removeConnectionInLoop(const TcpConnectionPtr & conn);
private:
    /* baseLoop: 用户定义的baseLoop */
    EventLoop * m_loop;
    const std::string m_IPport;
    const std::string m_name;
    /* 运行在mainLoop，任务是监听新连接事件 */
    std::unique_ptr<Acceptor> m_acceptor;
    /* one loop per thread */
    std::shared_ptr<EventLoopThreadPool> m_threadPool;
private:
    ConnectionCallback      m_connectionCallback;   //新连接时的回调
    MessageCallback         m_messageCallback;      //有读写消息时的回调
    WriteCompleteCallback   m_writeCompleteCallback;//消息发送完成后的回调
    ThreadInitCallback      m_threadInitCallback;   //loop线程初始化的回调
private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    ConnectionMap m_connections;
private:
    std::atomic_int m_started;
    int m_nextConnId;
};