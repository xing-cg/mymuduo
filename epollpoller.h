#pragma once
#include"poller.h"

#include<vector>

class EpollPoller : public Poller
{
public:
	EpollPoller(EventLoop *loop);
	~EpollPoller() override;
public:
	Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
public:
	void updateChannel(Channel *channel) override;
	void removeChannel(Channel *channel) override;

private:
	void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
private:
	void update(int operation, Channel * channel);

private:
    int m_epollfd;
    using EventList = std::vector<struct epoll_event>;
    EventList m_events;
    /* EventList初始的长度 */
	static const int kInitEventListSize = 16;
};