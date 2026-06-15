#include"timestamp.h"
namespace net{
    const static int64_t kMicroSecondsPerSecond=1000000;
    // 1.获取当前系统的时间戳（微秒级别 gettimeofday()）
            Timestamp Timestamp::now(){
                struct timeval tv;
                gettimeofday(&tv,NULL);
                return Timestamp(tv);
            }
            // 2.将时间戳转换为格式化字符串（主要用于日志输出）
            std::string Timestamp::toFormattedString(){
                time_t t=secondsSinceEpoch();
                struct tm lt;
                localtime_r(&t,&lt);
                char reval[32];
                snprintf(reval,31,"%4d-%2d-%2d %02d:%02d:%02d",
                lt.tm_year,lt.tm_mon,lt.tm_mday,
                lt.tm_hour,lt.tm_min,lt.tm_sec);
                return reval;
            }
            // 3.判断当前时间戳是否有效（0无效，大于0有效）
            bool valid();
            // 4.时间戳交换接口
            void swap(Timestamp&other);
            // 5.获取时间戳接口（单位：微秒/秒）
            time_t Timestamp::microSecondsSinceEpoch(){
                return _microSecondsSinceEpoch/kMicroSecondsPerSecond;
            }
            int64_t Timestamp::secondsSinceEpoch(){
                return secondsSinceEpoch/kMicroSecondsPerSecond;
            }
            // 6.设置时间戳接口（单位：微秒/秒）
            void Timestamp::fromUnixTime(time_t second){
                fromUnixTime(second,0);
            }
            void Timestamp::fromUnixTime(time_t second,int64_t usec){
                _microSecondsSinceEpoch=second*kMicroSecondsPerSecond+usec;
            }
            // 7.静态：创建一个无效的时间戳对象
            Timestamp Timestamp::invalid(){
                return Timestamp();
            }
}

