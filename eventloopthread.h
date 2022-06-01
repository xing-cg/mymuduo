#pragma once
#include"noncopyable.h"
#include"thread.h"
#include<functional>
#include<mutex>
#include<condition_variable>
class EventLoop;
class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
public:
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());
    ~EventLoopThread();
    EventLoop * startLoop();
private:
    void threadFunc();
private:
    EventLoop * m_loop;
    bool m_exiting;
    Thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    ThreadInitCallback m_callback;
};