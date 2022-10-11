#include "Epoller.h"
#include <unistd.h>
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
Epoller::Epoller(EventLoop* loop):Poller(loop),loop_(loop),
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
    int index=channel->index();
    if(index == knew || index == kdeleted){
        int fd=channel->fd();
        if(index==knew){
            assert(channels_.find(fd) == channels_.end());
            channels_[fd]=channel;

        }else{
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd]==channel);
        }
        channel->set_index(kadded);
    }else{
        int fd=channel->fd();
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd]==channel);
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
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd]==channel);
        assert(index == kadded || index == kdeleted);
        if( index == kadded){
            update(EPOLL_CTL_DEL,channel);
        }
        channel->set_index(knew);
        channels_.erase(fd);
}

void Epoller::update(int operation,Channel* channel){
    int fd=channel->fd();
    struct epoll_event event;
    memset(&event,0,sizeof(event));
    event.events=channel->Event();
    event.data.ptr=channel;
    event.data.fd=fd;
    if(::epoll_ctl(epollfd_,operation,fd,&event)<0){\
        if(operation==EPOLL_CTL_DEL)
            ERROR_LOG("epoll_ctl error:%d\n",errno);
        else
            FATAL_LOG("epoll_ctl error:%d\n",errno);
    }    
}

timestamp Epoller::poll(int timeoutMs,ChannelList* ChannelActiveList){
    int numEvent=::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
    //保存错误，防止被后续产生的错误刷新掉
    int saveErrno=errno;
    timestamp now_time;
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

void Epoller::fillActivateChannels(int numEvents,ChannelList* EventList){
    for(int i=0;i<numEvents;i++){
        Channel* channel=static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revent(events_[i].events);
        EventList->push_back(channel);
    }
}

