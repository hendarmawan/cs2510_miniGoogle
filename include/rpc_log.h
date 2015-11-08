#ifndef __RPC_LOG__
#define __RPC_LOG__

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string>

static std::string get_time_str() {
    time_t raw_time;
    time(&raw_time);

    struct tm local_time;
    localtime_r(&raw_time, &local_time);

    char time_str[128] = { 0 };
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &local_time);
    return time_str;
}

#define RPC_DEBUG(format, args...) \
    fprintf(stderr, "DEBUG %s %s:%d %s() " format"\n", \
            get_time_str().c_str(), __FILE__, __LINE__, __func__, ##args)

#define RPC_INFO(format, args...) \
    fprintf(stderr, "INFO %s %s:%d %s() " format"\n", \
            get_time_str().c_str(), __FILE__, __LINE__, __func__, ##args)

#define RPC_WARNING(format, args...) \
    fprintf(stderr, "WARNING %s %s:%d %s() " format"\n", \
            get_time_str().c_str(), __FILE__, __LINE__, __func__, ##args)

#define RPC_TERMINAL(format, args...) \
    fprintf(stderr, format, ##args)

#endif
