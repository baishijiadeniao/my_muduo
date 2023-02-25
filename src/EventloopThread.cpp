#include"EventloopThread.h"
#include"EventLoop.h"

EventloopThread::EventloopThread(const ThreadInitCallBack& cb,const std::string& name)
        :loop_(nullptr),
        exitting(false),
        thread_(std::bind(&EventloopThread::threadFunc,this),name),
        mutex_(),
        cond_(),
        CallBack_(cb){

}


EventloopThread::~EventloopThread(){
    exitting=true;
    if(loop_ != nullptr){
        loop_->quit();
        //退出线程
        thread_.join();
    }
}

//获取每个线程的loop
EventLoop* EventloopThread::startloop(){
    //创建一个线程执行Thread里的func，即下面的threadFunc
    thread_.start();
    //注意从这里开始分为两个线程，一个线程执行threadFunc，另一个线程执行下面的代码

    EventLoop* loop=nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        //如果loop_为空，说明上面创建的线程还没有创建完loop_所以主线程需要等待
        while(loop_ == nullptr){
            cond_.wait(lock);
        }
        loop=loop_;
    }
    return loop;
}

//这个方法是在单独的新线程中执行的
void EventloopThread::threadFunc(){
    EventLoop loop;     //创建一个eventloop，和上面的线程是一一对应的
    if(CallBack_){
        CallBack_(&loop);
    }

    //使用锁和条件变量保护，直到创建完loop
    {
    std::unique_lock<std::mutex> lock(mutex_);
    loop_=&loop;
    cond_.notify_one();
    }

    loop.loop();
    //退出loop，连接断开
    std::unique_lock<std::mutex> lock(mutex_);
    loop_=nullptr;
}