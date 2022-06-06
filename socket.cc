#include"socket.h"
#include"logger.h"
#include"inetaddress.h"
#include<unistd.h>      //close
#include<sys/socket.h>  //bind  SOL_SOCKET
#include<strings.h>     //bzero
#include<netinet/tcp.h> //TCP_NODELAY
Socket::~Socket()
{
    close(m_sockfd);
}
void Socket::bindAddress(const InetAddress &localAddr)
{
    /* bind return 0 when success */
    if(0 != ::bind(m_sockfd, (sockaddr*)localAddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd: %d fail, createNonblocking or Die\n", m_sockfd);
    }
}
void Socket::listen()
{
    if(0 != ::listen(m_sockfd, 1024))
    {
        LOG_FATAL("listen sockfd: %d fail\n", m_sockfd);
    }
}
int Socket::accept(InetAddress * peerAddr)
{
    struct sockaddr_in addr;
    socklen_t len;
    bzero(&addr, sizeof addr);
    int connfd = ::accept(m_sockfd, (sockaddr*)&addr, &len);
    if(connfd >= 0)
    {
        peerAddr->setSockAddr(addr);
    }
    return connfd;
}
void Socket::shutdownWrite()
{
    if(::shutdown(m_sockfd, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdown Write error\n");
    }
}
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}