#ifndef __LEVELLOG_H_
#define __LEVELLOG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>
#include <pthread.h>

    /* 定义日志级别 */
#define ERROR_LOG 0
#define WARN_LOG  1
#define INFO_LOG  2
#define DEBUG_LOG 3
#define LARGERNUM_LOG 4

    /* 默认log的最大日志文件为1M */
#define MIN_LOGFILE_SIZE 1024*1024

    /* 文件名的最大长度为512 */
#define PATH_LEN      512

    /**
     * 向全局G_pLog中写日志
     * @param fmt 日志的格式
     * @param ... 可变参数
     */
#define Err(fmt, ...)\
{    \
    LogRecord(G_pLog, __FILE__, __LINE__, ERROR_LOG, fmt, ##__VA_ARGS__);\
}

#define Warn(fmt, ...) \
{       \
    LogRecord(G_pLog, __FILE__, __LINE__, WARN_LOG, fmt, ##__VA_ARGS__); \
}

#define Info(fmt, ...) \
{       \
    LogRecord(G_pLog, __FILE__, __LINE__, INFO_LOG, fmt, ##__VA_ARGS__); \
}

#define Debug(fmt, ...) \
{       \
    LogRecord(G_pLog, __FILE__, __LINE__, DEBUG_LOG, fmt, ##__VA_ARGS__); \
}

#define LargeLog(fmt, ...) \
{       \
    LogRecord(G_pLog, __FILE__, __LINE__, LARGERNUM_LOG, fmt, ##__VA_ARGS__); \
}

    /*结构LOG 封装对日志的操作*/
    typedef struct LOG
    {
        int log_fd; /*日志文件描述符*/
        int log_size; /*日志文件大小*/
        int log_level; /*日志文件大小*/
        int log_filecount; /*日志文件循环个数*/
        int log_std; /*是否为标准输出*/
        char log_name[PATH_LEN]; /*日志文件名*/
        time_t log_date; /*日志时间*/
        pthread_mutex_t log_mutex;
    }LOG;

    extern struct LOG *G_pLog;

    /**
     * 初始化
     * @param fileName  日志文件名
     * @param logLevel  日志级别
     * @param maxSpace  日志文件大小
     * @param rollCount 日志文件循环的个数
     * @return   ptr    日志指针
     */
    struct LOG *CreateLog(const char *fileName, int logLevel, int maxSpace,
            int rollCount);

    void LogRecord(struct LOG *pLog, const char *file, int line, int log_level,
            const char *fmt, ...);

    /**
     * 销毁指针
     */
    void DestroyLog();

#ifdef __cplusplus
}
#endif

#endif

