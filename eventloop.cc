#include"eventloop.h"
#include"logger.h"
#include"poller.h"
#include"channel.h"
#include<sys/eventfd.h>
/**
 * @brief 防止一个线程创建多个loop;
 * __thread修饰表示这个全局变量转为了每个线程私有的属性.
 */
__thread EventLoop *t_loopInThisThread = nullptr;
/* 默认的超时时间 */
const int kPollTimeMs = 10000;
/**
 * @brief 创建wakeupfd，用来通知等待任务的subLoop，处理新的Channel事件.
 * @return int 返回的是eventfd
 */
int createEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("Failed in eventfd: %d\n", errno);
    }
    return evtfd;
}
/**
 * @brief 
 * 1. wakeupFd的作用就是一个门卫, 等待主loop去唤醒, 相当于主loop和每一个子loop之间的桥梁;
 * 2. wakeupFd通过createEventfd进行创建, 底层调用eventfd();
 * 3. wakeupChannel是一个指向Channel对象的智能指针, 此channel对应的是wakeupFd, 并且记录了当前loop;
 * 4. 初始化完成后, 还需对wakeupChannel设置事件及绑定回调函数;
 */
EventLoop::EventLoop()
    : m_looping(false), m_quit(false),
      m_callingPendingFunctors(false),
      m_threadId(CurrentThread::tid()),
      m_poller(Poller::newDefaultPoller(this)), //参数是loop
      m_wakeupFd(createEventfd()),
      m_wakeupChannel(new Channel(this, m_wakeupFd)), //Channel(this, fd)
      m_currentActiveChannel(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, m_threadId);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p already exists in this thread %d\n", t_loopInThisThread, m_threadId);
    }
    t_loopInThisThread = this;

    /* 设置wakeupfd的事件类型 以及发生事件后的回调操作(让子反应堆有事件, 从而loop的阻塞会返回, 开始工作) */
    m_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
    /* 每一个eventLoop都将监听wakeupChannel的EPOLLIN操作了 */
    m_wakeupChannel->enableReading();
}
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(m_wakeupFd, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("%s reads %d bytes instead of 8", __FUNCTION__, n);
    }
}
EventLoop::~EventLoop()
{
    m_wakeupChannel->disableAll();
    m_wakeupChannel->remove();
    close(m_wakeupFd);
    t_loopInThisThread = nullptr;
}