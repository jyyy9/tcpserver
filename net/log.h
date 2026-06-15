#include <cstdio>
#include <stdarg.h>

#define LOG_DEBUG(fmt, ...) { \
    printf("[DEBUG][%s:%d]" fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
}
#define LOG_ERROR(fmt, ...) { \
    printf("[ERROR][%s:%d]" fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
}
#define LOG_FATAL(fmt, ...) { \
    printf("[FATAL][%s:%d]" fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    abort();\
}