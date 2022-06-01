#include"thread.h"
#include"currentthread.h"
#include<semaphore.h>
std::atomic_int Thread::m_numCreated(0);
void Thread::setDefaultName(int numCreated)
{
    char buf[32] = {0};
    snprintf(buf, sizeof buf, "Thread%d", numCreated);
    m_name = buf;
}
Thread::Thread(ThreadFunc func, const std::string & name)
    : m_started(false), m_joined(false), m_tid(0),
      m_func(std::move(func)), m_name(name)
{
    int num = ++m_numCreated;
    if(m_name.empty())
    {
        setDefaultName(num);
    }
}
Thread::~Thread()
{
    /* 线程要么join, 要么detach */
    if(m_started && !m_joined)
    {
        m_thread->detach();
    }
}
void Thread::start()
{
    m_started = true;
    /* 为了tid初始化时期的线程安全, 保证tid有效 */
    sem_t sem;
    sem_init(&sem, false, 0);   //地址, 是否进程间共享, 初始值0
    
    m_thread = std::make_shared<std::thread>([&](){
        m_tid = CurrentThread::tid();
        
        sem_post(&sem);
        
        m_func();
    });

    sem_wait(&sem);
}
void Thread::join()
{
    m_joined = true;
    m_thread->join();
}