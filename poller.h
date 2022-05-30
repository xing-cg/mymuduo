#pragma once
#include<vector>
#include<unordered_map>
class Channel;
class EventLoop;
class Timestamp;
/**
 * @brief muduo库中多路事件分发器的核心IO复用模块 
 */
class Poller
{
public:
	Poller(EventLoop *loop);
    virtual ~Poller() = default;
public:
    using ChannelList = std::vector<Channel*>;
    /**
     * @brief 提供给系统的统一的一个IO复用接口
     * 
     * @param timeoutMs 超时时间，毫秒为单位
     * @param activeChannels 当前激活的、对事件注册好的channel列表
     * @return Timestamp 
     */
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    /**
     * @brief 当fd注册的事件有变更时，channel调用update，函数内包含updateChannel(this)
     * 
     * @param channel 外部channel传入的this指针
     */
    virtual void updateChannel(Channel * channel) = 0;
    /**
     * @brief 当fd注册的事件要注销时，channel调用remove，函数内包含removeChannel(this)
     * 
     * @param channel 外部channel传入的this指针
     */
    virtual void removeChannel(Channel * channel) = 0;
    /**
     * @brief 提供给EventLoop的接口，以获取默认的IO复用具体实现
     */
    static Poller* newDefaultPoller(EventLoop *loop);
public:
    /* 判断poller中是否拥有某一channel */
    virtual bool hasChannel(Channel * channel) const;

protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap m_channels;
private:
    /* poller从属的EventLoop */
    EventLoop * m_ownerLoop;
};