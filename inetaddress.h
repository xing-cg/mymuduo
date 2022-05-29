#pragma once
#include<netinet/in.h>      //sockaddr_in
#include<string>
/**
 * @brief 封装socket地址类型
 * 
 */
class InetAddress
{
public:
    explicit InetAddress(uint16_t port, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)
        : m_addr(addr)
    {

    }
    std::string toIP() const;
    std::string toIPport() const;
    uint16_t toPort() const;
    const sockaddr_in* getSockAddr() const {return &m_addr;};
private:
    sockaddr_in m_addr;
};