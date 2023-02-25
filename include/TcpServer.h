#pragma once

#include "noncopyable.h"
#include<string>
#include<memory>
#include<functional>
#include<atomic>
#include<unordered_map>
#include "CallBack.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "EventloopThreadPool.h"
#include "TcpConnection.h"

class EventLoop;

class TcpServer : noncopyable
{
public:
    using ThreadInitCallBack=std::function<void(EventLoop*)>;
    enum Option{
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop* loop,const InetAddress& listenAddr,const std::string& name,Option option = kNoReusePort);
    ~TcpServer();

    //开启服务器监听
    void start();
    //设置底层subloop的数量
    void setThreadNum(int numThreads);
    void setThreadInitCallBack(const ThreadInitCallBack& cb){ threadInitCallBack_=cb;};
    void setConnectionCallBack(const ConnectionCallBack& cb){ connectionCallBack_=cb;};
    void setMessageCallBack(const MessageCallBack& cb){ messageCallBack_=cb;};
    void setWriteCompletCallBack(const WriteCompletCallBack& cb){ writeCompletCallBack_=cb;};
private:
    //新建立连接
    void newConnection(int sockfd,const InetAddress& peerAddr);
    //删除连接
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);
    
    EventLoop* loop_;
    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventloopThreadPool> threadpool_;

    std::atomic_int start_;
    int nextConnId_;
    using ConnetcionMap=std::unordered_map<std::string,TcpConnectionPtr>;
    ConnetcionMap connections_;
    //新连接的回调
    ConnectionCallBack connectionCallBack_;
    //读事件的回调
    MessageCallBack messageCallBack_;
    //完成写的回调
    WriteCompletCallBack writeCompletCallBack_;
    //线程初始化回调
    ThreadInitCallBack threadInitCallBack_;
};