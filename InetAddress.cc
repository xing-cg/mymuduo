#include"InetAddress.h"

#include<strings.h>     //bzero(&m_addr, sizeof m_addr)
#include<arpa/inet.h>   //inet_addr(IP.c_str())
#include<cstring>       //strlen
/**
 * @brief 初始化m_addr成员中的三个属性，ip、端口号、AF家族
 */
InetAddress::InetAddress(uint16_t port, std::string IP)
{
    bzero(&m_addr, sizeof m_addr);
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    /* inet_addr 有俩作用：1、把ip地址由字符串转为点分十进制格式。2、转为网络字节序 */
    m_addr.sin_addr.s_addr = inet_addr(IP.c_str());
}
/**
 * @brief 把m_addr中的sin_addr.s_addr(即ipv4地址)从网络字节序转为本地字节序
 */
std::string InetAddress::toIP() const
{
    char buf[64] = {0};
    inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof buf);
    return buf;
}
/**
 * @brief 把m_addr中的sin_port(即端口号)从网络字节序转为本地字节序
 */
uint16_t InetAddress::toPort() const
{
    return ntohs(m_addr.sin_port);
}
/**
 * @brief 把m_addr中的sin_addr.s_addr(即ipv4地址)从网络字节序转为本地字节序; 
 * IP地址存储到buf中，再把m_addr中的sin_port转为本地字节序; 
 * 最后用sprintf后面追加buf中。由此组成了IPport字符数组
 */
std::string InetAddress::toIPport() const
{
    char buf[64] = {0};
    inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    /* 把m_addr中的sin_port转为本地字节序，返回一个16无符号整型 */
    uint16_t port = ntohs(m_addr.sin_port);
    /* buf为存储IPport的字符串始址, end为有效ip长度, 用sprintf后面追加port */
    sprintf(buf+end, ":%u", port);
    return buf;
}
//#define TEST_FOR_INETADDRESS
#ifdef TEST_FOR_INETADDRESS
#include<iostream>
int main()
{
    InetAddress addr(8080);
    std::cout << addr.toIP() << std::endl;
    std::cout << addr.toPort() << std::endl;
    std::cout << addr.toIPport() << std::endl;
}
#endif