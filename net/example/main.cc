#include "../eventloop.h"
#include "../timer.h"
#include <iostream>
#include <unistd.h>

int main()
{
    net::Buffer buffer;
    int32_t dataLen = 0;
    // 1. 向缓冲区中添加1000条数据：HelloWorld+%d\n, 并统计数据总长度
    for (int i = 0; i < 1000; ++i) {
        std::string str = "HelloWorld+" + std::to_string(i) + '\n';
        dataLen += str.size();
        buffer.append(str);
    }
    // 2. 向缓冲区前置添加数据总长度字段
    buffer.prepend(&dataLen, sizeof(dataLen));
    // 3. 断言缓冲区中的数据总长度，应该等于 内容数据总长度 + 4
    assert(buffer.readAbleBytes() == dataLen + sizeof(dataLen));
    // 4. 从缓冲区中peek32获取数字， 与总长度进行比较，这两个数据必须一致
    int32_t tmpSize = buffer.peekInt32();
    assert(tmpSize == dataLen);
    buffer.retrive(sizeof(dataLen));
    // 5. 使用getline逐行获取数据，与原来的数据进行对比
    for(int i = 0; i < 1000; i++) {
        std::optional<std::string> tmp = buffer.getline();
        assert(tmp);
        std::string str = "HelloWorld+" + std::to_string(i);
        assert(*tmp == str);
    }
    assert(buffer.readAbleBytes() == 0);
    std::cout << "测试通过!!\n";

   // net::EventLoopThread loopthread;
    // auto baseloop = loopthread.startLoop();
    // net::Timestamp now = net::Timestamp::now();

    // auto id = baseloop->runAt(net::addTime(now, 3), [](){
    //     std::cout << "这是个3s后的延迟任务\n";
    // });

    // baseloop->runAfter(5, [](){
    //     std::cout << "这是个5s后的延迟任务\n";
    // });

    // baseloop->runEvery(1, [](){
    //     std::cout << "这是个1s的循环任务\n";
    // });

    // baseloop->cancel(id);

    // sleep(10);
    return 0;
}