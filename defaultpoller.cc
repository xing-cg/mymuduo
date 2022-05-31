/**
 * @file defaultpoller.cc
 * @brief 本文件是为Poller类中的newDefaultPoller(EventLoop*);提供的单独的代码实现文件；
 * 避免写到poller.cc是因为Poller是抽象基类，不适合在其中编写具体实现类。
 */

#include"poller.h"
#include"epollpoller.h"
#include<cstdlib>   //::getenv("xxx")
Poller * Poller::newDefaultPoller(EventLoop *loop)
{
    if(getenv("MUDUO_USE_POLL"))
    {
        return nullptr;                 //生成poll实例对象
    }
    else
    {
        return new EpollPoller(loop);   //生成epoll实例对象
    }
}