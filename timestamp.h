#pragma once

#include "noncopyable.h"
#include<time.h>
#include<string>

using namespace std;


//获取当前时间的类
class timestamp
{
private:
    struct tm* m_tm;
public:
    explicit timestamp() = default;
    ~timestamp()= default;
    void now();
    //获取当前时间的字符串形式
    string toString();
};

