#include "Thread.h"
#include "CurrentThread.h"
#include<semaphore.h>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(Threadfunctor func,const std::string& name)
    :func_(std::move(func)),        //将func转化为右值，减少复制次数，提高性能
    name_(name),
    started_(false),
    join_(false),
    tid_(0){
        setDefaultName();
}

Thread::~Thread(){
    if(started_ && !join_){
        thread_->detach();        //分离线程，让内核自动回收线程
    }
}


void Thread::start(){
    started_=true;
    sem_t sem_;
    sem_init(&sem_,false,0);
    //开启线程，使用lambda表达式
    thread_= std::shared_ptr<std::thread>(new std::thread([&](){
        //获取线程的tid值
        tid_=CurrentThread::tid();
        //信号量+1
        sem_post(&sem_);
        //开启一个新线程，专门执行该线程函数
        func_();
    }));
    //必须等待获取新线程的tid
    sem_wait(&sem_);
}

void Thread::join(){
    join_=true;
    thread_->join();
}

void Thread::setDefaultName(){
    int num=++numCreated_;
    if(name_.empty()){
        char buf[32]={0};
        snprintf(buf,sizeof(buf),"Thread%d",num);
        name_=buf;
    }
}