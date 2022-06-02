#pragma once
#include"noncopyable.h"
#include"socket.h"
#include"channel.h"
#include<functional>
class EventLoop;
class InetAddress;
class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int fd, const InetAddress&)>;
    /* 此构造的三个参数本身也是TcpServer的三个参数 */
    Acceptor(EventLoop * loop, const InetAddress & listenAddr, bool reusePort);
    ~Acceptor();
public:
    void listen();
public:
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        m_newConnectionCallback = cb;
    }
    bool listenning() const {return m_listenning;}
private:
    void handleRead();
private:
    EventLoop * m_loop;
    Socket m_acceptSocket;
    Channel m_acceptChannel;
    NewConnectionCallback m_newConnectionCallback;
    bool m_listenning;
};