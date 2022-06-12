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
/**
 * 1. 判断当前连接的状态是否为connected; 
 * 2. 判断此loop是否在本thread中, 如果是则调用sendInLoop; 否则runInLoop, 绑定的函数也是sendInLoop;
 */
void TcpConnection::send(const std::string &buf)
{
    if(m_state == kConnected)
    {
        if(m_loop->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            m_loop->runInLoop(std::bind(&TcpConnection::sendInLoop,
                              this, buf.c_str(), buf.size()));
        }
    }
}
/**
 * 发送数据;
 * 应用写的快, 而内核发送数据慢, 
 * 需要把待发送数据写入缓冲区, 而且设置了水位回调.
 */
void TcpConnection::sendInLoop(const void * data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;	//记录是否产生错误
    if(m_state == kDisconnected)
    {
        LOG_ERROR("Disconnected, give up writing!");
        return;
    }
    /** 
     * m_channel->isWriting()为false表示channel第一次开始写数据, 
     * readableBytes()为0说明缓冲区没有待发送数据; 
     */
    if(!m_channel->isWriting() && m_outputBuffer.readableBytes() == 0)
    {
        nwrote = ::write(m_channel->fd(), data, len);
        if(nwrote >= 0)
        {
            remaining = len - nwrote;
            if(remaining == 0 && m_writeCompleteCallback)
            {
                //如果此时数据全部发送完毕, 不用再给channel设置epollout事件
                m_loop->queueInLoop(std::bind(m_writeCompleteCallback, shared_from_this()));
            }
        }
        else //nwrote < 0
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("%s\n", __FUNCTION__);
                if(errno == EPIPE || errno == ECONNRESET)// SIGPIPE or RESET
                {
                    faultError = true;
                }
            }
        }
    }
    if(!faultError && remaining > 0)//没有出错, 没有发送完毕, 剩余数据需要存到缓冲区, 然后给channel注册epollout事件, LT模式, poller发现tcp的发送缓冲区有空间, 会通知相应的sock->channel, 调用writeCallback方法, 即调用handleWrite, 直到把发送缓冲区中数据全部发送
    {
        size_t oldlen = m_outputBuffer.readableBytes();
        if(oldlen + remaining >= m_highWaterMark && oldlen < m_highWaterMark && m_highWaterMarkCallback)
        {
            m_loop->queueInLoop(std::bind(m_highWaterMarkCallback, shared_from_this(), oldlen+remaining));
        }
        m_outputBuffer.append((char*)data+nwrote, remaining);//data+nworte即剩余的位置
        if(!m_channel->isWriting())//m_channel->isWriting()为false表示channel第一次开始写数据, 之前没有注册epollout, 现在需要注册
        {
            m_channel->enableWriting();
        }
    }
}
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    m_channel->tie(shared_from_this());
    m_channel->enableReading();	//向poller注册channel的epollin事件
    //有新连接建立, 调用connectionCallback
    m_connectionCallback(shared_from_this());
}
void TcpConnection::connectDestoryed()
{
    if(m_state == kConnected)
    {
        setState(kDisconnected);
        m_channel->disableAll();
        m_connectionCallback(shared_from_this());
    }
    m_channel->remove();
}
void TcpConnection::shutdown()
{
    if(m_state == kConnected)
    {
        setState(kDisconnecting);
        m_loop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}
/**
 * 关闭写端, 将会触发epollhup, 
 * 会调用closeCallback, 
 * 即TcpConnection中的handleClose方法,
 * handleClose将会: 
 *  1. setState(kDisconnected);
 *  2. m_channel->disableAll();
 *  3. connectionCallback, closeCallback
 */
void TcpConnection::shutdownInLoop()
{
    if(!m_channel->isWriting())//说明outputBuffer中的数据已经全部发送完成
    {
        m_socket->shutdownWrite();//关闭写端, 将会触发epollhup, 会调用closeCallback, 即TcpConnection中的handleClose方法
    }
}