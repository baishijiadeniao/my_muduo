#include "EventLoop.h"
#include "Poller.h"
#include "logger.h"
#include "Channel.h"
#include <functional>
#include<sys/eventfd.h>
#include<iostream>
#include"CurrentThread.h"

using namespace std;

//判断线程中是否已有循环
__thread EventLoop* t_loopInThisThread=nullptr;
//poller IO多路复用连接时间
const int kPollTimeMs=10000;

int createEventfd(){
    int evefd=::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(evefd<0){
        FATAL_LOG("%s","Failed in eventfd");
    }
    return evefd;
}

EventLoop::EventLoop()
        :looping_(false),
        quit_(false),
        threadId_(CurrentThread::tid()),
        wakefd_(createEventfd()),
        wakeupchannel_(new Channel(this,wakefd_)),
        poller_(Poller::newDefaultPoller(this)),
        callingPendingFunctor_(false){
            //判断当前线程是否已有循环
            if(t_loopInThisThread){
                FATAL_LOG("another eventloop %p exists in this thread %d",t_loopInThisThread,threadId_);
            }else{
                t_loopInThisThread=this;
            }
            //不懂为什么缺少了ReadEventCallBack参数
            //设置wakeupfd事件监听类型和事件发生后的回调操作
            wakeupchannel_->setReadCallBack(bind(&EventLoop::handleRead,this));
            //每一个eventloop都将监听wakeupchannel_中的读事件
            wakeupchannel_->enableReadEvent();
}

EventLoop::~EventLoop(){
    //将事件设为none
    wakeupchannel_->disableAllEvent();
    //将channel从poller中删掉
    wakeupchannel_->remove();
    ::close(wakefd_);
    t_loopInThisThread=NULL;
}


void EventLoop::handleRead(){
    uint64_t one=1;
    ssize_t n=read(wakefd_,&one,sizeof one);
    if(n != sizeof one){
        ERROR_LOG("handleRead() reads %lu bytes instead of 8",n);
    }
}

//开始事件循环
//监听两类fd，一类是client的fd，一类是wakeupfd
void EventLoop::loop(){
    looping_=true;
    quit_=false;

    INFO_LOG("Eventloop %p starting loopping\n",this);
    while(!quit_){
        activeChannelList_.clear();
        pollReturnTime_=poller_->poll(kPollTimeMs,&activeChannelList_);
        std::cout<<"run to here12"<<std::endl;
        std::cout<<activeChannelList_.size()<<std::endl;
        for(Channel* channel:activeChannelList_){
            //poller监听channel发生了哪些事件，然后返回给eventloop，eventloop通知channel处理相应的事件
            channel->handlewithEvent(pollReturnTime_);
        }
        std::cout<<"run to here13"<<std::endl;        
        //执行当前eventloop事件循环需要处理的回调操作,queueInLoop执行的函数都放到PendingFunctors执行
        //mainloop负责accept事件，它事先注册一个回调，当唤醒subloop后，subloop执行下面方法，执行之前mainloop注册的回调
        doPendingFunctors();
    }
    INFO_LOG("Eventloop %p stopped\n",this);
}


//退出事件循环
/*两种情况，一种是loop在自己的loop中调用quit，；一种线程调用了不是由自己线程创建的loop的quit，
比如subloop的线程中调用了mainloop的quit，此时的quit_是属于mainloop的，因为不知道mainloop的情况，所以需要将
wakeup唤醒一下，否则如果没有唤醒而且mainloop正在poller_->poll()中阻塞的话就无法执行一圈退出循环了。
*/
void EventLoop::quit(){
    quit_=true;
    
    if(!isInLoopThread()){
        wakeup();                //如果quit不是自己的，就要唤醒quit_对应的loop让他走完一圈跳出while（loop()函数中的while）
    }
}

void EventLoop::updateChannel(Channel* channel){
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel){
    return poller_->hasChannel(channel);
}

void EventLoop::runInLoop(Functor cb){
    if(!t_loopInThisThread){
        cb();
    }else{
        queueInLoop(cb);  //在非当前loop线程中执行回调，需要唤醒loop所在线程
    }
}

//把cb放入队列中，唤醒loop所在的线程，，执行cb
void EventLoop::queueInLoop(Functor cb){
    {
        unique_lock<mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    /* 第二种情况中，如果callingPendingFunctor_为true则因为loop在执行回调，如果subloop
    执行完上一轮的回调后，subloop会返回到重新进行while循环，然后阻塞在poll上，就无法执行
    mainloop加到subloop上新的回调了，所以要把subloop唤醒。 */
    if(!t_loopInThisThread || callingPendingFunctor_){
        wakeup();
    }
}

//唤醒loop所在线程
/*mainloop
mainloop和subloop之间没有通过新建一个任务队列来进行间接通信，而是mainloop直接通过wakefd与subloop通信
subloop1 subloop2 subloop3
*/
void EventLoop::wakeup(){
    uint64_t one=1;
    ssize_t n=write(wakefd_,&one,sizeof(one));
    if(n != sizeof(one)){
        ERROR_LOG("wakeup write %lu bytes instead of 8",n);
    }
}


//执行回调
void EventLoop::doPendingFunctors(){
    /* 为什么这里需要再添加一个vector而不是直接使用pendingFunctors_，原因是subloop需要不断地从vector中取出回调并执行
    由于此时vector被锁控制，那么mainloop就无法及时地向vector添加回调，造成时延，所以新建一个vector使添加vector操作和
    取出vector操作不用互相等待*/
    vector<Functor> functors;
    callingPendingFunctor_ =true;
    {
        unique_lock<mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    
    for(const Functor &functor : functors){
        functor();
    }

    callingPendingFunctor_ = false;
}
