#include"acceptor.h"
#include"logger.h"
#include"inetaddress.h"
#include<sys/socket.h>
#include<netinet/tcp.h>
#include<netinet/in.h>      //IPPROTO_TCP
#include<unistd.h>          //close
static int createNonblockingSocket()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socketfd create err: %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}
Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort)
    : m_loop(loop),
      m_acceptSocket(createNonblockingSocket()),    //create socket
      m_acceptChannel(loop, m_acceptSocket.fd()),
      m_listenning(false)
{
    m_acceptSocket.setReuseAddr(true);
    m_acceptSocket.setReusePort(true);
    m_acceptSocket.bindAddress(listenAddr);         //bind addr to socket
    // TcpServer::start() Acceptor::listen(), 如果有新连接需要执行回调 connfd->channel->subloop
    //baseLoop -> m_acceptChannel(listenfd) -> 
    m_acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}
Acceptor::~Acceptor()
{
    m_acceptChannel.disableAll();
    m_acceptChannel.remove();
}
void Acceptor::listen()
{
    m_listenning = true;
    m_acceptSocket.listen();                        //listen
    m_acceptChannel.enableReading();                //m_acceptChannel -> poller
}
/* listenfd有事件发生, 即有新连接 */
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = m_acceptSocket.accept(&peerAddr);
    if(connfd > 0)
    {
        if(m_newConnectionCallback)
        {
            /* 轮询找到subLoop, 唤醒, 分配当前新客户端的channel */
            m_newConnectionCallback(connfd, peerAddr);
        }
        else
        {
            close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err: %d", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE)
        {
            LOG_ERROR("(sockfd reached max limit)");
        }
        LOG_ERROR("\n");
    }
}