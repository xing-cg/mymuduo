#pragma once
#include<vector>
#include<string>
/**
 * @brief 网络库底层的缓冲区类型定义
 */
class Buffer
{
public:
    static const size_t kCheapPrepend = 0;
    static const size_t kInitialSize = 1024;
public:
    explicit Buffer(size_t initialSize = kInitialSize)
        : m_buffer(kCheapPrepend + initialSize),
          m_readerIndex(kCheapPrepend),
          m_writerIndex(kCheapPrepend)
    {
    }
    ~Buffer() = default;
public:
    size_t readableBytes() const
    {
        return m_writerIndex - m_readerIndex;
    }
    size_t writableBytes() const
    {
        return m_buffer.size() - m_writerIndex;
    }
    size_t prependableBytes() const
    {
        return m_readerIndex;
    }
private:
    /* begin - 获取buffer实际首部指针 */
    char * begin()
    {
        return &*m_buffer.begin();
    }
    /* begin - 获取buffer实际首部指针 */
    const char * begin() const
    {
        return &*m_buffer.begin();
    }
public:
    /* 返回缓冲区数据包中可读数据起始位置 */
    const char * peek() const
    {
        return begin() + m_readerIndex;
    }
    /* 返回可写的数据起始位置 */
    char * beginWrite()
    {
        return begin() + m_writerIndex;
    }
    /* 返回可写的数据起始位置 */
    const char * beginWrite() const
    {
        return begin() + m_writerIndex;
    }
public:
    /* 将m_readerIndex和m_writerIndex调整位置 */
    void retrieve(size_t len)
    {
        if(len < readableBytes())//只读取了可读缓冲区数据的一部分
        {
            m_readerIndex += len;
        }
        else
        {
            retrieveAll();
        }
    }
    /* 将m_readerIndex和m_writerIndex调整位置 */
    void retrieveAll()
    {
        m_readerIndex = m_writerIndex = kCheapPrepend;
    }
    /* 把buffer中的数据转为string类型，多与onMessage配合使用 */
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }
    /* 把buffer中的数据转为string类型，多与onMessage配合使用 */
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }
public:
    /* 确保buffer可写空间大小足够len，不足则扩容 */
    void ensureWritableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            makeSpace(len);
        }
    }
public:
    void append(const char * data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        m_writerIndex += len;
    }
public:
    ssize_t readFd(int fd, int * savedErrno);
private:
    std::vector<char> m_buffer;
    size_t m_readerIndex;
    size_t m_writerIndex;
private:
    void makeSpace(size_t len)
    {
        if(writableBytes()+prependableBytes()-kCheapPrepend < len)
        {
            m_buffer.resize(m_writerIndex + len);
        }
        else//move readable data to the front to make space
        {
            size_t readable = readableBytes();
            //将m_readerIndex到m_writerIndex的内容复制到kCheapPrepend处
            std::copy(begin() + m_readerIndex, begin() + m_writerIndex, begin() + kCheapPrepend);
            m_readerIndex = kCheapPrepend;
            m_writerIndex = m_readerIndex + readable;
        }
    }
};