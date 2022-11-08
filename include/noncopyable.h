#pragma once

class noncopyable
{
private:
    /* data */
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) =delete;
    noncopyable() = default;
    ~noncopyable() = default;
};