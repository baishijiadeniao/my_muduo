#include "Acceptor.h"
#include "logger.h"
#include "InetAddress.h"
#include<unistd.h>
#include<iostream>
static int CreateNonBlockingOrDie(){
    int sockfd = socket(AF_INET,SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK,IPPROTO_TCP);
    if (sockfd<0)
    {
        FATAL_LOG("%s:%s:%d listen socket create error",__FILE__,__FUNCTION__,errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport)
    :loop_(loop),
    acceptSocket_(CreateNonBlockingOrDie()),
    acceptChannel_(loop,acceptSocket_.fd()),
    listening_(false){
        acceptSocket_.setReusePort(true);
        acceptSocket_.setReuseAddr(true);
        acceptSocket_.bindaddress(listenAddr);
        //TCPServer.start() acceptor.listen() acceptor监听到有新连接，执行一个回调，将fd打包成channel,然后通过轮询交给subloop的poller处理
        //注册baseloop的acceptchannel的读事件回调，有读事件来就可以执行回调
        acceptChannel_.setReadCallBack(std::bind(&Acceptor::handleread,this));
}

Acceptor::~Acceptor(){
    acceptChannel_.disableAllEvent();
    acceptChannel_.remove();
}

void Acceptor::listen(){
    listening_=true;
    acceptSocket_.listen(); 
    acceptChannel_.enableReadEvent();  //channel通知 poller开始监听读事件
}

void Acceptor::handleread(){
    InetAddress peerAddr;
    std::cout<<"run to here-1"<<std::endl;
    int connfd=acceptSocket_.accept(&peerAddr);
    std::cout<<"connfd: "<<connfd<<std::endl;
    if(connfd>=0){
        if(newConnectionCallBack){
            newConnectionCallBack(connfd,peerAddr);  //负责轮询、唤醒subloop，然后将channel分发给subloop
        }else{
            ::close(connfd);
        }
    }else{
        ERROR_LOG("%s:%s:%d accept error",__FILE__,__FUNCTION__,errno);
        //fd数量达到服务器上限
        if(errno==EMFILE){
        ERROR_LOG("%s:%s:%d sockfd reach limit",__FILE__,__FUNCTION__,errno);                
        }
    }
}