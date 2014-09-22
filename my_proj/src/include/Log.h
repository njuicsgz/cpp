#ifndef __LEVELLOG_H_
#define __LEVELLOG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>
#include <pthread.h>

    /* ������־���� */
#define ERROR_LOG 0
#define WARN_LOG  1
#define INFO_LOG  2
#define DEBUG_LOG 3
#define LARGERNUM_LOG 4

    /* Ĭ��log�������־�ļ�Ϊ1M */
#define MIN_LOGFILE_SIZE 1024*1024

    /* �ļ�������󳤶�Ϊ512 */
#define PATH_LEN      512

    /**
     * ��ȫ��G_pLog��д��־
     * @param fmt ��־�ĸ�ʽ
     * @param ... �ɱ����
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

    /*�ṹLOG ��װ����־�Ĳ���*/
    typedef struct LOG
    {
        int log_fd; /*��־�ļ�������*/
        int log_size; /*��־�ļ���С*/
        int log_level; /*��־�ļ���С*/
        int log_filecount; /*��־�ļ�ѭ������*/
        int log_std; /*�Ƿ�Ϊ��׼���*/
        char log_name[PATH_LEN]; /*��־�ļ���*/
        time_t log_date; /*��־ʱ��*/
        pthread_mutex_t log_mutex;
    }LOG;

    extern struct LOG *G_pLog;

    /**
     * ��ʼ��
     * @param fileName  ��־�ļ���
     * @param logLevel  ��־����
     * @param maxSpace  ��־�ļ���С
     * @param rollCount ��־�ļ�ѭ���ĸ���
     * @return   ptr    ��־ָ��
     */
    struct LOG *CreateLog(const char *fileName, int logLevel, int maxSpace,
            int rollCount);

    void LogRecord(struct LOG *pLog, const char *file, int line, int log_level,
            const char *fmt, ...);

    /**
     * ����ָ��
     */
    void DestroyLog();

#ifdef __cplusplus
}
#endif

#endif

