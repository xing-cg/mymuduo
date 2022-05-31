#include"eventloop.h"
#include"logger.h"
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
#include<sys/eventfd.h>
int createEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("Failed in eventfd: %d\n", errno);
    }
    return evtfd;
}