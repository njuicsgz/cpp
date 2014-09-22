#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/file.h>
#include "Log.h"

/*һ����Ϣ����󳤶�*/
#define ERROR_MAXMSGLEN 8192

/*��־�ṹָ���ȫ�ֱ���*/
struct LOG* G_pLog = NULL;
char curfile[PATH_LEN];

char szLogOut[][8] = {"Error", "Warn-", "Info-", "Debug"};

/**
 * RollFile         ѭ����־�ļ�, pLog->log_name��Զ�ǵ�ǰ��־�ļ�,
 * pLog->log_name.1, pLog->log_name.1, ...pLog->log_name.pLog->log_filecount���ΰ��¾ɵ�����
 * @param pLog       ��־�ṹ��ָ��
 */
static void RollFile(struct LOG* pLog)
{
    close(pLog->log_fd);
    pLog->log_fd = -1;

    char fileNew[PATH_LEN];
    char fileOld[PATH_LEN];
    struct stat statfile;

    snprintf(fileOld, sizeof(fileOld) - 1, "%s.%d", pLog->log_name,
            pLog->log_filecount);
    if (stat(fileOld, &statfile) == 0)
    {
        remove(fileOld);
    }

    /* ѭ���Ľ�pLog->log_name.pLog->log_filecount - 1  -> pLog->log_name.pLog->log_filecount
     ...
     pLog->log_name.2 -> pLog->log_name.3
     pLog->log_name.1 -> pLog->log_name.2*/
    int i;
    for(i = pLog->log_filecount - 1; i > 0; i--)
    {
        snprintf(fileOld, sizeof(fileOld) - 1, "%s.%d", pLog->log_name, i);
        snprintf(fileNew, sizeof(fileNew) - 1, "%s.%d", pLog->log_name, i + 1);

        if (stat(fileOld, &statfile) == 0)
        {
            rename(fileOld, fileNew);
        }
    }

    /* ��pLog->log_name ->��pLog->log_name.1 */
    snprintf(fileNew, sizeof(fileNew) - 1, "%s.1", pLog->log_name);
    if (stat(pLog->log_name, &statfile) == 0)
    {
        rename(pLog->log_name, fileNew);
    }

    pLog->log_fd = open(pLog->log_name, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (pLog->log_fd == -1)
    {
        fprintf(stderr, "Open log file %s fail: %s\n", pLog->log_name,
                strerror(errno));
        free(pLog);
        pLog = NULL;
    }
}

/**
 * Is_File_OverSize  ��־�ļ��Ƿ񳬹�������޶�
 * @param pLog       ��־�ṹ��ָ��
 */
static int Is_File_OverSize(struct LOG *pLog)
{
    struct stat statfile;
    if (stat(pLog->log_name, &statfile) == -1)
    {
        return 0;
    }
    else
    {
        return (statfile.st_size >= pLog->log_size);
    }
}

/**
 * _Error_Out
 * @param pLog       ��־�ṹ��ָ��
 * @param loglevel  ��־����
 * @param msg       ��־��Ϣ
 */
static void _Error_Out(struct LOG *pLog, int loglevel, const char *msg)
{
    //get time now
    time_t now = time(NULL);
    struct tm ptm;
    static char szBuf[64] = {0};

    localtime_r(&now, &ptm);
    strftime(szBuf, sizeof(szBuf) - 1, "%Y-%m-%d %H:%M:%S", &ptm);

    static char out[ERROR_MAXMSGLEN + 2];
    snprintf(out, sizeof(out) - 2, "%s-%s:%s\n", szLogOut[loglevel], szBuf, msg);
    out[sizeof(out) - 1] = 0;

    if (pLog->log_fd > 0 && !pLog->log_std)
    {
        pthread_mutex_lock(&pLog->log_mutex);

        if (Is_File_OverSize(pLog))
        {
            RollFile(pLog);
        }

        write(pLog->log_fd, out, strlen(out));

        pthread_mutex_unlock(&pLog->log_mutex);

    }
    else if (pLog->log_std)
    {
        printf("%s\n", out);
    }
}

/*
 CreatLog : ��ʼ����־�ṹָ��
 * @param fileName  ��־�ļ���
 * @param maxSpace  ��־�ļ���С
 * @param logLevel  ��־����
 * @param rollCount ��־�ļ�ѭ���ĸ���
 * @return ptr
 */
struct LOG * CreateLog(const char *fileName, int logLevel, int maxSpace,
        int rollCount)
{
    G_pLog = (struct LOG*) malloc(sizeof(struct LOG));
    if (G_pLog == NULL)
        return NULL;

    memset(G_pLog, 0, sizeof(struct LOG));

    strncpy(G_pLog->log_name, fileName, PATH_LEN - 1);

    G_pLog->log_date = time(NULL);
    G_pLog->log_level = logLevel < 0 ? 0 : logLevel;
    G_pLog->log_size = maxSpace < MIN_LOGFILE_SIZE ? MIN_LOGFILE_SIZE : maxSpace;
    G_pLog->log_filecount = rollCount < 3 ? 3 : rollCount;
    G_pLog->log_std = 0;

    if (0 != pthread_mutex_init(&G_pLog->log_mutex, NULL))
        return NULL;

    if (strcmp(G_pLog->log_name, "/dev/null") == 0)
        G_pLog->log_fd = -1;
    else if (strcmp(G_pLog->log_name, "stdout") == 0)
        G_pLog->log_std = 1;
    else
    {
        snprintf(curfile, PATH_LEN - 1, "%s", G_pLog->log_name);

#ifdef WIN32
        if (access(curfile, 00 | 06) == 0)
#else
            if (access(curfile, F_OK | R_OK | W_OK) == 0)
#endif
            G_pLog->log_fd = open(curfile, O_RDWR | O_APPEND, 0644);
        else
            G_pLog->log_fd = open(curfile, O_CREAT | O_RDWR | O_APPEND, 0644);

        if (G_pLog->log_fd == -1)
        {
            fprintf(stderr, "Open log file %s fail: %s\n", G_pLog->log_name,
                    strerror(errno));
            free(G_pLog);
            G_pLog = NULL;
            return NULL;
        }
    }
    return G_pLog;
}

/**
 * DestroyLog ������־�ṹָ��
 * @param pLog ��־�ṹ��ָ��
 */
void DestroyLog(struct LOG *pLog)
{
    if (pLog)
    {
        pthread_mutex_destroy(&pLog->log_mutex);
        close(pLog->log_fd);
        free(pLog);
        pLog = NULL;
    }
}

/**
 * LogRecord д������־
 * @param pLog ��־�ṹ��ָ��
 * @param fmt  ��־�ĸ�ʽ
 * @param ...  �ɱ����
 */
void LogRecord(struct LOG *pLog, const char *file, int line, int log_level,
        const char *fmt, ...)
{
    if (pLog == NULL || pLog->log_level < log_level)
        return;

    char msg[BUFSIZ];
    char out[BUFSIZ];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    snprintf(out, BUFSIZ - 1, "<%s:%d>:%s", file, line, msg);
    _Error_Out(pLog, log_level, out);
}
