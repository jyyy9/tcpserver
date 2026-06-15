#include "buffer.h"
#include <cassert>
#include <cstring>
#include <sys/uio.h>

namespace net {
    Buffer::Buffer()
        : _buffer(kInitialSize + kCheapPrepend)
        , _readerIndex(kCheapPrepend)
        , _writerIndex(kCheapPrepend){}
    Buffer::Buffer(int initSize)
        : _buffer(initSize + kCheapPrepend)
        , _readerIndex(kCheapPrepend)
        , _writerIndex(kCheapPrepend){}
    Buffer::Buffer(const Buffer& other)
        : _buffer(other._buffer)
        , _readerIndex(other._readerIndex)
        , _writerIndex(other._writerIndex){ }
    Buffer::Buffer(Buffer&& other){
        Buffer tmp;
        tmp.swap(other);
        tmp.swap(*this);
    }
    Buffer& Buffer::operator=(const Buffer& other){
        Buffer tmp(other);
        tmp.swap(*this);
        return *this;
    }
    Buffer& Buffer::operator=(Buffer&& other){
        Buffer tmp(std::move(other));
        tmp.swap(*this);
        return *this;
    }

    void Buffer::swap(Buffer& other){
        _buffer.swap(other._buffer);
        std::swap(_readerIndex, other._readerIndex);
        std::swap(_writerIndex, other._writerIndex);
    }
    // read data size
    size_t Buffer::readAbleBytes(){ return _writerIndex - _readerIndex; }
    // write idle space size
    size_t Buffer::writeAbleBytes(){ return _buffer.size() - _writerIndex; } 
    // prepend idle space size
    size_t Buffer::prependAbleBytes(){ return _readerIndex; } 
    //  get read position
    const char* Buffer::peek() const{ return begin() + _readerIndex; }
    char* Buffer::peek(){ return begin() + _readerIndex; } 
    // move _readerIndex
    void Buffer::retrive(size_t len){ 
        assert(len <= readAbleBytes());
        _readerIndex += len;
    } 
    std::string Buffer::retriveAllAsString(){
        return retriveAsString(readAbleBytes());
    }
    std::string Buffer::retriveAsString(size_t len){
        assert(len <= readAbleBytes());
        std::string retval(peek(), len);
        retrive(len);
        return retval;
    }
    int32_t Buffer::peekInt32(){
        int32_t data;
        // memcpy(&data, peek(), sizeof(int32_t));
        std::copy(peek(), peek() + sizeof(int32_t), (char*)&data);
        return data;
    } // peek get data as int32

    char *Buffer::beginWrite(){ return begin() + _writerIndex; } // get write position
    void Buffer::hasWriten(size_t len){
        assert(len <= writeAbleBytes());
        _writerIndex += len;
    } // move _writerIndex
    void Buffer::append(const void *data, size_t len){
        ensureWritableBytes(len); // 
        std::copy((const char*)data, 
                  (const char*)data + len, 
                  beginWrite());
        hasWriten(len);
    }
    void Buffer::append(const std::string &data){
        append(data.data(), data.size());
    }
    void Buffer::prepend(const void *data, size_t len){
        assert(len <= prependAbleBytes());
        _readerIndex -= len;
        std::copy((const char*)data, 
                  (const char*)data + len, 
                  begin() + _readerIndex);
    }

    // get space start
    char *Buffer::begin() { return &_buffer[0]; }
    const char* Buffer::begin() const{  return &_buffer[0]; }
    
    void Buffer::ensureWritableBytes(size_t len){
        if (writeAbleBytes() >= len) {
            return;
        }else {
            makeSpace(len);
        }
    } //
    void Buffer::makeSpace(size_t len){
        if (writeAbleBytes() + prependAbleBytes() < len + kCheapPrepend) {
            _buffer.resize(_writerIndex + len);
        }else {
            int oldReadSize = readAbleBytes(); //  get data size
            std::copy(begin() + _readerIndex, 
                      begin() + _writerIndex, 
                      begin() + kCheapPrepend);
            _readerIndex = kCheapPrepend;
            _writerIndex = _readerIndex + oldReadSize;
        }
    }
    // 从文件中读取数据到缓冲区中：readv分块接收-一次性实现获取socket所有数据
    ssize_t Buffer::readFd(int fd){
        //ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
        size_t wableSize = writeAbleBytes();
        char temp[65536];
        struct iovec iov[2];
        iov[0].iov_base = beginWrite(); //缓冲区的可写空间起始地址
        iov[0].iov_len = wableSize; // 缓冲区的可写空间大小
        iov[1].iov_base = temp;
        iov[1].iov_len = 65536;
        // 1. 分块接收数据
        int count = wableSize >= 65536 ? 1 : 2;
        ssize_t ret = readv(fd, iov, count);
        if (ret > 0) {
            // 2. 判断实际读取的数据大小是否大于缓冲区的剩余可写空间大小
            if (ret <= wableSize) {
                //  小于等于：我们的缓冲区已经获取了所有的数据
                hasWriten(ret);
                return ret;
            }else {
                //  大于：有多余的数据放到temp中， 多余的数据大小就是ret - writeAbleBytes()
                // 3. 如果有多余的数据在临时空间中，则追加到缓冲区的末尾
                hasWriten(wableSize);
                append(temp, ret - wableSize);
            }
        }
        return ret;
    } //  get data from file 
    std::optional<std::string> Buffer::getline(bool hasCRLF) {
        // 1. 从缓冲区的可读数据中查找\n字符
        char *pos = (char*)std::memchr(peek(), '\n', readAbleBytes());
        // 2. 如果没找到，则直接返回 nullopt
        if (pos == nullptr) {
            return std::nullopt;
        }
        // 3. 找到了，则从起始位置peek位置，获取数据直到\n位置
        size_t size = pos - peek() + 1;
        if (hasCRLF == true) {
            return retriveAsString(size);
        }
        std::string retval = retriveAsString(size);
        if (retval.back() == '\n') retval.pop_back();
        if (retval.back() == '\r') retval.pop_back();
        return retval;
    }
}