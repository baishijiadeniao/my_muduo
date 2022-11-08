#pragma once

#include "noncopyable.h"
#include "timestamp.h"


using namespace std;
//日志等级
enum level{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

class logger: noncopyable
{
private:
    level m_level;
    timestamp m_time;
    //单例模式，将构造函数和析构函数声明为私有；
    logger() = default;
    ~logger() = default;    
public:
    //单例模式，获取唯一实例
    static logger& get_instance(){
        static logger logger_;
        return logger_;
    }
    //设置日志等级
    void setmylevel(level level_);   
    //写日志
    void write_log(string str);
};

//宏定义函数，方便使用
#define INFO_LOG(format,...)                       \
    do{                                            \
        logger::get_instance().setmylevel(INFO);   \
        char buff[1024]={0};                        \
        sprintf(buff,format,##__VA_ARGS__);         \
        logger::get_instance().write_log(buff);    \
    }while(0)                                      

#define ERROR_LOG(format,...)                      \
    do{                                            \
        logger::get_instance().setmylevel(ERROR);  \
        char buff[1024]={0};                        \
        sprintf(buff,format,##__VA_ARGS__);         \
        logger::get_instance().write_log(buff);    \
    }while(0)

#define FATAL_LOG(format,...)                      \
    do{                                            \
        logger::get_instance().setmylevel(FATAL);  \
        char buff[1024]={0};                        \
        sprintf(buff,format,##__VA_ARGS__);         \
        logger::get_instance().write_log(buff);    \
    }while(0)

#ifdef MUDEBUG
#define DEBUG_LOG(format,...)                      \
    do{                                            \
        logger::get_instance().setmylevel(DEBUG);  \
        char buff[1024]={0};                        \
        sprintf(buff,format,##__VA_ARGS__);         \
        logger::get_instance().write_log(buff);    \
    }while(0)
#else
    #define DEBUG_LOG(format,...)  
#endif