#pragma once
#include"noncopyable.h"
#include"timestamp.h"
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
    using Functor = std::function<void()>;
private:
    using ChannelList = std::vector<Channel*>;
    ChannelList m_activeChannels;
private:
    std::atomic_bool m_looping;
    std::atomic_bool m_quit;
    std::atomic_bool m_callingPendingFunctors;
private:
    const pid_t m_threadId;
private:
    std::unique_ptr<Poller> m_poller;
private:
    Timestamp m_pollReturnTime;
private:
    int m_wakeupFd;
private:
    std::unique_ptr<Channel> m_wakeupChannel;
private:
    std::vector<Functor> m_pendingFunctors;
    mutable std::mutex m_mutex;
};