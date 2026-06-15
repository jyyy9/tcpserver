#pragma once//防止头文件重复包含
/*
该文件实现时间戳管理类：
    1.获取当前系统的时间戳（微秒级别 gettimeofday()）
    2.将时间戳转换为格式化字符串（主要用于日志输出）
    3.判断当前时间戳是否有效（0无效，大于0有效）
    4.时间戳交换接口
    5.获取时间戳接口（单位：微秒/秒）
    6.设置时间戳接口（单位：微秒/秒）
    7.静态：创建一个无效的时间戳对象
    成员：
        微秒与秒的转换单位成员
        时间戳成员（初始化0）

针对时间戳对象，重载<,==比较运算符
针对时间戳对象设计一个addTime(Timestamp timestamp, double seconds);接口，获取一个偏移的时间戳对象
*/
#include<time.h>
#include<cstring>
#include<iostream>
namespace net{
    class Timestamp{
        public:
            Timestamp();//构造无效的时间戳对象
            Timestamp(int64_t usec);//
            const static int64_t kMicroSecondsPerSecond=1000000;
            // 1.获取当前系统的时间戳（微秒级别 gettimeofday()）
            Timestamp now();
            // 2.将时间戳转换为格式化字符串（主要用于日志输出）
            std::string toFormattedString();
            // 3.判断当前时间戳是否有效（0无效，大于0有效）
            bool valid();
            // 4.时间戳交换接口
            void swap(Timestamp&other);
            // 5.获取时间戳接口（单位：微秒/秒）
            time_t microSecondsSinceEpoch();
            int64_t secondsSinceEpoch();
            // 6.设置时间戳接口（单位：微秒/秒）
            void fromUnixTime(time_t second);
            void fromUnixTime(time_t second,int64_t usec);
            // 7.静态：创建一个无效的时间戳对象
            Timestamp invalid();
        private:
            int64_t _microSecondsSinceEpoch;
    };
}