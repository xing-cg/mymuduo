#pragma once
#include"noncopyable.h"
#include"timestamp.h"
#include"currentthread.h"
#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>
class Channel;
class Poller;
/**
 * @brief 事件循环类：
 * 主要包含了两大模块。1、Channel；2、Poller（epoll的抽象）
 */
class EventLoop : noncopyable
{
public:
    EventLoop();
    ~EventLoop();
public:
    using Functor = std::function<void()>;
public:
    /* loop - 开始事件循环 */
    void loop();
    /* quit - 结束事件循环 */
    void quit();
public:
    Timestamp pollReturnTime() const
    {
        return m_pollReturnTime;
    }
public:
    /* 在当前loop中执行cb */
    void runInLoop(Functor cb);
public:
    /* 把cb放入队列中，唤醒loop所在的线程，执行cb */ 
    void queueInLoop(Functor cb);
public:
    /* mainLoop唤醒subLoop所在的线程 */
    void wakeup();
public:
    /* 更新Channel - EventLoop的方法调用Poller的方法 */
    void updateChannel(Channel *channel);
    /* 移除Channel - EventLoop的方法调用Poller的方法 */
    void removeChannel(Channel *channel);
    /* 判断是否有此Channel */
    bool hasChannel(Channel *channel);
public:
    /* 判断EventLoop对象是否在自己的线程里面 */
    bool isInLoopThread() const
    {
        return m_threadId == CurrentThread::tid();
    }


private:
    /* 处理wakeup唤醒相关的逻辑 */
    void handleRead();
private:
    /* 执行回调 */
    void doPendingFunctors();

/*************************成员变量*************************/
private:
    using ChannelList = std::vector<Channel*>;
    /* EventLoop管理的所有的Channel的List */
    ChannelList m_activeChannels;
    /* 主要用于断言 */
    Channel * m_currentActiveChannel;
private:
    /* 事件循环状态标志 - 真则正在循环，假则将要退出循环 */
    std::atomic_bool m_looping;
    /* 标识退出loop循环 */
    std::atomic_bool m_quit;
    /* 标识当前loop当前是否有需要执行的回调操作 */
    std::atomic_bool m_callingPendingFunctors;
private:
    /* 记录当前Loop所在线程的ID */
    const pid_t m_threadId;
private:
    /* EventLoop所管理的poller, 去轮询监听channels上发生的事件 */
    std::unique_ptr<Poller> m_poller;
private:
    /* poller返回发生事件的channels的时间点 */
    Timestamp m_pollReturnTime;
private:
    /**
     * @brief mainLoop获取到一个新用户的channel后, 搭配轮询算法选择一个
     * 等待任务的subLoop, 通过wakeupFd对其进行唤醒来处理channel;
     * 用eventfd创建;
     * eventfd使用线程间的wait/notify事件通知机制, 直接在内核唤醒, 效率较高;
     * 与此处理相似的是, libevent使用的是socketpair的双向通信机制, 相当于网络通信层面的机制, 效率较低;
     */
    int m_wakeupFd;
    /* 把wakeupFd封装起来和其Channel关联，因为操作的往往不是fd而是其channel */
    std::unique_ptr<Channel> m_wakeupChannel;
private:
    /**
     * @brief 存储loop需要执行的所有的回调操作; 
     * 与callingPendingFunctors标识结合使用; 
     * 如果此标识显示当前loop有需要执行的回调操作, 则这些回调操作将在此vector容器中存放; 
     * 需要用mutex保护其线程安全;
     */
    std::vector<Functor> m_pendingFunctors;
    mutable std::mutex m_mutex;
};