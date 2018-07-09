#ifndef M1_COMMON_LOG_H
#define M1_COMMON_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#define DEBUG_LOG_OUTPUT_TO_FD     0
#define DEBUF_FILE_NANE_OUTPUT     0

// 功能代码
#define FM_ALL_NORMAL    0x00000   // 重新设置属性到缺省设置
#define FM_BOLD          0x10000   // 粗体 
#define FM_HALF_BRIGHT   0x20000   // 设置一半亮度 
#define FM_UNDERLINE     0x40000
#define FM_FLASH         0x80000   // 闪烁
#define FM_RES_WHITE     0x100000  // 反白显示
#define FM_IN_VISIBLE    0x200000  // 不可见

// 前景色
#define FR_NORMAL  0x00 
#define FR_BLACK   0x01  
#define FR_RED     0x02
#define FR_GREEN   0x03
#define FR_YELLOW  0x04
#define FR_BLUE    0x05
#define FR_MAGENTA 0x06   // Magenta 紫红色
#define FR_CYAN    0x07   // 青蓝色
#define FR_WHITE   0x08  
#define FR_SET_UNDERLINE 0x09   // 默认前景色加下划线
#define FR_RMV_UNDERLINE 0x0A

// 背景色
#define BK_NORMAL  0x0000
#define BK_BLACK   0x0100
#define BK_RED     0x0200
#define BK_GREEN   0x0300
#define BK_YELLOW  0x0400
#define BK_BLUE    0x0500
#define BK_MAGENTA 0x0600  // Magenta 紫红色
#define BK_CYAN    0x0700  // 青蓝色
#define BK_WHITE   0x0800 
#define BK_DEFAULT 0x0900  // 默认背景色

void fmt_print(char* str,...);
void fm_print(char* str, unsigned int ulFormat);

#define FMT_ALL_NORMAL    "0"  // 重新设置属性到缺省设置
#define FMT_BOLD          "1"  // 粗体 
#define FMT_HALF_BRIGHT   "2"  // 设置一半亮度 
#define FMT_UNDERLINE     "4"
#define FMT_FLASH         "5"  // 闪烁
#define FMT_RES_WHITE     "7"  // 反白显示
#define FMT_IN_VISIBLE    "8"  // 不可见

// 前景色
#define FR_COLOR_BLACK   "30"  
#define FR_COLOR_RED     "31"
#define FR_COLOR_GREEN   "32"
#define FR_COLOR_YELLOW  "33"
#define FR_COLOR_BLUE    "34"
#define FR_COLOR_MAGENTA "35"   // Magenta 紫红色
#define FR_COLOR_CYAN    "36"   // 青蓝色
#define FR_COLOR_WHITE   "37"  
#define FR_COLOR_SET_UNDERLINE "38"   // 默认前景色加下划线
#define FR_COLOR_RMV_UNDERLINE "39"

// 背景色
#define BK_COLOR_BLACK   "40"
#define BK_COLOR_RED     "41"
#define BK_COLOR_GREEN   "42"
#define BK_COLOR_YELLOW  "43"
#define BK_COLOR_BLUE    "44"
#define BK_COLOR_MAGENTA "45"  // Magenta 紫红色
#define BK_COLOR_CYAN    "46"   // 青蓝色
#define BK_COLOR_WHITE   "47" 
#define BK_COLOR_DEFAULT "49"   // 默认背景色

#define FM_PRINT_BNG     "\033["
#define FM_PRINT_MID     "m"
#define FM_PRINT_END     "\033[0m"

extern char m1LogBuf[];

/*
|---------------|---------------|---------------|---------------|
     前景色           背景色        特殊样式        光标控制
|---------------|---------------|---------------|---------------|

*/
typedef enum
{  
    E_BK_NORMAL = 0,  
    E_BK_BLACK,    // 1
    E_BK_RED,      //2// 2 
    E_BK_GREEN,    // 3
    E_BK_YELLOW,   // 4
    E_BK_BLUE,     // 5
    E_BK_MAGENTA,  // 6 
    E_BK_CYAN,     // 7 
    E_BK_WHITE,    // 8
    E_BK_DEFAULT,  // 9

}eBackColor;

typedef enum
{
    EFMT_ALL_NORMAL,   // 0  重新设置属性到缺省设置
    EFMT_BOLD,         // 1  粗体 
    EFMT_HALF_BRIGHT,  // 2  设置一半亮度 
    EFMT_UNDERLINE,    // 4
    EFMT_FLASH,        // 5  闪烁
    EFMT_RES_WHITE,    // 7  反白显示
    EFMT_IN_VISIBLE,   // 8  不可见
    
}eEffect;

void show_info(char *str);
void show_item_warn(char *str);
void show_error(char *str);

char *get_cur_time(void);

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

void debug_switch(char* input_info);

void m1_common_log_set_level(m1_log_level_t m1LogLevel);

#if DEBUF_FILE_NANE_OUTPUT

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
            //printf("[info] %s:%d %s()| "format"\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
            printf("[info] %s %s:%d %s()| "format"\n", get_cur_time(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
            fflush(stdout);\
        }\
    }

    #define M1_LOG_WARN(format, ...) \
    {\
        if(m1LogLevel <= M1_LOG_LEVEL_WARN)\
        {\
            printf("[warn] %s %s:%d %s()| "format"\n", get_cur_time(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
            fflush(stdout);\
        }\
    }

    #define M1_LOG_ERROR(format,...) \
    {\
        if(m1LogLevel <= M1_LOG_LEVEL_ERROR)\
        {\
            printf("[error] %s %s:%d %s()| "format"\n", get_cur_time(),__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
            fflush(stderr);\
        }\
    }

    #define M1_LOG_FATAL(format, ...) \
    {\
        if(m1LogLevel <= M1_LOG_LEVEL_FATAL)\
        {\
            printf("[notice] %s:%d %s()| "format"\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
            fflush(stderr);\
        }\
    }
#else
     #define M1_LOG_DEBUG(format, ...) \
    {\
        if(m1LogLevel <= M1_LOG_LEVEL_DEBUG)\
        {\
            printf("[debug]:%d %s()| "format"\n", __LINE__, __FUNCTION__, ##__VA_ARGS__);\
            bzero(m1LogBuf,512);\
            snprintf(m1LogBuf,512,"%s %d %s()| "format"\n", get_cur_time(),__LINE__, __FUNCTION__,##__VA_ARGS__);\
            WriteLog("/mnt/syslog.log",m1LogBuf);\
            fflush(stdout);\
        }\
    }

    #define M1_LOG_INFO(format, ...) \
    {\
        if(m1LogLevel <= M1_LOG_LEVEL_INFO)\
        {\
            show_info("[info]:");\
            printf(":%s %d %s()| "format"\n", get_cur_time(),__LINE__, __FUNCTION__, ##__VA_ARGS__);\
            bzero(m1LogBuf,512);\
            snprintf(m1LogBuf,512,"%s %d %s()| "format"\n", get_cur_time(),__LINE__, __FUNCTION__,##__VA_ARGS__);\
            WriteLog("/mnt/syslog.log",m1LogBuf);\
            fflush(stdout);\
        }\
    }

    #define M1_LOG_WARN(format, ...) \
    {\
        if(m1LogLevel <= M1_LOG_LEVEL_WARN)\
        {\
            show_item_warn("[warn]:");\
            printf("%s %d %s()| "format"\n", get_cur_time(),__LINE__, __FUNCTION__, ##__VA_ARGS__);\
            bzero(m1LogBuf,512);\
            snprintf(m1LogBuf,512,"%s %d %s()| "format"\n", get_cur_time(),__LINE__, __FUNCTION__,##__VA_ARGS__);\
            WriteLog("/mnt/syslog.log",m1LogBuf);\
            fflush(stdout);\
        }\
    }

    #define M1_LOG_ERROR(format,...) \
    {\
        if(m1LogLevel <= M1_LOG_LEVEL_ERROR)\
        {\
            show_error("[error]:");\
            printf("%s %d %s()| "format"\n", get_cur_time(),__LINE__, __FUNCTION__, ##__VA_ARGS__);\
            bzero(m1LogBuf,512);\
            snprintf(m1LogBuf,512,"%s %d %s()| "format"\n", get_cur_time(),__LINE__, __FUNCTION__,##__VA_ARGS__);\
            WriteLog("/mnt/syslog.log",m1LogBuf);\
            fflush(stderr);\
        }\
    }

    #define M1_LOG_FATAL(format, ...) \
    {\
        if(m1LogLevel <= M1_LOG_LEVEL_FATAL)\
        {\
            printf("[notice]:%d %s()| "format"\n", __LINE__, __FUNCTION__, ##__VA_ARGS__);\
            fflush(stderr);\
        }\
    }

#endif  //DEBUF_FILE_NANE_OUTPUT


#endif  //M1_COMMON_LOG_H
