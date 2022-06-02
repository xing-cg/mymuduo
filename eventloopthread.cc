#include"eventloopthread.h"
#include"eventloop.h"
EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : m_loop(nullptr), m_exiting(false),
      m_thread(std::bind(&EventLoopThread::threadFunc, this), name),
      m_mutex(), m_cond(), m_callback(cb)
{

}
EventLoopThread::~EventLoopThread()
{
    m_exiting = true;
    if(m_loop != nullptr)
    {
        m_loop->quit();
        m_thread.join();
    }
}
EventLoop * EventLoopThread::startLoop()
{
    m_thread.start();   //启动底层新线程，执行回调函数，

    EventLoop * loop = nullptr;
    {/* 临界区m_loop */
    std::unique_lock<std::mutex> lock(m_mutex);
    while(m_loop == nullptr)
    {
        m_cond.wait(lock);
    }
    loop = m_loop;
    }/* 临界区m_loop */
    return loop;
}
/**
 * @brief Thread对象实际执行的线程函数，在单独的子线程中执行
 */
void EventLoopThread::threadFunc()
{
    EventLoop loop; //构造一个独立的eventloop, 和m_thread一对一, one loop per thread的证据
    if(m_callback)	//如果m_callback(即ThreadInitCallback)不为空则执行此函数
    {
        m_callback(&loop);
    }
    {/* 临界区m_loop */
    std::unique_lock<std::mutex> lock(m_mutex);
    m_loop = &loop;
    m_cond.notify_one();	//通知主线程的startLoop(), loop已经在子线程创建好了。
    }/* 临界区m_loop */
    loop.loop();    //EventLoop loop => Poller.poll
    
    /* 执行到此处说明loop已经结束 退出循环 */
    std::unique_lock<std::mutex> lock(m_mutex);
    m_loop = nullptr;
}