#pragma once 
/*
    该文件中实现时间戳管理类：
        1. 静态：获取当前系统时间的时间戳(微秒级别  gettimeofday())
        2. 将时间戳转换为格式化字符串（主要用于日志输出）
        3. 判断当前时间戳对象是否有效（0-无效； 大于0表示有效）
        4. 时间戳交换接口
        5. 获取时间戳接口（微秒/秒）
        6. 设置时间戳接口（微秒/秒）
        7. 静态： 创建一个无效的时间戳对象
        成员：
            微秒与秒的转换单位成员
            时间戳成员 （初始化为0）

    针对时间戳对象，重载<， ==比较运算符
    针对时间戳对象实现一个addTime(timestamp, int second)接口,获取一个偏移的时间戳对象
*/
#include <stdint.h>
#include <time.h>
#include <string>

namespace net {
    class Timestamp {
        public:
            //秒与微秒的转换单位
            const static int64_t kMicroSecondPerSecond = 1000000;
            Timestamp();
            Timestamp(int64_t usec);
            // 3. 判断当前时间戳对象是否有效（0-无效； 大于0表示有效）
            bool valid();
            // 4. 时间戳交换接口
            void swap(Timestamp &other);
            // 5. 获取时间戳接口（微秒/秒）
            int64_t microSecondsSinceEpoch();
            time_t secondsSinceEpoch();
            // 6. 设置时间戳接口（微秒/秒）
            void fromUnixTime(time_t second);
            void fromUnixTime(time_t second, int64_t usec);
            // 2. 将时间戳转换为格式化字符串（主要用于日志输出）
            std::string toFormatString();
            // 1. 静态：获取当前系统时间的时间戳(微秒级别  gettimeofday())
            static Timestamp now();
            // 7. 静态： 创建一个无效的时间戳对象
            static Timestamp invalid();
        private:
            int64_t _microSecondSinceEpoch; //1900-1-1 0:0:0
    };

    bool operator< (Timestamp lhs, Timestamp rhs);
    bool operator== (Timestamp lhs, Timestamp rhs);
    Timestamp addTime(Timestamp lhs, double second);
}