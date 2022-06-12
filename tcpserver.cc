#include"tcpserver.h"
#include"logger.h"
#include"tcpconnection.h"
#include<functional>
#include<strings.h>
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
TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
                     const std::string &nameArg, Option option)
    : m_loop(CheckLoopNotNull(loop)),
      m_IPport(listenAddr.toIPport()),
      m_name(nameArg),
      m_acceptor(new Acceptor(loop, listenAddr, option == kReusePort)),
      m_threadPool(new EventLoopThreadPool(loop, m_name)),
      m_connectionCallback(),
      m_messageCallback(),
      m_nextConnId(1)
{
    m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection,
                                                   this, _1, _2));//connfd, peerAddr
}
TcpServer::~TcpServer()
{
    for(auto & item : m_connections)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestoryed, conn));
    }
}
void TcpServer::setThreadNum(int numThreads)
{
    m_threadPool->setThreadNum(numThreads);
}
void TcpServer::start()
{
    //防止一个TcpServer对象被start多次;
    if(m_started++ == 0)//即使bool为1，bool++后的值也还是1
    {
        m_threadPool->start(m_threadInitCallback);
        m_loop->runInLoop(std::bind(&Acceptor::listen, m_acceptor.get()));
    }
}
/**
 * 有新客户端连接时, acceptor会执行这个回调操作; 
 */
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *ioLoop = m_threadPool->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", m_IPport.c_str(), m_nextConnId);
    ++m_nextConnId;
    std::string connName = m_name + buf;
    LOG_INFO("%s [%s] - new connection [%s] from %s\n",
             __FUNCTION__, m_name.c_str(), connName.c_str(), peerAddr.toIPport().c_str());
    
    //通过sockfd获取其绑定的ip地址和端口信息
    sockaddr_in local;
    bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if(::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("socket::getsockname\n");
    }
    InetAddress localAddr(local);

    //根据连接成功的sockfd, 创建TcpConnection连接对象
    TcpConnectionPtr conn(std::make_shared<TcpConnection>
        (ioLoop, connName, sockfd, localAddr, peerAddr));
    m_connections[connName] = conn;
    //下面的回调是用户给TcpServer设置的
    conn->setConnectionCallback(m_connectionCallback);
    conn->setMessageCallback(m_messageCallback);
    conn->setWriteCompleteCallback(m_writeCompleteCallback);

    //设置如何关闭连接 - 用户调用conn->shutdown() 时回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));
    //直接调用TcpConnection::connectEstablished, 把state置为connected, channel tie操作, enableReading;
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}
void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    m_loop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("%s [%s] - connection %s\n",
             __FUNCTION__, m_name.c_str(), conn->name().c_str());
    m_connections.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestoryed, conn));
}