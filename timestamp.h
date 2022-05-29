#pragma once

#include<iostream>

class Timestamp
{
public:
    Timestamp();
    /* 防止隐式转换 */
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    /* 获取当前时间，返回Timestamp对象 */
    static Timestamp now();
    /* 把Timestamp转换为 年月日时分秒 的字符串 */
    std::string toString() const;
private:
    int64_t m_microSecondsSinceEpoch;
};