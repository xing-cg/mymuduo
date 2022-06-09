#pragma once
#include"noncopyable.h"
#include"inetaddress.h"
#include"buffer.h"
#include"callbacks.h"
#include<memory>
#include<atomic>
class EventLoop;
class Socket;
class Channel;
class TcpConnection : noncopyable,
               public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop, const std::string& name, int sockfd,
                  const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();
public:
    void connectEstablished();
    void connectDestoryed();
public:
    /* 发送数据 */
    void send(const void * message, int len);
public:
    /* 关闭连接 */
    void shutdown();
public:
    void setConnectionCallback(const ConnectionCallback & cb)
    {
        m_connectionCallback = cb;
    }
    void setMessageCallback(const MessageCallback & cb)
    {
        m_messageCallback = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback & cb)
    {
        m_writeCompleteCallback = cb;
    }
    void setHighWaterMarkCallback(const HighWaterMarkCallback & cb, size_t highWaterMark)
    {
        m_highWaterMarkCallback = cb;
        m_highWaterMark = highWaterMark;
    }
public:
    bool connected() const {return m_state == kConnected;}
public:
    EventLoop * getLoop() const {return m_loop;}
    const std::string& name() const {return m_name;}
    const InetAddress& localAddress() const {return m_localAddr;}
    const InetAddress& peerAddress() const {return m_peerAddr;}
private:
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
private:
    void sendInLoop(const void * message, size_t len);
private:
    void shutdownInLoop();
/*************************属性*************************/
private:
    EventLoop *m_loop;                  //subLoop
private:
    std::unique_ptr<Socket>  m_socket;  //unique_ptr管理
    std::unique_ptr<Channel> m_channel; //unique_ptr管理
private:
    const InetAddress m_localAddr;      //本地地址信息
    const InetAddress m_peerAddr;       //对端地址信息
private:
    Buffer m_inputBuffer;               //写缓冲区
    Buffer m_outputBuffer;              //读缓冲区
private:
    const std::string m_name;
    
    enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting};
    std::atomic_int m_state;            //atomic，用枚举类变量赋值
    
    bool m_reading;
    
    size_t m_highWaterMark;             //高水位阈值
private:
    ConnectionCallback	    m_connectionCallback;
    MessageCallback	        m_messageCallback;
    WriteCompleteCallback   m_writeCompleteCallback;
    HighWaterMarkCallback   m_highWaterMarkCallback;
    CloseCallback           m_closeCallback;
};