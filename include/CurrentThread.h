#pragma once

#include<unistd.h>
#include<sys/syscall.h>
namespace CurrentThread{
    extern __thread int t_cachedTid;
    void cachetid();
    inline int tid(){
        if (__builtin_expect(t_cachedTid==0,0))
        {
            cachetid();
        }
        return t_cachedTid;
    }
}