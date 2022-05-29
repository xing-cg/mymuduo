#include"timestamp.h"
#include<time.h>    //time(NULL)
Timestamp::Timestamp()
    : m_microSecondsSinceEpoch(0)
{

}
Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : m_microSecondsSinceEpoch(microSecondsSinceEpoch)
{
    
}
Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));
}
std::string Timestamp::toString() const
{
    char buf[128] = {0};
    /**
     * localtime 也是 time.h 中定义的函数
     * 函数原型：struct tm * localtime(const time_t *);
     *  struct tm
     *  {
     *      int tm_sec;     // [0-60] (1 leap second)
     *      int tm_min;     // [0-59]
     *      int tm_hour;    // [0-23]
     *      int tm_mday;    // [1-31]
     *      int tm_mon;     // [0-11]
     *      int tm_year;    // begin from 1900
     *      ...
     *  } 
     */
    struct tm * tm_time = localtime(&m_microSecondsSinceEpoch);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
            tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
            tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
    return buf;
}
#ifdef TEST_FOR_TIMESTAMP
#include<iostream>
int main()
{
    std::cout << Timestamp::now().toString() << std::endl;
    return 0;
}
#endif