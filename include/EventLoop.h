#pragma once

#include "noncopyable.h"
#include "CurrentThread.h"
#include "timestamp.h"
#include <atomic>
#include<fcntl.h>
#include<functional>
#include<vector>
#include<mutex>
#include<memory>
class Channel;
class Poller;
using namespace std;

class EventLoop : noncopyable
{
private:
    int wakefd_;
    unique_ptr<Channel> wakeupchannel_;
    unique_ptr<Poller> poller_;
    //线程id
    const pid_t threadId_;
    //poller返回发生事件的channels的时间点
    timestamp pollReturnTime_;

    //使用指针不用分配大量内存空间
    using ChannelList=vector<Channel*>;
    using Functor=function<void()>;
    ChannelList activeChannelList_;
    // Channel* currentActiveChannel_;

    atomic_bool looping_; //判断是否开始循环 ,原子操作，通过CAS实现
    atomic_bool quit_; //标识退出loop循环

    mutex mutex_; //互斥锁，用来保护上面的vector容器
    atomic_bool callingPendingFunctor_;  //判断是否有需要执行的回调操作
    vector<Functor> pendingFunctors_; //用来存储所有需要执行的回调操作 
    
    void handleRead();   //唤醒其他subloop
    void doPendingFunctors(); //执行回调
public:
    EventLoop();
    ~EventLoop();
    //调用poller的相应方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);
    //在当前线程执行cb
    void runInLoop(Functor cb);
    //放到队列中，由其他线程执行cb
    void queueInLoop(Functor cb);
    //判断是否在当前线程,如果是则当前eventloop对象在创建它的thread里，如果不是则不在创建它的thread里，需要调用queueloop唤醒创建它的线程
    bool isInLoopThread ()const { return threadId_==CurrentThread::tid();}
    //唤醒loop所在的线程
    void wakeup();
    //开启事件循环
    void loop();
    //退出事件循环
    void quit();
};

