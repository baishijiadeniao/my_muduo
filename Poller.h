#pragma once
#include "timestamp.h"
#include "Channel.h"
#include "noncopyable.h"
#include<unordered_map>

#include <vector>
using namespace std;
class EventLoop;

//EventLoop中的Poller类，是Epoller和poller类的基类
class Poller:noncopyable
{
private:
    EventLoop* loop_;
protected:
    //允许派生类访问
    using ChannelMap=unordered_map<int,Channel*>;
    ChannelMap channels_;
public:
    using ChannelList=vector<Channel*>;
    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    //为IO复用模型提供统一接口
    virtual timestamp poll(int timeoutMs,ChannelList* ChannelActiveList) =0;
    virtual void updateChannel(Channel*) =0;
    virtual void removeChannel(Channel*) =0;
    virtual bool hasChannel(Channel*) const;

    //作用类似于get_instance(),获取唯一IO复用派生类
    //Poller.cc文件中没有newDefaultPoller静态方法的实现，因为poller是基类，而实现newDefaultPoller方法需要引用派生类，基类引用派生类是不好的做法
    static Poller* newDefaultPoller(EventLoop* loop);
};
