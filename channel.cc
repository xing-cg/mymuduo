#include"channel.h"
#include"eventloop.h"       //update()
#include"timestamp.h"       //handleEvent(Timestamp)
#include"logger.h"

#include<sys/epoll.h>       //EPOLLIN EPOLLPRI EPOLLOUT

/* static变量类外初始化 */
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop * loop, int fd)
    : m_loop(loop), m_fd(fd), m_events(0), m_revents(0),
      m_index(-1), m_tied(false)
{

}
Channel::~Channel()
{

}
/**
 * @brief 何时调用tie？
 */
void Channel::tie(const std::shared_ptr<void> &obj)
{
    m_tie = obj;
    m_tied = true;
}
/**
 * @brief 负责更新poller中fd相应的事件，
 * 即调用epoll_ctl。
 * EventLoop 包含 channelList 和 poller。
 * channel通过所属的Eventloop去调用poller相关操作。
 */
void Channel::update()
{
    // add code...
    // m_loop->updateChannel(this);
}
/**
 * @brief 在所属的loop中删除本channel。
 */
void Channel::remove()
{
    // add code...
    // m_loop->removeChannel(this);
}
/**
 * @brief fd得到poller的事件通知后，处理事件的函数
 */
void Channel::handleEvent(Timestamp receiveTime)
{
    if(m_tied)
    {
        std::shared_ptr<void> guard = m_tie.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}
/**
 * @brief 根据poller通知的channel发生的具体事件，
 * 由channel负责调用相应的回调函数
 */
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents: %d\n", m_revents);
    if((m_revents & EPOLLHUP) && !(m_revents & EPOLLIN))
    {
        if(m_closeCallback)
        {
            m_closeCallback();
        }
    }
    if(m_revents & EPOLLERR)
    {
        if(m_errorCallback)
        {
            m_errorCallback();
        }
    }
    /* 可读事件发生 */
    if(m_revents & (EPOLLIN | EPOLLPRI))
    {
        if(m_readCallback)
        {
            m_readCallback(receiveTime);
        }
    }
    if(m_revents & EPOLLOUT)
    {
        if(m_writeCallback)
        {
            m_writeCallback();
        }
    }
}