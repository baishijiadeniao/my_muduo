#pragma once

#include "noncopyable.h"
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>


//打包sockaddr_in
class InetAddress
{
private:
    struct sockaddr_in address_;
public:
    explicit InetAddress(std::string ip="127.0.0.1",uint16_t port=0);
    explicit InetAddress(const struct sockaddr_in& addr):address_(addr){}
    struct sockaddr_in m_address(){ return address_;}
    //获取ip
    std::string toIP() const;
    //获取ip+端口
    std::string toIpPort() const;
    const sockaddr_in* getSockAddr() const { return &address_;}
    void setSockAddr (const sockaddr_in& addr){ address_=addr;}
};


