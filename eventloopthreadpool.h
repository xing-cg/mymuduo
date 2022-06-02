#pragma once
#include"noncopyable.h"
#include<functional>
#include<vector>
#include<memory>
class EventLoop;
class EventLoopThread;
class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();
public:
    /* 供TcpServer调用 */
    void setThreadNum(int numThreads){m_numThreads = numThreads;}
public:
    /* 开启事件循环线程 */
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
public:
    /* 如果工作在多线程中，baseLoop默认以轮询的方式分配channel给subLoop */
    EventLoop * getNextLoop();
public:
    /* 获取所有管理着的loop，存到vector中，相当于拷贝了m_loops */
    std::vector<EventLoop*> getAllLoops();
public:
    bool started() const {return m_started;}
    const std::string name() const {return m_name;}
private:
    /* 用户最开始创建的loop */
    EventLoop * m_baseLoop;
    /* 包含了所有创建的线程 */
    std::vector<std::unique_ptr<EventLoopThread>> m_threads;
    /* 包含了所有管理着的loop的指针: 通过m_threads中的某个thread进行startLoop()返回loop的指针 */
    std::vector<EventLoop*> m_loops;

    std::string m_name;
    bool m_started;
    int m_numThreads;
    int m_next;
};