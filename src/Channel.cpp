#include "Channel.h"
#include "logger.h"
#include<sys/epoll.h>
#include<iostream>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop,int fd):loop_(loop),fd_(fd),events_(0),revents_(0),index_(-1),tied_(false){
}

Channel::~Channel(){

}

void Channel::update(){
    loop_->updateChannel(this);
}

void Channel::tie(const std::shared_ptr<void>& obj){
    tie_=obj;
    tied_ = true;
}

void Channel::handlewithEvent(timestamp revents_time){
    std::cout<<"run to here7"<<std::endl;
    if(tied_){
        //用于监听读写事件的Channel，与TcpConnection对应，直接调用回调函数
        //弱引用指针提升为强引用
        shared_ptr<void> ptr=tie_.lock();
        if(ptr){
            handlewithGuard(revents_time);
        }
    }else{
        std::cout<<"run to here8"<<std::endl;
        //用于监听连接的Channel，与Acceptor对应，直接调用回调函数
        handlewithGuard(revents_time);
    }
}

void Channel::handlewithGuard(timestamp revents_time){
    INFO_LOG("channel handleEvent revents:%d\n", revents_);
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){
        CloseCallBack();
    }
    if(revents_ & EPOLLERR){
        ErrorCallBack();
    }
    if(revents_ & (EPOLLIN | EPOLLPRI)){
        std::cout<<"run to here9"<<std::endl;
        ReadCallBack(revents_time);
    }
    if(revents_ & EPOLLOUT){
        WriteCallBack();
    }
}

void Channel::remove(){
    loop_->removeChannel(this);
}