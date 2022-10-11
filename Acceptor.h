#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include <functional>
#include "InetAddr.h"

class EventLoop;

class Acceptor: noncopyable
{
public:
    using NewConnectionCallBack=function<void(int sockfd,const InetAddr&)>;
    Acceptor(EventLoop* loop,const InetAddr& listenAddr,bool reuseport);
    ~Acceptor();
    void setConnectionCallBack(const NewConnectionCallBack& cb){
        newConnectionCallBack=cb;
    }
    //让poller开始listen
    void listen();
    bool listening() const { return listening_;}
private:
    //注册在acceptchannel中的回调
    void handleread();
    EventLoop* loop_;
    //用来接收新连接的socket
    Socket acceptSocket_;
    //用来接收新连接的channel
    Channel acceptChannel_;
    bool listening_;
    //负责轮询、唤醒subloop，然后将channel分发给subloop
    NewConnectionCallBack newConnectionCallBack;
};


