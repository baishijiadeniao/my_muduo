#include "TcpConnection.h"
#include "logger.h"
#include "Channel.h"
#include "Socket.h"
#include "timestamp.h"
#include "InetAddress.h"
#include<string>
#include<memory>
#include<netinet/tcp.h>
#include<iostream>
#include<sys/socket.h>
#include<errno.h>

//设为静态，否则名字和TcpServer中的CheckLoopNotNull相同造成编译冲突
static EventLoop* CheckLoopNotNull(EventLoop *loop){
    if(loop == nullptr){
        FATAL_LOG("%s:%s:%d mainloop is null",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,
                  const std::string nameArg,
                  int sockfd,         //TcpServer提供
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr):
                  loop_(loop),
                  state_(kConnecting),
                  reading_(true),
                  name_(nameArg),
                  socket_(new Socket(sockfd)),
                  channel_(new Channel(loop,sockfd)),
                  localaddr_(localAddr),
                  peeraddr_(peerAddr),
                  hightWaterMark_(64*1024*1024)       //64M
                  {
                      channel_->setWriteCallBack(std::bind(&TcpConnection::handleWrite,this));
                      channel_->setCloseCallBack(std::bind(&TcpConnection::handleClose,this));
                      channel_->setErrorCallBack(std::bind(&TcpConnection::handleError,this));
                      channel_->setReadCallBack(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
                      INFO_LOG("TcpConnection ctor[%s] at fd=%d",name_.c_str(),sockfd);

                      socket_->setKeepAlive(true);
                  }

TcpConnection::~TcpConnection(){
    INFO_LOG("TcpConnection dtor[%s] at fd=%d state=%d \n",name_.c_str(),channel_->fd(),(int)state_);
}

 std::string TcpConnection::StateToString(int state) const{
    std::string buf;
    switch (state)
    {
    case kDisconnected:
         buf="kDisconnected";
    case KDisconnecting:
         buf="KDisconnecting";
    case kConnected:
         buf="kConnected";
    case kConnecting:
         buf="kConnecting";
    default:
         buf="unknown state";
    }
    return buf;
}

void TcpConnection::connectEstablished(){
    setState(kConnected);
    //绑定Channel和TcpConnection，使用弱引用智能指针确保TcpConnetction存在(如果tcpConnection不存在了则不执行相应的回调，比如客户端退出了就不执行回调了)
    channel_->tie(shared_from_this());
    channel_->enableReadEvent();
    std::cout<<"run to here4"<<std::endl;
    //建立连接的时候调用一次，关闭连接和handleclose的时候调用一次
    connectionCallBack_(shared_from_this());
}

void TcpConnection::connectDestroyed(){
    if(state_ == kConnected){
        setState(kDisconnected);
        channel_->disableAllEvent();

        connectionCallBack_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::shutdown(){
    if(state_==kConnected){
        setState(KDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,shared_from_this()));
    }
}

void TcpConnection::shutdownInLoop(){
    //当没有写事件注册时意味着所有数据都发送出去了
    if(!channel_->iswriting()){
        //关闭写端，调用全局shutdown方法
        socket_->shutdownwrite();
    }
}

//判断是TcpConnection否连接,已连接则调用sendInLoop
void TcpConnection::send(const std::string &buf){
    if(state_ == kConnected){
        if(loop_->isInLoopThread()){
            sendInLoop(buf.c_str(),buf.size());
        }else{
            loop_->queueInLoop(std::bind(&TcpConnection::sendInLoop,this,buf.c_str(),buf.size()));
        }
    }
}

/* 真正发送数据的函数，如果一次性发送不完则调用handleWrite回调，
因为发送数据，应用写得快，内核发送数据满，所以要设置缓冲区，并设置水位回调 */
void TcpConnection::sendInLoop(const void* message,size_t len){
    ssize_t nwrote;
    size_t remaining;
    bool faultError = false;
    if(state_ == kDisconnected){
        ERROR_LOG("disconnected,giving up writing");
        return;
    }
    //表示channel_第一次写数据，而且缓冲区中没有数据
    if(!channel_->iswriting() && outputBuffer_.readableBytes()==0){
        nwrote = ::write(channel_->fd(),message,len);
        if(len >= 0){
            remaining=len - nwrote;
            //既然这里数据都发送完了，那么就不用注册epollout事件了，也就不用调用handlewrite了
            if (remaining==0 && writeCompletCallBack_)
            {
                //为啥这里用queueInLoop而不是runInLoop，这不是就在自己线程里吗
                loop_->queueInLoop(std::bind(&TcpConnection::writeCompletCallBack_,shared_from_this()));
            }
        }else{
            nwrote=0;
            if(errno != EWOULDBLOCK){
                ERROR_LOG("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }
            }
        }
    }
    /*当前这一次write没有把所有的数据都发送出去，剩余没发送的数据保存到缓冲区，然后
       给channel注册epollout事件，poller发现tcp缓冲区有空余，会通知相应的channel,
       调用handlewrite回调*/
    if (!faultError && remaining >0)
    {
        //目前发送剩余的在缓冲区中的数据长度
        size_t oldLen= outputBuffer_.readableBytes();
        //如果oldLen >remaining说明上一次已经调用hightWaterMarkCallBack_了
        if(oldLen + remaining >= hightWaterMark_ && oldLen < hightWaterMark_ && hightWaterMarkCallBack_){
            loop_->queueInLoop(std::bind(&TcpConnection::hightWaterMarkCallBack_,shared_from_this()));
        }
        //将数据添加到缓冲区
        outputBuffer_.append((char*)message + nwrote,remaining);
        if (!channel_->iswriting())
        {
            //注册epollout事件
            channel_->enableWriteEvent();
        }
        
    }
    
}

//poller监听到有读事件发生的回调
void TcpConnection::handleRead(timestamp receiveTime){
    int saveError=0;
    ssize_t n=inputBuffer_.readFd(channel_->fd(),&saveError);
    if(n>0){
        //读事件后调用用户实现设置的回调
        messageCallBack_(shared_from_this(),&inputBuffer_,receiveTime);
    }else if(n==0){
        handleClose();
    }else{
        errno=saveError;
        ERROR_LOG("TcpConnection::handleRead");
        handleError();
    }
}

//poller监听到有写事件发生的回调
void TcpConnection::handleWrite(){
    //如果对写事件感兴趣的话则把outbuffer中的数据写到fd中
    if(channel_->iswriting()){
        int saveError=0;
        ssize_t n=outputBuffer_.writeFd(channel_->fd(),&saveError);
        if(n>0){
            //重置readIndex标志位
            outputBuffer_.retrieve(n);
            //readableBytes()==0,数据已经写完
            if (outputBuffer_.readableBytes()==0)
            {
                channel_->disableWriteEvent();
                if(writeCompletCallBack_){
                    //这里也可以不用queueInloop,可以直接执行，同上面messageCallBack。因为TcpConnection是在subloop中的。
                    loop_->queueInLoop(std::bind(writeCompletCallBack_,shared_from_this()));
                }
                //客户端断开或者服务器shutdown
                if(state_ == KDisconnecting){
                    shutdownInLoop();
                }
            }
        }else{
            ERROR_LOG("TcpConnection::handleWrite");
        }
    }else{
        ERROR_LOG("TcpConnection fd=%d is down,no more writing \n",channel_->fd());
    }
}

//poller=>Channel::closeCallBack=>TcpConnection::handleclose
//连接关闭后回调
void TcpConnection::handleClose(){
    INFO_LOG("TcpConnection:handleClose fd=%d state=%s",channel_->fd(),StateToString(state_).c_str());
    setState(kDisconnected);
    channel_->disableAllEvent();
    TcpConnectionPtr connPtr(shared_from_this());
    if (connectionCallBack_)
    {
        connectionCallBack_(connPtr);  //连接关闭的回调
    }
    if (closeCallBack_)
    {
        closeCallBack_(connPtr);   //关闭连接的回调,执行的是TcpServer::removeConnection的方法
    }
    
}

//真正的错误回调
void TcpConnection::handleError(){
    int optval;
    socklen_t optlen=sizeof(optval);
    int err=0;
    //返回socket发生的错误，使用一个名为so_error的变量记录对应的错误代码，存在optval中；作用和errno差不多，比errno更好
    if(getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen)<0){
        err=errno;
    }else{
        err=optval;
    }
    INFO_LOG("TcpConnection:handleError name=%s - ERROR: %d",name_.c_str(),err);
}

