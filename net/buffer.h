#pragma once

/*
    实现一个用户态的缓冲区类
*/
#include <vector>
#include <string>
#include <optional>

namespace net {
    class Buffer {
            const static int kInitialSize = 1024; //
            const static int kCheapPrepend = 8; //
        public:
            Buffer();
            Buffer(int initSize);
            Buffer(const Buffer& other);
            Buffer(Buffer&& other);
            Buffer& operator=(const Buffer& other);
            Buffer& operator=(Buffer&& other);

            void swap(Buffer& other);
            size_t readAbleBytes();// read data size
            size_t writeAbleBytes(); // write idle space size
            size_t prependAbleBytes(); // prepend idle space size

            const char* peek() const;
            char* peek(); //  get read position
            void retrive(size_t len); // move _readerIndex
            std::string retriveAllAsString();
            std::string retriveAsString(size_t len);
            int32_t peekInt32(); // peek get data as int32

            char *beginWrite(); // get write position
            void hasWriten(size_t len); // move _writerIndex
            void append(const void *data, size_t len);
            void append(const std::string &data);
            void prepend(const void *data, size_t len);
            ssize_t readFd(int fd); //  get data from file 
            std::optional<std::string> getline(bool hasCRLF = false); //CRLF ： \r\n
        private:
            char *begin() ;// get space start
            const char* begin() const;

            void ensureWritableBytes(size_t len); //
            void makeSpace(size_t len);

        private: 
            std::vector<char> _buffer;
            int _readerIndex; //
            int _writerIndex; //
    };
}