#pragma once

#include "noncopyable.h"
#include "timestamp.h"
#include "EventLoop.h"
#include <functional>
#include<iostream>
#include <memory>
using namespace std;

//eventloop最重要的两个组件之一，负责将感兴趣的事件注册给poller，并接收poller返回的事件，并分发给相应的事件处理器
class Channel: noncopyable
{
public:
    Channel(EventLoop *loop,int fd);
    ~Channel();
    //定义回调函数形式
    using EventCallBack=std::function<void()>;
    using ReadEventCallBack=std::function<void(timestamp)>;
    
    int fd() const {return fd_;}
    int Event() const {return events_;};
    int index()const {return index_;}
    void set_index(int new_index){ index_=new_index;}
    void set_revents(int revent){ std::cout<<"revents_: "<<revents_<<std::endl; revents_ = revent ;}
    int get_revent() const{return revents_;}
    //在新连接建立的时候将TcpConnection和channel绑定，防止TcpConnetion不存在Channel调用TcpConnetion中相应的回调
    void tie(const std::shared_ptr<void>& obj);

    //调用事件对应的事件处理器
    void handlewithEvent(timestamp);
    void handlewithGuard(timestamp);
    //判断是否有事件,是否正在读写事件
    bool isnoneEvent() const {return events_==kNoneEvent;};
    bool iswriting() const{return events_ & kWriteEvent;}
    bool isreading() const{return events_ & kReadEvent;}

    //向poller注册感兴趣的事件
    void enableReadEvent() { events_ |= kReadEvent; update();};
    void disableReadEvent() { events_ &= ~kReadEvent; update();};
    void enableWriteEvent() { events_ |= kWriteEvent; update();};
    void disableWriteEvent(){ events_ &= ~kWriteEvent; update();};
    void disableAllEvent(){ events_ = kNoneEvent; update();};
    //Channel无法直接调用poller的函数，而是通过eventloop间接调用
    //Channel update() remove() -> eventloop updateChannel()removeChannel() -> Poller updateChannel()removeChannel()
    void update();

    //设置事件处理器的回调函数
    void setWriteCallBack(EventCallBack cb) {WriteCallBack=move(cb);};
    void setCloseCallBack(EventCallBack cb) {CloseCallBack=move(cb);};
    void setErrorCallBack(EventCallBack cb) {ErrorCallBack=move(cb);};
    void setReadCallBack(ReadEventCallBack cb) {ReadCallBack=move(cb);};

    weak_ptr<void> tie_;
    bool tied_;
    //删除fd
    void remove();
private:
    //poller监听的对象
    int fd_;
    //感兴趣的事件
    int events_;
    //正在发生的事件
    int revents_;
    EventLoop* loop_;
    int index_;
    //回调函数
    EventCallBack WriteCallBack;
    ReadEventCallBack ReadCallBack;
    EventCallBack CloseCallBack;
    EventCallBack ErrorCallBack;
    //一些读写事件
    static const int kReadEvent;
    static const int kWriteEvent;
    static const int kNoneEvent; 
};