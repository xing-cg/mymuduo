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
    /* 开启监听wakeupChannel的EPOLLIN操作 */
    m_wakeupChannel->enableReading();
}
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(m_wakeupFd, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("%s reads %lu bytes instead of 8", __FUNCTION__, n);
    }
}
EventLoop::~EventLoop()
{
    m_wakeupChannel->disableAll();
    m_wakeupChannel->remove();
    close(m_wakeupFd);
    t_loopInThisThread = nullptr;
}
/**
 * @brief 开启事件循环;
 * 主要工作就是驱动Poller调用poll,
 * epoll_wait返回以后, 遍历执行有事件的channel回调操作;
 * 
 */
void EventLoop::loop()
{
    m_looping = true;
    m_quit = false;
    LOG_INFO("EventLoop %p start looping\n", this);
    /**
     * 1. 通过调用poller->poll, 传入m_activeChannels地址, 有事件发生则填入容器;
     * 2. 遍历m_activeChannels, 里面全部是监听到事件发生的channel, 每个都执行一次回调操作;
     * 3. 监听两类fd, 一种是client的fd(服务器/客户端通信用), 一种是wakeupfd(mainLoop与subLoop通信用);
     * 4. doPendingFunctors用于mainLoop wakeup subLoop之后要求其执行的回调操作, 需要由mainLoop事先添加;
     */
    while(!m_quit)
    {
        m_activeChannels.clear();
        m_pollReturnTime = m_poller->poll(kPollTimeMs, &m_activeChannels);
        for(Channel * channel : m_activeChannels)
        {
            m_currentActiveChannel = channel;
            m_currentActiveChannel->handleEvent(m_pollReturnTime);
        }
        m_currentActiveChannel = nullptr;
        /**
         * 执行当前EventLoop事件循环需要处理的回调操作;
         * mainLoop事先注册一个回调cb(需要subLoop来执行),
         * wakeup subLoop后, 执行doPendingFunctors, 即之前mainLoop在PendingFunctors注册的函数;
         */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping\n", this);
    m_looping = false;
}
/**
 * @brief 退出事件循环;
 * 情况1. 在loop的本线程中自己调用quit, 只需把m_quit置为true. 当程序不再阻塞即可进行, 自己无需控制;
 * 情况2. 在本线程中调用非本loop的quit, 如subLoop调用mainLoop的quit;
 * 对于情况2, 不仅要把m_quit置为true, 还需要把对方进行wakeup操作(通知其wake事件), 以便从poll返回, 下一次while循环时, 条件(!m_quit)将不满足, 退出循环;
 */
void EventLoop::quit()
{
    m_quit = true;
    /* 判断EventLoop对象是否在自己的线程里面 */
    if(!isInLoopThread())
    {
        wakeup();
    }
}
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else
    {
        /* 在非本loop线程中执行cb, 就需要唤醒loop所在线程, 执行cb */
        queueInLoop(cb);
    }
}
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_pendingFunctors.emplace_back(cb);
    }
    /** 
     * 唤醒 需要执行上面回调操作的loop线程
     * 判断条件1: 此loop不是当前线程;
     * 判断条件2: 此loop正在执行回调;
     */
    if(!isInLoopThread() || m_callingPendingFunctors)
    {
        wakeup();
    }
}
/**
 * @brief 与handleRead相对应;
 * 用来唤醒loop所在线程, 向wakeupFd写一个数据, 于是wakeupChannel发生可读事件, 此loop即被唤醒;
 */
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(m_wakeupFd, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("%s writes %lu bytes instead of 8", __FUNCTION__, n);
    }
}
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    m_callingPendingFunctors = true;
    {
    std::unique_lock<std::mutex> lock(m_mutex);
    functors.swap(m_pendingFunctors);
    }
    for(const Functor & functor : functors)
    {
        functor();
    }
    m_callingPendingFunctors = false;
}



/* 以下的函数实际上本应channel和poller直接沟通, 由于结构限制, 拿EventLoop作为父组件, 中介人 */
void EventLoop::updateChannel(Channel * channel)
{
    m_poller->updateChannel(channel);
}
void EventLoop::removeChannel(Channel * channel)
{
    m_poller->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel * channel)
{
    return m_poller->hasChannel(channel);
}
