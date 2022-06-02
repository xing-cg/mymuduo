#pragma once
#include"noncopyable.h"
class InetAddress;
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : m_sockfd(sockfd)
    {

    }
    ~Socket();
    int fd() const {return m_sockfd;}
    void bindAddress(const InetAddress &localAddr);
    void listen();
    int accept(InetAddress *peerAddr);
public:
    void shutdownWrite();
    /* 更改TCP选项, 直接交付, 不进行缓存 */
    void setTcpNoDelay(bool on);
    /* 更改TCP选项 */
    void setReuseAddr(bool on);
    /* 更改TCP选项 */
    void setReusePort(bool on);
    /* 更改TCP选项 */
    void setKeepAlive(bool on);
private:
    const int m_sockfd;
};