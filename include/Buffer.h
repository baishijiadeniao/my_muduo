#pragma once

#include "noncopyable.h"
#include<string>
#include<vector>

class Buffer : noncopyable
{
public:
    //防止粘包
    static const size_t kCheapPrepend=8;
    //真正用于数据存放部分
    static const size_t kInitizeSize=1024;
    Buffer(size_t initialSize = kInitizeSize)
        :buffer_(kInitizeSize+kCheapPrepend),
        readerIndex_(kCheapPrepend),
        writeIndex_(kCheapPrepend){

    }
    ~Buffer(){};
    //可读数据的长度
    size_t readableBytes() const{
        return writeIndex_-readerIndex_;
    }
    //可写数据的长度
    size_t writeableBytes() const{
        return buffer_.size()-writeIndex_;
    }
    //返回可以开始读的索引
    size_t prependableBytes() const{
        return readerIndex_;
    }
    //返回缓冲区可读数据的起始地址
    const char* peek() const{
        return begin() + readerIndex_;
    }
    // buffer -> string 如果len小于可读数据长度，则将readerIndex_前进len个长度，否则则将readerIndex_和writeIndex_复位
    void retrieve(size_t len){
        if(len<readableBytes()){
            readerIndex_ +=len;
        }else{
            retrieveAll();
        }
    }
    //将可读和可写的索引复位
    void retrieveAll(){
        readerIndex_= kCheapPrepend;
        writeIndex_ = kCheapPrepend;
    }
    //把onMessage上报的buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString(){
        return retrieveAsString(readableBytes());
    }
    //将len长度的buffer转为string并返回
    std::string retrieveAsString(size_t len){
        std::string result(peek(),len);
        retrieve(len);
        return result;
    }
    //判断可写部分是否够用，不够大则调用makeSpace()扩容
    void ensureWriteableBytes(size_t len){
        if(writeableBytes()<len){
            //对buffer进行扩容
            makeSpace(len);
        }
    }
    //将数据写到buffer，buffer空间不够则扩容
    void append(const char* data,size_t len){
        ensureWriteableBytes(len);
        std::copy(data,data+len,buffer_.begin()+writeIndex_);
        writeIndex_+=len;
    }
    //读取fd中的数据，Buffer中最重要的函数
    size_t readFd(int fd,int* saveError);
    size_t writeFd(int fd,int* saveError);
private:
    //返回buffer_起始地址
    char* begin(){
        return &*buffer_.begin();
    }
    //用于常函数
    const char* begin() const{
        return &*buffer_.begin();
    }
    //扩容函数
    void makeSpace(size_t len){
        //| kCheapPrepend | void | reader |     void     |
        //| kCheapPrepend |        len        |
        //如果reader部分被读了一部分数据，那么前面一段buffer就会空出来，所以需要比较len部分和reader前面空出来的部分与reader后面可写部分的长度和
        if(writeableBytes()+prependableBytes()<len+kCheapPrepend){
            buffer_.resize(writeIndex_+len);
        }else{
            //要先将可读数据部分长度进行保存，因为后面的readerIndex_会被移动
            size_t readable=readableBytes();
            //将readerIndex_和writeIndex_之间的数据复制到kCheapPrepend位置（初始位置）
            std::copy(begin()+readerIndex_,begin()+writeIndex_,begin()+kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writeIndex_=kCheapPrepend+ readable;
        }
    }
    //使用vector放置buffer，方便扩容
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writeIndex_;
};
