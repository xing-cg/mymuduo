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
    enum StateE{
        kDisconnected, kConnecting, kConnected, kDisconnecting
    };
    TcpConnection(EventLoop *loop, const std::string& name, int sockfd,
                  const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();
public:
    void connectEstablished();
    void connectDestoryed();
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
    void setState(StateE state) {m_state = state;}
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
    /**
     * 发送数据;
     * 用户会给TcpServer注册onMessageCallback, 
     * 已建立连接的用户有读写事件时, 尤其是读事件, onMessage会响应; 
     * 处理完客户端发来的事件后(onMessageCallback), 服务端会send给客户端回发消息; 
     * 
     * 收发数据的方式: 
     * 本项目的数据收发统一使用json或protobuf格式化的字符串进行, 
     * 所以此send函数的参数为了方便起见, 直接规定为string类型; 
     */
    void send(const std::string & buf);
private:
    /**
     * 发送数据;
     * 应用写的快, 而内核发送数据慢, 
     * 需要把待发送数据写入缓冲区, 而且设置了水位回调.
     */
    void sendInLoop(const void * data, size_t len);
private:
    void shutdownInLoop();
/*************************属性*************************/
private:
    ConnectionCallback	    m_connectionCallback;       //关于连接的回调
    MessageCallback	        m_messageCallback;          //已连接用户有读写事件发生时的回调
    WriteCompleteCallback   m_writeCompleteCallback;    //消息发送完成后的回调
    HighWaterMarkCallback   m_highWaterMarkCallback;    //高水位回调，为了控制收发流量稳定
    CloseCallback           m_closeCallback;            //关闭连接的回调
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
    
    std::atomic_int m_state;            //atomic，用枚举类变量赋值
    
    bool m_reading;
    
    size_t m_highWaterMark;             //高水位阈值
};