#pragma once

#include "noncopyable.h"
#include "InetAddr.h"
#include "Buffer.h"
#include "CallBack.h"
#include<memory>
#include<atomic>

class EventLoop;
class Channel;
class Socket;

/* TcpServer =》 Acceptor => 有新用户连接，通过accept函数拿到connfd 
    => TcpConnection设置回调 => 设置给Channel => 注册到Poller，检测到读事件 => Channel的回调操作*/
/*函数调用顺序： 构造函数将TcpConnection的回调注册到Channel
    connectEstablished建立连接，注册读事件,connectDestroyed 销毁连接，删除所有注册的事件
    send判断是否连接->sendInLoop第一次发不完->不断注册epollout，调用handlewrite直到发送完
    shutdown：调用shutdInLoop，关闭socket      */
//TcpConnection相当于mainloop中的acceptor
class TcpConnection:noncopyable , public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                  const std::string nameArg,
                  int sockfd,         //TcpServer提供
                  const InetAddr& localAddr,
                  const InetAddr& peerAddr);
    ~TcpConnection();

    const std::string& name(){ return name_;}
    const InetAddr& localAddr(){return localaddr_;}
    const InetAddr& peerAddr(){return peeraddr_;}
    bool isReading() const {return reading_;}
    
    //发送数据
    void send(void* message,size_t len);
    //关闭连接
    void shutdown(); 

    //设置回调
    void setConnectionCallBack(const ConnectionCallBack& cb){
        connectionCallBack_ = cb;
    }
    void setMessageCallBack(const MessageCallBack& cb){
        messageCallBack_ = cb;
    }
    void setWriteCompletCallBack(const WriteCompletCallBack& cb){
        writeCompletCallBack_ = cb;
    }
    void setHightWaterMarkCallBack(const HightWaterMarkCallBack& cb){
        hightWaterMarkCallBack_ = cb;
    }
    void setCloseCallBack(const CloseCallBack& cb){
        closeCallBack_ = cb;
    }

    EventLoop* get_loop() const {return loop_;}
    bool connected(){return state_==kConnected;}

    //建立连接，注册读事件
    void connectEstablished();
    //销毁连接，删除所有注册的事件
    void connectDestroyed();

    //发送数据
    void send(const std::string &buf);
    void sendInLoop(const void* message,size_t len);
private:
    //连接状态
    enum StateE{
        kDisconnected,
        KDisconnecting,
        kConnected,
        kConnecting
    };
    void setState(StateE state){ state_=state;}
    std::string StateToString(int state) const;
    //poller监听到有读事件发生的回调，作用同Acceptor中的handleRead
    void handleRead(timestamp receiveTime);
    //poller监听到有写事件发生的回调
    void handleWrite();
    void handleClose();
    void handleError();

    //新连接的回调
    ConnectionCallBack connectionCallBack_;
    //读事件的回调
    MessageCallBack messageCallBack_;
    //完成写的回调
    WriteCompletCallBack writeCompletCallBack_;
    //关闭连接的回调
    CloseCallBack closeCallBack_;
    HightWaterMarkCallBack hightWaterMarkCallBack_;
    size_t hightWaterMark_;
    
    void shutdownInLoop(); 

    EventLoop* loop_;      //TcpConnection只能作用于subloop中，不可以是baseloop
    const std::string name_;
    std::atomic_int state_; 
    bool reading_;

    //和acceptor类似 Acceptor=>mainloop TcpConnection=>subloop
    std::unique_ptr<Channel> channel_;
    std::unique_ptr<Socket> socket_;
    const InetAddr localaddr_;
    const InetAddr peeraddr_;

    //用于存放读入数据的缓冲区
    Buffer inputBuffer_;
    //用于存放发送数据的缓冲区
    Buffer outputBuffer_;
};