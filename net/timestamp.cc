#include "timestamp.h"
#include <sys/time.h>

namespace net {
    Timestamp::Timestamp(): _microSecondSinceEpoch(0){}
    Timestamp::Timestamp(int64_t usec): _microSecondSinceEpoch(usec){}
    // 3. 判断当前时间戳对象是否有效（0-无效； 大于0表示有效）
    bool Timestamp::valid(){ return _microSecondSinceEpoch != 0; }
    // 4. 时间戳交换接口
    void Timestamp::swap(Timestamp &other){
        std::swap(_microSecondSinceEpoch, other._microSecondSinceEpoch);
    }
    // 5. 获取时间戳接口（微秒/秒）
    int64_t Timestamp::microSecondsSinceEpoch(){
        return _microSecondSinceEpoch;
    }
    time_t Timestamp::secondsSinceEpoch(){
        return _microSecondSinceEpoch / kMicroSecondPerSecond;
    }
    // 6. 设置时间戳接口（微秒/秒）
    void Timestamp::fromUnixTime(time_t second){
        fromUnixTime(second, 0);
    }
    void Timestamp::fromUnixTime(time_t second, int64_t usec){
        _microSecondSinceEpoch = second * kMicroSecondPerSecond + usec;
    }
    // 7. 静态： 创建一个无效的时间戳对象
    Timestamp Timestamp::invalid(){
        return Timestamp();
    }
    // 2. 将时间戳转换为格式化字符串（主要用于日志输出）
    std::string Timestamp::toFormatString(){
        // yyyy-mm-dd hh:mm:ss
        time_t t = secondsSinceEpoch();
        struct tm lt;
        localtime_r(&t, &lt); // 将时间戳转换为格式化时间结构
        char retval[32];
        snprintf(retval, 31, "%4d-%02d-%02d %02d:%02d:%02d",
            lt.tm_year, lt.tm_mon, lt.tm_mday,
            lt.tm_hour, lt.tm_min, lt.tm_sec);
        return retval;
    }
    // 1. 静态：获取当前系统时间的时间戳(微秒级别  gettimeofday())
    Timestamp Timestamp::now(){
        // 获取细粒度系统时间
        struct timeval tv;
        //int gettimeofday(struct timeval *tv, struct timezone *tz)
        gettimeofday(&tv, NULL);
        return Timestamp(tv.tv_sec * kMicroSecondPerSecond + tv.tv_usec);
    }

    bool operator< (Timestamp lhs, Timestamp rhs){
        return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }
    bool operator== (Timestamp lhs, Timestamp rhs){
        return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }
    Timestamp addTime(Timestamp lhs, double second){
        // 计算微秒差值
        int64_t detal = second * Timestamp::kMicroSecondPerSecond;
        return Timestamp(lhs.microSecondsSinceEpoch() + detal);
    }
}