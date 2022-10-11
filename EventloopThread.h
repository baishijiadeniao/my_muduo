#pragma once

#include"noncopyable.h"
#include"Thread.h"
#include<functional>
#include<mutex>
#include<condition_variable>
#include<string>

class EventLoop;

class EventloopThread : noncopyable
{
public:
    //线程初始化回调
    using ThreadInitCallBack=std::function<void(EventLoop*)>;
    EventloopThread(const ThreadInitCallBack& cb=ThreadInitCallBack(),const std::string& name=std::string());
    ~EventloopThread();
    EventLoop* startloop();
private:
    //创建loop
    void threadFunc();

    Thread thread_;
    ThreadInitCallBack CallBack_;
    //是否退出循环
    bool exitting;
    std::mutex mutex_;
    std::condition_variable cond_;
    EventLoop* loop_;
};