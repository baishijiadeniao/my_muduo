#include "Socket.h"
#include "logger.h"
#include<string.h>
#include<netinet/tcp.h>
#include<sys/socket.h>

Socket::~Socket(){}

void Socket::bindaddress(const InetAddress& localaddr){
    if(0 !=::bind(sockfd_,(struct sockaddr*)localaddr.getSockAddr(),sizeof(sockaddr_in))){
        FATAL_LOG("bind error:%d \n",sockfd_);
    }
}

void Socket::listen(){
    if(0 !=::listen(sockfd_,1024)){
        FATAL_LOG("listen error:%d \n",sockfd_);
    }
}

int Socket::accept(InetAddress* peeraddr){
    struct sockaddr_in address;
    socklen_t len=sizeof address;
    bzero(&address,sizeof address);
    //使用的是accept4,比accept多一个参数
    int connfd=::accept4(sockfd_,(struct sockaddr*)&address,&len,SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd >=0){
        peeraddr->setSockAddr(address);
    }
    return connfd;
}

void Socket::shutdownwrite(){
    if(0 != ::shutdown(sockfd_,SHUT_WR)){
        ERROR_LOG("shudown error:%d \n",sockfd_);
    }
}

//与nagle算法有关
void Socket::setTCPnoDelay(bool on){
    int optval= on ? 1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof optval);
}
//支持多个进程或者线程绑定到同一端口
void Socket::setReusePort(bool on){
    int optval= on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof optval);
}
//一般来说，一个端口释放后会等待两分钟之后才能再被使用，SO_REUSEADDR是让端口释放后立即就可以被再次使用。
void Socket::setReuseAddr(bool on){
    int optval= on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof optval);
}

void Socket::setKeepAlive(bool on){
    int optval= on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof optval);
}