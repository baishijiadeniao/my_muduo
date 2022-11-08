#include "TcpServer.h"
#include "logger.h"
#include "TcpConnection.h"
#include "InetAddress.h"
#include<string>
#include<strings.h>
#include<sys/socket.h>
#include<iostream>
#include<netinet/in.h>

EventLoop* CheckLoopNotNull(EventLoop *loop){
    if(loop == nullptr){
        FATAL_LOG("%s:%s:%d mainloop is null",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop,const InetAddress& listenAddr,const std::string& name,Option option)
    :loop_(CheckLoopNotNull(loop)),
    acceptor_(new Acceptor(loop,listenAddr,option==kReusePort)),     //Acceptor只运行在mainloop
    threadpool_(new EventloopThreadPool(loop,name)),
    name_(name),
    ipPort_(listenAddr.toIpPort()),
    connectionCallBack_(),
    nextConnId_(1),
    start_(0)
    {   
        //当有新用户连接时，会执行newConnection回调
        acceptor_->setConnectionCallBack(std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));
}

TcpServer::~TcpServer(){
    for(auto& item:connections_){
        //如果直接reset的话就无法访问TcpConnection::connDestroyed，这个局部的TcpConnectionPtr出了这个大括号可以自动被释放
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        //销毁连接
        conn->get_loop()->runInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
    }
}

//启动监听
void TcpServer::start(){
    //防止一个TcpServer多次start
    if(start_++ ==0){
        threadpool_->start(threadInitCallBack_);    //启动底层threadpool
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}

//设置线程数量（底层subloop数量）
void TcpServer::setThreadNum(int numThreads){
    threadpool_->setThreadNum(numThreads);
}

void TcpServer::newConnection(int sockfd,const InetAddress& peerAddr){
    //轮询得到可用的subloop
    std::cout<<"run to here"<<std::endl;
    EventLoop* ioLoop=threadpool_->getNextloops();
    char buf[64]= {0};
    snprintf(buf,sizeof(buf),"-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    string connName= name_ + buf;
    std::cout<<"run to here1"<<std::endl;
    INFO_LOG("TcpServer::newConnection[%s] - new connection [%s] from %s \n",name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());

    struct sockaddr_in local;
    ::bzero(&local,sizeof(local));
    socklen_t addrlen=sizeof(local);
    //获取sockfd的信息并打包成InetAddress
    if(::getsockname(sockfd,(struct sockaddr*)&local,&addrlen)<0){
        ERROR_LOG("%s","TcpServer::getlocalAddr");
    }

    InetAddress localAddr(local);

    //根据连接成功的sockfd，创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop,connName,sockfd,localAddr,peerAddr));

    //添加到存放TcpConnection的map中
    connections_[connName]=conn;

    //下面的回调是用户设置给TcpServer->TcpConnection->Channel->Poller
    conn->setConnectionCallBack(connectionCallBack_);
    conn->setMessageCallBack(messageCallBack_);
    conn->setWriteCompletCallBack(writeCompletCallBack_);
    //设置了如何关闭连接的回调
    conn->setCloseCallBack(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));
    std::cout<<"run to here2"<<std::endl;
    //建立连接
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn){
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}

/* 连接关闭的调用顺序： poller监听到连接断开 => Channel调用closeCallBack => 即TcpConnection::handleClose => TcpServer::removeConnection 
   => TcpServer::removeConnectionInLoop 将TcpConnection从map表中删掉 => TcpConnection::connectDestroyed关掉所有事件 => Channel::remove*/
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn){
    INFO_LOG("TcpServer::removeConnectionInLoop [%s] - connection %s",name_.c_str(),conn->name().c_str());

    //在map中删除连接
    connections_.erase(conn->name());

    EventLoop * ioLoop=conn->get_loop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
}