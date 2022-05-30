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
    /* 定义通用事件回调函数、只读事件回调函数的函数对象类型别名 */
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
public:
    /* 对外提供的设置回调函数对象的接口 */
    void setReadCallback(ReadEventCallback cb);
    void setWriteCallback(EventCallback cb);
    void setCloseCallback(EventCallback cb);
    void setErrorCallback(EventCallback cb);
public:
    /* fd得到poller的通知后，处理事件 */
    void handleEvent(Timestamp receiveTime);

public:
    /* 防止channel被手动remove后，还在执行回调操作 */
    void tie(const std::shared_ptr<void>&);

/* 以下是fd、events、revents相关的接口 */
public:
    int fd() const {return m_fd;}
    int events() const {return m_events;}
    /* 向poller提供的设置revents的接口 */
    void set_revents(int revt)
    {
        m_revents = revt;
    }
    /* 判断函数：判断有没有注册事件等等 */
    bool isNoneEvent() const
    {
        return m_events == kNoneEvent;
    }
	bool isWriting() const
    {
        return m_events & kWriteEvent;
    }
	bool isReading() const
    {
        return m_events & kReadEvent;
    }
    /* 使能、使不能函数：设置fd相应的事件状态。
     * 对m_events进行位操作之后调用update()，即epoll_ctl */
	void enableReading()
    {
        m_events |= kReadEvent;
        update();
    }
	void disableReading()
    {
        m_events &= ~kReadEvent;
        update();
    }
	void enableWriting()
    {
        m_events |= kWriteEvent;
        update();
    }
	void disableWriting()
    {
        m_events &= ~kWriteEvent;
    }
	void disableAll()
    {
        m_events = kNoneEvent;
        update();
    }

public:
    /* 与EventLoop相关 - 获取所属的Loop */
	EventLoop * ownerLoop() {return m_loop;}
public:
    /* 删除本channel */
	void remove();
/* 其他函数 */
public:
    /* for poller 的函数 */
	int index() {return m_index;}
	void set_index(int idx) {m_index = idx;}

/*************************私有成员*************************/
private:
    /* update() - 相当于调用epoll_ctl */
	void update();
private:
    /* handleEventWithGuard - 受保护地处理事件 */
	void handleEventWithGuard(Timestamp receiveTime);

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
    /* 四个函数对象，可以绑定外部用户传入的相关操作。
     * 因为channel知道发生了哪些事情（revents记录），
     * 所以channel负责调用具体事件的回调函数。 */
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