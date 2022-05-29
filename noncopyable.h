#pragma once
/**
 * noncopyable 被继承以后，
 * 派生类对象无法拷贝构造、相互赋值。
 * 无参构造、析构是默认处理。
 */
class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable & operator=(const noncopyable &) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};