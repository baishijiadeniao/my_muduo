#pragma once

#include "noncopyable.h"
#include "InetAddr.h"

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd):sockfd_(sockfd){}
    ~Socket();
    void bindaddress(const InetAddr& localaddr);
    void listen();
    int accept(InetAddr* peeraddr);
    void shutdownwrite();
    int fd(){return sockfd_;}
    void setTCPnoDelay(bool on);
    void setReusePort(bool on);
    void setReuseAddr(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};