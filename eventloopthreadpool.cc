#include"eventloopthreadpool.h"
#include"eventloopthread.h"
EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : m_baseLoop(baseLoop),
      m_name(nameArg),
      m_started(false),
      m_numThreads(0),
      m_next(0)
{

}
EventLoopThreadPool::~EventLoopThreadPool()
{
    /**
     * nothing to do, bacause evey loop is on the thread stack,
     * that will destruct automatically.
     */
}
void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    m_started = true;
    for(int i = 0; i < m_numThreads; ++i)
    {
        char buf[m_name.size() + 32];
        /* 以线程池name + 下标序列号 作为thread线程的名字 */
        snprintf(buf, sizeof buf, "%s%d", m_name.c_str(), i);
        std::string threadName(buf);
        m_threads.push_back(std::make_unique<EventLoopThread>(cb, threadName));
        m_loops.push_back(m_threads.back()->startLoop());
    }
    /* m_numThreads == 0时, 上面for循环不会执行, 执行下面的操作 */
    if(m_numThreads == 0 && cb != nullptr)
    {
        cb(m_baseLoop);
    }
}
/* 体现了对subLoop的轮询算法 */
EventLoop* EventLoopThreadPool::getNextLoop()
{
    EventLoop * loop = m_baseLoop;
    if(!m_loops.empty())
    {
        loop = m_loops[m_next];
        ++m_next;
        if(m_next >= m_loops.size())
        {
            m_next = 0;
        }
    }
    return loop;
}
std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if(m_loops.empty())
    {
        return std::vector<EventLoop*>(1, m_baseLoop);
    }
    else
    {
        return m_loops;
    }
}