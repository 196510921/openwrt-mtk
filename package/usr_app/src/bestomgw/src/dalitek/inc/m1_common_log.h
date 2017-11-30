#ifndef M1_COMMON_LOG_H
#define M1_COMMON_LOG_H

#include <stdio.h>
#include <stdlib.h>


typedef enum M1_LOG_LEVEL
{
    M1_LOG_LEVEL_DEBUG = 0,
    M1_LOG_LEVEL_INFO,
    M1_LOG_LEVEL_WARN,
    M1_LOG_LEVEL_ERROR,
    M1_LOG_LEVEL_FATAL,
    M1_LOG_LEVEL_NONE,
}m1_log_level_t;

extern m1_log_level_t m1LogLevel;

void m1_common_log_set_level(m1_log_level_t m1LogLevel);


#define M1_LOG_DEBUG(format, ...) \
{\
    if(m1LogLevel <= M1_LOG_LEVEL_DEBUG)\
    {\
        printf("[debug] %s:%d %s()| "format"\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
        fflush(stdout);\
    }\
}

#define M1_LOG_INFO(format, ...) \
{\
    if(m1LogLevel <= M1_LOG_LEVEL_INFO)\
    {\
        printf("[info] %s:%d %s()| "format"\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
        fflush(stdout);\
    }\
}

#define M1_LOG_WARN(format, ...) \
{\
    if(m1LogLevel <= M1_LOG_LEVEL_WARN)\
    {\
        printf("[warn] %s:%d %s()| "format"\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
        fflush(stdout);\
    }\
}

#define M1_LOG_ERROR(format,...) \
{\
    if(m1LogLevel <= M1_LOG_LEVEL_ERROR)\
    {\
        printf("[error] %s:%d %s()| "format"\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
        fflush(stdout);\
    }\
}

#define M1_LOG_FATAL(format, ...) \
{\
    if(m1LogLevel <= M1_LOG_LEVEL_FATAL)\
    {\
        printf("[notice] %s:%d %s()| "format"\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
        fflush(stdout);\
    }\
}

#endif
