#include"tcpserver.h"
#include"logger.h"
#include<functional>
using namespace std::placeholders;
EventLoop* CheckLoopNotNull(EventLoop* loop)
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