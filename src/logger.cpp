#include "logger.h"
#include <iostream>
#include <string.h>
#include <string>

void logger::setmylevel(level level_){
    this->m_level=level_;
    return;
}

void logger::write_log(string str){
    char buf[64]={0};
    switch (m_level)
    {
    case INFO:
        strcpy(buf,"[INFO]\t");
        break;
    case ERROR:
        strcpy(buf,"[ERROR]\t");
        break;
    case FATAL:
        strcpy(buf,"[FATAL]\t");
        break;
    case DEBUG:
        strcpy(buf,"[DEBUG]\t");
        break;
    default:
        break;
    }
    cout<<buf<<m_time.toString()<<str<<endl;
}


