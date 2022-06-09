#include"tcpconnection.h"
#include"logger.h"
#include"socket.h"
#include"channel.h"
#include"eventloop.h"
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
/**
 * 1. 给loop赋值，name起名字
 * 2. 赋state为kConnecting、reading为true
 * 3. 以sockfd为参数构造socket，new后赋给智能指针m_socket
 * 4. 以loop、sockfd为参数构造channel，new后赋给智能指针m_channel
 * 5. 赋值localAddr、peerAddr
 * 6. 赋高水位阈值m_highWaterMark为64*1024*1024(64M)
 * 7. 给m_channel设置相应的回调，当poller给channel通知感兴趣的事件发生，则channel会回调相应的操作函数
 * 8. 对m_socket调用setKeepAlive，使TCP启动保活机制
 */
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
    LOG_INFO("%s [%s] at fd = %d, state = %d\n",
              __FUNCTION__, m_name.c_str(), m_channel->fd(), m_state.load());
}
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = m_inputBuffer.readFd(m_channel->fd(), &savedErrno);
    if(n > 0)
    {
        //已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
        m_messageCallback(shared_from_this(), &m_inputBuffer, receiveTime);
    }
    else if(n == 0) //客户端断开
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("%s\n", __FUNCTION__);
        handleError();
    }
}
void TcpConnection::handleWrite()
{
    if(m_channel->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = m_outputBuffer.writeFd(m_channel->fd(), &savedErrno);
        if(n > 0)
        {
            m_outputBuffer.retrieve(n);
            if(m_outputBuffer.readableBytes() == 0)
            {
                m_channel->disableWriting();
                if(m_writeCompleteCallback)
                {
                    /* 唤醒loop对应的thread线程, 执行回调 */
                    m_loop->queueInLoop(std::bind(m_writeCompleteCallback,
                                                  shared_from_this()));
                }
                if(m_state == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("%s\n", __FUNCTION__);
        }
    }
    else
    {
        LOG_ERROR("%s: connection fd = %d is down, no more writing.\n",
                   __FUNCTION__, m_channel->fd());
    }
}
void TcpConnection::handleClose()
{
    LOG_INFO("%s: fd = %d, state: %d\n", __FUNCTION__, m_channel->fd(), m_state.load());
    setState(kDisconnected);
    m_channel->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    m_connectionCallback(connPtr);
    m_closeCallback(connPtr);
}
void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if(::getsockopt(m_channel->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("%s name: %s - SO_ERROR: %d\n", __FUNCTION__, m_name.c_str(), err);
}