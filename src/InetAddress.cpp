#include "InetAddress.h"
#include "logger.h"
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string>

InetAddress::InetAddress(string ip,uint16_t port){
    bzero(&address_,sizeof(address_));
    address_.sin_family=AF_INET;
    address_.sin_port=htons(port);
    address_.sin_addr.s_addr=inet_addr(ip.c_str());
}

std::string InetAddress::toIP() const{
    char buf[64]={0};
    ::inet_ntop(AF_INET,&address_.sin_addr,buf,sizeof(buf));
    return buf;
}

std::string InetAddress::toIpPort() const{
    char buf[64]={0};
    ::inet_ntop(AF_INET,&address_.sin_addr,buf,sizeof(buf));
    size_t end= strlen(buf);
    uint64_t port = ntohs(address_.sin_port);
    sprintf(buf+end,":%lu",port);
    return  buf;
}

// int main(){
//     InetAddress address_(9111,"192.168.10.8");
//     INFO_LOG("%s",address_.toIP().c_str());
//     ERROR_LOG("%s",address_.toIpPort().c_str());
//     DEBUG_LOG("%s","test");
// }