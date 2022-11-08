#include"../include/EventLoop.h"
#include"../include/TcpServer.h"
#include"../include/InetAddress.h"

// #include<muduo/net/EventLoop.h>
// #include<muduo/net/TcpServer.h>
// #include<muduo/net/InetAddressess.h>
#include<iostream>

// using namespace muduo;
// using namespace muduo::net;

/*
1、组合TcpServer对象
2、创建EventLoop事件循环对象的指针
3、明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4、在当前服务器类的构造函数中，注册处理连接和处理读写事件的回调函数
5、设置合适的服务端线程数量，muduo库会自己分配I/O线程和工作线程
*/
class ChatServer
{
public:
    //构造函数设置回调
    ChatServer(EventLoop* loop,const InetAddress& listenAddr,const string& nameArg):loop_(loop),tcpserver_(loop_,listenAddr,nameArg){
        tcpserver_.setConnectionCallBack(std::bind(&ChatServer::onConnection,this,std::placeholders::_1));
        //_1,_2,_3是占位符，muduo库内部调用时会填充这些参数
        tcpserver_.setMessageCallBack(std::bind(&ChatServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
        //4个线程,1个I/O线程，3个工作线程
        tcpserver_.setThreadNum(4);
    };
    void start(){
        tcpserver_.start();
    }
    ~ChatServer(){};
private:
    //专门处理用户的连接建立和断开事件
    void onConnection(const TcpConnectionPtr& conn){
        if(conn->connected()){
            std::cout<<"from "<<conn->peerAddress().toIpPort()<<"to "<<
            conn->localAddress().toIpPort()<<" connected"<<std::endl;
        }else{
            std::cout<<"from "<<conn->peerAddress().toIpPort()<<"to "<<
            conn->localAddress().toIpPort()<<" disconnected"<<std::endl;
            conn->shutdown();
        }
    }
    //专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            timestamp timestamp){
        std::string buff=buffer->retrieveAllAsString();
        std::cout<<"reveive data from "<<conn->peerAddress().toIpPort()<<" : "<<buff<<std::endl;
        conn->send(buff);
    }
    EventLoop* loop_;
    TcpServer tcpserver_;
};


int main(){
    EventLoop loop;
    InetAddress listenaddr("127.0.0.1",6000);
    ChatServer chatserver(&loop,listenaddr,"my_server");
    chatserver.start();
    //epoll_wait以阻塞的方式等待新用户的连接或读写事件的发生等
    loop.loop();
}

//编译：  g++ ./use_muduo.cpp  -o use_muduo -lmuduo_net -lmuduo_base -lpthread -std=c++11 
//-lpthread要放在 -lmuduo_net -lmuduo_base 后面