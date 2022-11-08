#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class timestamp;

using TcpConnectionPtr=std::shared_ptr<TcpConnection>;
using ConnectionCallBack=std::function<void(const TcpConnectionPtr&)>;
using CloseCallBack=std::function<void(const TcpConnectionPtr&)>;
using MessageCallBack=std::function<void(const TcpConnectionPtr&,Buffer*,timestamp)>;
using WriteCompletCallBack=std::function<void(const TcpConnectionPtr&)>;
using HightWaterMarkCallBack=std::function<void(const TcpConnectionPtr&,size_t)>;