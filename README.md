# mymuduo

重写muduo库，锻炼网络编码技能。

# 总体思路

1. EventLoop包含ChannelList和Poller，Poller中又包含一个unoredered_map<int fd, Channel*>
2. channel包含它所从属的EventLoop的指针，从而在想要把自己添加到Loop、从Loop删除时调用EventLoop::updateChannel、EventLoop::removeChannel。这两个调用分别包装后封装到channel自己的类中——Channel::update(), Channel::remove。
3. 实际上，EventLoop::updateChannel、EventLoop::removeChannel最终仍需由loop调用Poller的相应方法，注册fd的events事件。