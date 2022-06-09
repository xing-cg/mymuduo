#include"tcpconnection.h"
#include"logger.h"
#include"socket.h"
#include"channel.h"
using namespace std::placeholders;
/* 写为static，防止函数名字冲突 */
static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}
TcpConnection::TcpConnection(EventLoop* loop, const std::string &nameArg, int sockfd,
                             const InetAddress &localAddr, const InetAddress &peerAddr)
    : m_loop(CheckLoopNotNull(loop)), m_name(nameArg),
      m_state(kConnecting), m_reading(true),
      m_socket(new Socket(sockfd)),
      m_channel(new Channel(loop, sockfd)),
      m_localAddr(localAddr), m_peerAddr(peerAddr),
      m_highWaterMark(64*1024*1024) //64M
{
    m_channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
    m_channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    m_channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    m_channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_INFO("%s [%s] at fd = %d\n", __FUNCTION__, m_name.c_str(), sockfd);
    m_socket->setKeepAlive(true);   //启动TCP保活机制
}
TcpConnection::~TcpConnection()
{
    LOG_INFO("%s [%s] at fd = %d, state = %d\n", m_name.c_str(), m_channel->fd(), m_state);
}