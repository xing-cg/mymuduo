#pragma once
#include<unistd.h>          //pid_t  syscall
#include<sys/syscall.h>     //SYS_gettid
namespace CurrentThread
{
    /**
     * @brief 此变量被__thread修饰, 相当于C++11标准中的thread_local修饰符, 
     * 用于修饰全局变量。修饰之前, 全局变量只能被若干线程共享; 
     * 修饰之后, 此全局变量变成每个线程专有的属性。
     */
    extern __thread int t_cachedTid;
    /* 通过Linux系统调用SYS_gettid, 加载当前线程的tid值到t_cachedTid */
    void cacheTid()
    {
        if(t_cachedTid == 0)
        {
            t_cachedTid = static_cast<pid_t>(syscall(SYS_gettid));
        }
    }
    /* 返回当前线程的tid, 若加载过则直接返回存储过的值 */
    inline int tid()
    {
        /* 若t_cachedTid为0说明是第一次加载, 需要调用cacheTid */
        if(__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}