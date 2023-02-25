#pragma once

#include"noncopyable.h"
#include <atomic>
#include<fcntl.h>
#include<memory>
#include<unistd.h>
#include<string>
#include<thread>
#include<functional>


//一个线程对象记录一个线程的具体信息
class Thread : noncopyable
{
private:
    using Threadfunctor=std::function<void()>;
    bool started_;
    bool join_;
    //原子变量，创建的线程数
    static std::atomic_int numCreated_;
    pid_t tid_;
    //不能直接new一个线程，因为直接创建线程线程就会立刻开始工作，所以先要创建指针
    std::shared_ptr<std::thread> thread_;
    Threadfunctor func_;
    std::string name_;
    //设置线程名字
    void setDefaultName();
public:
    explicit Thread(Threadfunctor,const std::string& name=std::string());
    ~Thread();
    //开启线程
    void start();
    //join线程
    void join();
    const std::string& name() const {return name_;}
    bool started() const { return started_;}
    bool joined() const { return join_;}
};
