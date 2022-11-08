#pragma once

#include "noncopyable.h"
#include "InetAddress.h"

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd):sockfd_(sockfd){}
    ~Socket();
    void bindaddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress* peeraddr);
    void shutdownwrite();
    int fd(){return sockfd_;}
    void setTCPnoDelay(bool on);
    void setReusePort(bool on);
    void setReuseAddr(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};