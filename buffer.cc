#include"buffer.h"
#include<sys/uio.h>
ssize_t Buffer::readFd(int fd, int * savedErrno)
{
    char extrabuf[65536] = {0};	//栈上内存空间
    struct iovec iov[2];
    const size_t writable = writableBytes();//buffer底层缓冲区剩余的可写空间大小
    iov[0].iov_base = begin() + m_writerIndex;
    iov[0].iov_len = writable;
    iov[1].iov_base = extrabuf;
    iov[1].iov_len = sizeof extrabuf;
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    //如果可写空间大小少于64kb则可以按需写到vec[0]/vec[1];
    //如果可写空间大小大于等于64kb则只能写到vec[0]。
    //说明, 可收到的数据大小限制至少为64kb。
    const ssize_t n = ::readv(fd, iov, iovcnt);
    if(n < 0)
    {
        *savedErrno = errno;
    }
    else if(n <= writable)
    {
        m_writerIndex += n;
    }
    else// n > writable, 需要拼接extrabuf
    {
        m_writerIndex = m_buffer.size();
        append(extrabuf, n-writable);
    }
    return n;
}