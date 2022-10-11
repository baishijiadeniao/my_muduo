#pragma once

#include"noncopyable.h"
#include"EventloopThread.h"
#include<string>
#include<vector>
#include<memory>

class EventLoop;

class EventloopThreadPool : noncopyable
{
public:
    using ThreadInitCallBack=std::function<void(EventLoop*)>;
    EventloopThreadPool(EventLoop* baseloop,const std::string& nameArg);
    ~EventloopThreadPool();
    //创建EventLoopThread
    void start(const ThreadInitCallBack& cb=ThreadInitCallBack());
    void setThreadNum(int ThreadNum){ThreadNum_=ThreadNum; }
    std::vector<EventLoop*> getAllLoops();
    //通过轮询方式将channel分配给subloop
    EventLoop* getNextloops();
    bool started() const { return started_;}
    const std::string name() const { return name_;}
private:
    EventLoop* baseloop_;   //如果没有设置多线程，那么baseloop既负责连接又负责读写
    bool started_;
    std::string name_;
    int next_;  //轮询下标
    int ThreadNum_;
    std::vector<std::unique_ptr<EventloopThread>> threads_;   //包含所有创建的线程
    std::vector<EventLoop*> loops_;
};