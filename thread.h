#pragma once
#include"noncopyable.h"
#include<functional>
#include<memory>
#include<thread>
#include<atomic>
class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;
    explicit Thread(ThreadFunc, const std::string & name = std::string());
    ~Thread();
    void start();
    void join();
    bool started() const {return m_started;}
    /* 此线程id不是真实的pthread_self线程id, 而是和top命令显示的id */
    pid_t tid() const {return m_tid;}
    const std::string & name() const {return m_name;}
    static int numCreated() {return m_numCreated;}
private:
    void setDefaultName(int);
private:
    bool m_started;
    bool m_joined;
    std::shared_ptr<std::thread> m_thread;
    pid_t m_tid;
    ThreadFunc m_func;
    std::string m_name;
private:
    static std::atomic_int m_numCreated;
};