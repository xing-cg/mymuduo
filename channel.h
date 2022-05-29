#pragma once

#include"noncopyable.h"

#include<functional>        //std::function<>
#include<memory>            //std::weak_ptr

class Timestamp;
class EventLoop;
/**
 * @brief 通道，封装了sockfd和其感兴趣的event，
 * 如EPOLLIN、EPOLLOUT事件。
 * 还绑定了poller监听返回的具体时间。
 * 不可拷贝。
 */
class Channel : noncopyable
{
public:
    Channel(EventLoop *loop, int fd);
    ~Channel();
public:
    void handleEvent(Timestamp receiveTime);

public:
    /* 定义通用事件回调函数、只读事件回调函数的函数对象类型别名 */
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
private:
    EventLoop * m_loop; /* 事件循环 */
    const int m_fd;     /* fd，即Polle监听的对象 */
    int m_events;       /* fd感兴趣的事件注册信息 */
    int m_revents;      /* poller操作的fd上具体发生的事件 */
    int m_index;
private:
    /* 以下3个变量描述当前fd的状态，
     * 没有感兴趣的事件 or 对读事件感兴趣 or 对写事件感兴趣? */
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
private:
    /* 因为channel通道里面能够获知fd发生的事件revents，
     * 所以它负责调用具体事件的回调操作 */
    ReadEventCallback m_readCallback;
    EventCallback m_writeCallback;
    EventCallback m_closeCallback;
    EventCallback m_errorCallback;
private:
    /* 防止手动调用remove channel后仍使用此channel，
     * 用于监听跨线程的对象生存状态 */
    std::weak_ptr<void> m_tie;
    bool m_tied;
};