#include "Channel.h"
#include<sys/epoll.h>

Channel::Channel(EventLoop *loop,int fd):loop_(loop),fd_(fd),events_(0),receive_(0),index_(0),tied_(false){
}

void Channel::update(){

}

void Channel::tie(const std::shared_ptr<void>& obj){
    tie_=obj;
    tied_ = true;
}

void Channel::handlewithEvent(timestamp receive_time){
    //弱引用指针提升为强引用
    shared_ptr<void> ptr=tie_.lock();
    if(tied_){
        if(ptr){
            handlewithGuard(receive_time);
        }
    }else{
        handlewithGuard(receive_time);
    }
}

void Channel::handlewithGuard(timestamp receive_time){
    if((receive_ & EPOLLHUP) && !(receive_ & EPOLLIN)){
        CloseCallBack();
    }
    if(receive_ & EPOLLERR){
        ErrorCallBack();
    }
    if(receive_ & EPOLLIN){
        ReadCallBack(receive_time);
    }
    if(receive_ & EPOLLOUT){
        WriteCallBack();
    }
}

void Channel::remove(){

}