#pragma once
#include "noncopyable.h"
#include "Poller.h"
#include "EventLoop.h"
#include<vector>
#include<sys/epoll.h>
using namespace std;


class EventLoop;
class Channel;

//Poller类的IO多路复用子类
class Epoller : public Poller
{
private:
    //初始化EventList数组的长度
    static const int kInitEventListSize = 16;
    //存放事件的列表，通过vector保存，方便扩容
    using EventList = vector<struct epoll_event>;
    EventList events_;
    int epollfd_;
    
    //向eventloop返回活跃的channel列表
    void fillActivateChannels(int numEvents,ChannelList*) const;
    //调用Epoll_ctl注册事件
    void update(int operation,Channel* channel);
public:
    Epoller(EventLoop* loop);
    ~Epoller() override;
    //重载Poller类的纯虚函数
    void updateChannel(Channel*) override;
    void removeChannel(Channel*) override;

    //使用epoll_wait监听事件
    timestamp poll(int timeoutMs,ChannelList* ChannelActiveList) override;
};