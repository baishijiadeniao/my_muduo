#include "Epoller.h"
#include <unistd.h>
#include<iostream>
#include <errno.h>
#include "logger.h"
#include <assert.h>
#include <string.h>
#include "logger.h"
#include "Channel.h"
#include "timestamp.h"
#include<sys/epoll.h>
const int knew=-1; //channel的成员index是-1
const int kadded=1;
const int kdeleted=2;

//注意这里要初始化基类对象，因为我们声明了Poller的构造函数，Poller没有默认构造函数，所以Poller不能用默认构造函数初始化对象
//我们必须显示地调用Poller带参的构造函数
Epoller::Epoller(EventLoop* loop):Poller(loop),events_(kInitEventListSize),
        epollfd_(::epoll_create1(EPOLL_CLOEXEC)){
        if(epollfd_<0){
            FATAL_LOG("EPOLL create failure: %d\n",errno);
        }
}

Epoller::~Epoller(){
    ::close(epollfd_);
}

void Epoller::updateChannel(Channel* channel){
    //调用update
    const int index=channel->index();
    if(index == knew || index == kdeleted){
        if(index==knew){
            int fd=channel->fd();
            channels_[fd]=channel;

        }
        channel->set_index(kadded);
        update(EPOLL_CTL_ADD,channel);
    }else{
        // channel已经在poller上注册过了
        int fd=channel->fd();
        if(channel->isnoneEvent()){
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kdeleted);
        }else {
            update(EPOLL_CTL_MOD,channel);
        }
    }
}

void Epoller::removeChannel(Channel* channel){
        //del事件
        int index=channel->index();
        int fd=channel->fd();
        channels_.erase(fd);
        if( index == kadded){
            update(EPOLL_CTL_DEL,channel);
        }
        channel->set_index(knew);
}

void Epoller::update(int operation,Channel* channel){
    int fd=channel->fd();
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.events=channel->Event();
    event.data.fd=fd;
    event.data.ptr=channel;
    std::cout<<"operation: "<<operation<<std::endl;
    if(::epoll_ctl(epollfd_,operation,fd,&event)<0){
        if(operation==EPOLL_CTL_DEL)
            ERROR_LOG("epoll_ctl del error:%d\n",errno);
        else
            FATAL_LOG("epoll_ctl add/mod error:%d\n",errno);
    }    
}

timestamp Epoller::poll(int timeoutMs,ChannelList* ChannelActiveList){
    int numEvent=::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
    //保存错误，防止被后续产生的错误刷新掉
    int saveErrno=errno;
    timestamp now_time;
    std::cout<<"numEvent: "<<numEvent<<std::endl;
    if(numEvent>0){
        //减少日志带来的性能开销
        DEBUG_LOG(" %d %s -> %s",numEvent,__FUNCTION__ ,"event happen");
        fillActivateChannels(numEvent,ChannelActiveList);
        
        if(events_.size()==numEvent){
            events_.resize(events_.size()*2);
        }
    }
    else if(numEvent==0){
        INFO_LOG("%s timeout\n",__FUNCTION__);
    }else{
        if(saveErrno !=EINTR){
            errno=saveErrno;
            ERROR_LOG("%s","epoll_wait failure");
        }
    }
    return now_time;
}

void Epoller::fillActivateChannels(int numEvents,ChannelList* EventList) const{
    for(int i=0;i<numEvents;i++){
        Channel* channel=static_cast<Channel*>(events_[i].data.ptr);
        std::cout<<"e "<<channel->Event()<<std::endl;
        std::cout<<"events_[i].events "<<events_[i].events<<std::endl;
        channel->set_revents(events_[i].events);
        EventList->push_back(channel);
    }
    std::cout<<"EventList.size() "<<EventList->size()<<std::endl;
}

