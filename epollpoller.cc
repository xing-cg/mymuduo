#include"epollpoller.h"

#include"logger.h"      //LOG_FATAL
#include<errno.h>       //errno
#include<sys/epoll.h>
#include<unistd.h>      //close

#include"channel.h"
#include<cassert>
#include<cstring>       //bzero

#include"timestamp.h"
const int kNew 		= -1;	//从未添加到epoll的channel
const int kAdded 	= 1;	//已经添加到epoll的channel
const int kDeleted 	= 2;	//已把该channel从epoll中删除
/**
 * @brief 
 * 1. 初始化基类Poller中的loop;
 * 2. m_pollfd 初始化 为调用epoll_create1后的返回值,
 *    并标记为EPOLL_CLOEXEC(类似于FD_CLOEXEC);
 * 3. 初始化m_events(是个vector<epoll_event>)的长度;
 */
EpollPoller::EpollPoller(EventLoop * loop)
    : Poller(loop),
      m_epollfd(epoll_create1(EPOLL_CLOEXEC)),
      m_events(kInitEventListSize)  // vector<epoll_event>
{
    if(m_epollfd < 0)
    {
        LOG_FATAL("epoll_create error: %d\n", errno);
    }
}
/**
 * @brief close m_epollfd
 */
EpollPoller::~EpollPoller()
{
    close(m_epollfd);
}
/**
 * @brief 
 * 1. EventLoop包含一个ChannelList和一个Poller;
 * 2. ChannelList是所有channel的集合, 而Poller中也有一个channel的集合ChannelMap<fd, channel*>;
 * 3. ChannelList包含的channel的数目 大于等于 ChannelMap中的数目, 因为只有注册到poller上的channel才会保存到map中;
 */
void EpollPoller::updateChannel(Channel *channel)
{
    /* index 表示的是channel的三种状态之一 */
    const int index = channel->index();
    const int fd = channel->fd();
    LOG_INFO("function: %s, fd: %d, events: %d, index: %d\n",
             __FUNCTION__, fd, channel->events(), index);
    if(index == kNew || index == kDeleted)
    {
        if(index == kNew)           //表示此channel未添加到poller中
        {
            assert(m_channels.find(fd) == m_channels.end());
            m_channels[fd] = channel;
        }
        else if(index == kDeleted)  //表示此channel已添加到poller中, 但在epoll中删除
        {
            assert(m_channels.find(fd) != m_channels.end());
            assert(m_channels[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else                            //channel已添加到poller中并注册到epoll
    {
        assert(m_channels.find(fd) != m_channels.end());
        assert(m_channels[fd] == channel);
        assert(index == kAdded);
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
/**
 * @brief 根据channel最新状态 和 指明的epoll_operation 进行对epoll注册信息的更新;
 * 由updateChannel(Channel*)函数内部调用;
 * @param operation epoll_ctl add/mod/del
 */
void EpollPoller::update(int operation, Channel * channel)
{
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if(epoll_ctl(m_epollfd, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error: %d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error: %d\n", errno);
        }
    }
}
/**
 * @brief 把传入的具体channel*从Poller中的ChannelMap中移除;
 * 与EventLoop中的ChannelList无关;
 */
void EpollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    LOG_INFO("function: %s, fd: %d\n",
             __FUNCTION__, fd);
    assert(m_channels.find(fd) != m_channels.end());
    assert(m_channels[fd] == channel);
    assert(channel->isNoneEvent());

    int index = channel->index();
    assert(index == kAdded || index == kDeleted);

    /* 从ChannelMap中按fd删除 */
    size_t n = m_channels.erase(fd);//erase返回删除的元素数，对于唯一性map只可能为0或1
    assert(n == 1);
    /* 如果index为kAdded状态(即在epoll中注册过), 还需在epoll中删除 */
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}
/**
 * @brief 由EventLoop调用poller.poll;
 * 1. epoll_wait, 第1个参数: epollfd为成员属性m_epollfd, 第2个参数: 数组首址, 为成员属性m_events首元素的首址;
 * 2. 若epoll_wait返回值大于0则有事件发生, 调用fillActiveChannels函数, 第1个参数为事件数目, 第2个参数是activeChannels(vector容器)指针;
 * 
 * @param timeoutMs 事件循环超时时间
 * @param activeChannels 把EventLoop中的ChannelList通过指针传给poller, poller将回写发生事件的信息以通知EventLoop
 * @return Timestamp 
 */
Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    LOG_INFO("function: %s, fd total count: %lu\n", __FUNCTION__, m_channels.size());
    /* epoll_wait的第二个参数本来需要传入纯粹的数组, 但本项目为了便于数组的扩容, 使用vector管理, 为了找到地址, 传入了首元素的地址 */
    int numEvents = epoll_wait(m_epollfd, &*m_events.begin(),
                               static_cast<int>(m_events.size()), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents > 0)//有事件发生
    {
        LOG_INFO("%d events happened\n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
    }
    if(numEvents == m_events.size())//事件全部发生
    {
        m_events.resize(m_events.size() * 2);
    }
    else if(numEvents == 0)
    {
        LOG_INFO("%s :timeout!\n", __FUNCTION__);
    }
    else            // < 0
    {
        if(savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_ERROR("%s error: %d\n", __FUNCTION__, errno);
        }
    }
    return now;
}
/**
 * @brief 
 * 
 * @param numEvents 
 * @param activeChannels 
 */
void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for(int i = 0; i < numEvents; ++i)
    {
        Channel * channel = static_cast<Channel*>(m_events[i].data.ptr);
        /* channel自己的revents 根据poller中的m_events数组[i].events设置 */
        channel->set_revents(m_events[i].events);
        activeChannels->push_back(channel);
    }
}