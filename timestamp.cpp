#include "timestamp.h"

void timestamp::now(){
    time_t time_=time(NULL);
    m_tm=localtime(&time_);
    return;
}

string timestamp::toString(){
    this->now();
    char buf[64]={0};
    sprintf(buf,"%4d%02d%02d %02d:%02d\t",m_tm->tm_year+1900,m_tm->tm_mon+1,m_tm->tm_mday,m_tm->tm_hour,m_tm->tm_min);
    return buf;
}

