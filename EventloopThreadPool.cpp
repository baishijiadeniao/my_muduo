#include "EventloopThreadPool.h"
#include "EventloopThread.h"

EventloopThreadPool::EventloopThreadPool(EventLoop *baseloop, const std::string &nameArg)
    : next_(0),
      started_(false),
      ThreadNum_(0),
      baseloop_(baseloop),
      name_(nameArg)
{
}

EventloopThreadPool::~EventloopThreadPool()
{
}

void EventloopThreadPool::start(const ThreadInitCallBack &cb)
{
        started_ = true;
        for (int i = 0; i < ThreadNum_; i++)
        {
                char tmp[name_.size() + 32];
                snprintf(tmp, sizeof(tmp), "%s%d", name_.c_str(), i);
                EventloopThread *t = new EventloopThread(cb, tmp);
                threads_.push_back(std::unique_ptr<EventloopThread>(t));
                loops_.push_back(t->startloop()); //底层创建新线程，绑定一个新的Eventloop，，并返回loop
        }
        //如果服务端只有一个线程运行着baseloop，则直接运行cb
        if (loops_.empty() && cb)
        {
                cb(baseloop_);
        }
}

std::vector<EventLoop *> EventloopThreadPool::getAllLoops()
{
        if (loops_.empty())
        {
                return std::vector<EventLoop *>(1, baseloop_);
        }
        else
        {
                return loops_;
        }
}

//如果运行在多线程中，则通过轮询方式将channel分配给subloop
EventLoop *EventloopThreadPool::getNextloops()
{
        EventLoop *loop = baseloop_;
        if (!loops_.empty())
        {
                loop=loops_[next_];
                ++next_;
                if(next_ >= loops_.size()){
                        next_=0;
                }
        }
        return loop;
}