#include "Buffer.h"
#include <sys/uio.h>
#include <errno.h>
#include<unistd.h>

size_t Buffer::readFd(int fd, int *saveError)
{
    const size_t writeable = writeableBytes();
    //使用栈，方便内存回收，而且速度快
    char extraBuf[65536] = {0};

    /* 使用readv可以操作多段非连续缓冲区，因为我们读之前不知道要读多少数据，
     所以如果buffer太小则造成缓冲区放不下数据的情况，如果buffer太大则会造成内存浪费
     使用多段缓冲区可以减少内存的浪费*/
    struct iovec vec[2];

    //iovec的第一个参数是缓冲区的起始地址，第二个参数是缓冲区的长度
    //注意这里是vector首地址而不能直接用迭代器buffer_.begin()
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writeable;

    vec[1].iov_base = extraBuf;
    vec[1].iov_len = sizeof(extraBuf);

    //一次最多只读64k的数据，如果buffer可写部分大于64k，那么只用buffer就可以了，即只用一个缓冲区
    const int iovcnt = (writeable < sizeof(extraBuf)) ? 2 : 1;
    const size_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saveError = errno;
    }
    //如果writeable够用，则移动可写标记为即可
    else if (n <= writeable)
    {
        writeIndex_ += n;
    }
    //如果writeable部分不够用则要扩容
    else
    {
        writeIndex_ = buffer_.size();
        append(extraBuf, n - writeable);
    }
    return n;
}


size_t Buffer::writeFd(int fd, int *saveError){
    ssize_t n = write(fd,peek(),readableBytes());
    if(n<0){
        *saveError=errno;
    }
    return n;
}