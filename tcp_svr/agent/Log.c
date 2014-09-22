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

/*一条信息的最大长度*/
#define        ERROR_MAXMSGLEN    1024

/*日志结构指针的全局变量*/
struct LOG* G_pLog=NULL;
char curfile[PATH_LEN];
/**
* RollFile         循环日志文件,pLog->log_name永远是当前日志文件,
                   pLog->log_name.1,pLog->log_name.1,...pLog->log_name.pLog->log_filecount依次按新旧的排列
* @param pLog       日志结构的指针
*/
static void RollFile(struct LOG* pLog)
{
    close(pLog->log_fd);
    pLog->log_fd = -1;
    
    char fileNew[PATH_LEN];
    char fileOld[PATH_LEN];
    struct stat statfile;

    /* 先删除pLog->log_name.pLog->log_filecount文件 */
    if(pLog->log_filecount <= 0) /* 不滚动,加上时间*/
    {
        time_t now;
        struct tm *timenow;
        time(&now);
        timenow = localtime(&now);
        snprintf(curfile,sizeof(curfile) -1,"%s.%.4d%.2d%.2d%.2d%.2d%.2d",pLog->log_name,
                timenow->tm_year+1900,timenow->tm_mon+1,timenow->tm_mday,
                timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
        if(stat(curfile,&statfile) == 0)    
        {
            remove(curfile);
        }
        pLog->log_fd = open(curfile, O_CREAT | O_RDWR | O_APPEND, 0644 );
        if(pLog->log_fd==-1)
        {
            fprintf(stderr,"Open log file %s fail: %s\n",pLog->log_name,strerror(errno));
            free(pLog);
            pLog = NULL;
        }
        return;
    }

    snprintf(fileOld,sizeof(fileOld) -1,"%s.%d",pLog->log_name,pLog->log_filecount);
    if(stat(fileOld,&statfile) == 0)    
    {
            remove(fileOld);
    }

    /* 循环的将pLog->log_name.pLog->log_filecount - 1  -> pLog->log_name.pLog->log_filecount
               ...
           pLog->log_name.2 -> pLog->log_name.3
           pLog->log_name.1 -> pLog->log_name.2*/
    int i;
    for( i = pLog->log_filecount -1 ; i > 0 ; i--)
    {
        snprintf(fileOld,sizeof(fileOld) -1,"%s.%d",pLog->log_name,i);
        snprintf(fileNew,sizeof(fileNew) -1,"%s.%d",pLog->log_name,i+1);

        if(stat(fileOld,&statfile) == 0)    
        {
            rename(fileOld,fileNew);
        }
    }

    /* 将pLog->log_name ->　pLog->log_name.1 */
    snprintf(fileNew,sizeof(fileNew) -1,"%s.1",pLog->log_name);
    if(stat(pLog->log_name,&statfile) == 0)    
    {
            rename(pLog->log_name,fileNew);
    }

    pLog->log_fd = open(pLog->log_name, O_CREAT | O_RDWR | O_APPEND, 0644 );
    if(pLog->log_fd==-1)
    {
        fprintf(stderr,"Open log file %s fail: %s\n",pLog->log_name,strerror(errno));
        free(pLog);
        pLog = NULL;
    }
}

/**
* Is_File_OverSize 日志文件是否超过了最大限额 
* @param pLog       日志结构的指针
*/
static int Is_File_OverSize(struct LOG *pLog)
{
    struct stat statfile;
    if(stat(pLog->log_name,&statfile) == -1)
    {
        return 0;
    }
    else
    {
        return (statfile.st_size >= pLog->log_size);
    }
}

static int Is_File_OverSize2(int  log_size,const char *path)
{
    struct stat statfile;
    if(stat(path,&statfile) == -1)
    {
        return 0;
    }
    else
    {
        return (statfile.st_size >= log_size);
    }
}

/**
* _Error_Out 
* @param pLog       日志结构的指针
* @param loglevel  日志级别
* @param msg       日志消息
*/
static void _Error_Out(struct LOG *pLog , int loglevel , const char *msg)
{
    char szLogOut[][8] = {"Error","Warn","Info","Debug"};

    static char out[ERROR_MAXMSGLEN + 2];
    int len;

    time_t now=time(NULL);
    len = snprintf(out, sizeof(out) - 2, "%s - %.24s : %s\n",szLogOut[loglevel],ctime(&now), msg);
 
    if (pLog->log_fd > 0 && !pLog->log_std)
    {
        flock(pLog->log_fd,LOCK_EX); 
	    if(pLog->log_filecount <= 0) /* 非循环日志*/
		{
			if(Is_File_OverSize2(pLog->log_size,curfile))
				RollFile(pLog);
		}
        else if(Is_File_OverSize(pLog))
        {
            RollFile(pLog);
        }

        out[len] = '\0';
        write(pLog->log_fd,out,len);
        flock(pLog->log_fd,LOCK_UN);        

    }
    else if(pLog->log_std)
    {
        printf("%s\n",out);
    }
}

/* 
CreatLog : 初始化日志结构指针
* @param fileName  日志文件名
* @param maxSpace  日志文件大小 
* @param logLevel  日志级别 
* @param rollCount 日志文件循环的个数 
* @return ptr
*/
struct LOG * CreateLog(const char *fileName ,int logLevel, int maxSpace , int rollCount)
{
    struct LOG *pLog;
  
    pLog = (struct LOG*)malloc( sizeof ( struct LOG ) );
    if( pLog == NULL )
        return NULL;

    memset(pLog, 0, sizeof(struct LOG ) );

    strncpy(pLog->log_name,fileName,PATH_LEN);

    pLog->log_date  = time(NULL);
    pLog->log_level = logLevel < 0 ? 0 : logLevel;
    pLog->log_size  = maxSpace < MIN_LOGFILE_SIZE ? MIN_LOGFILE_SIZE : maxSpace;
    pLog->log_filecount = rollCount;
    pLog->log_std = 0 ;

    if(strcmp(pLog->log_name,"/dev/null") == 0)
        pLog->log_fd = -1;
    else if(strcmp(pLog->log_name,"stdout") == 0)
        pLog->log_std = 1;
    else
    {
        if(pLog->log_filecount <= 0)
        {
            time_t now;
            struct tm *timenow;
            time(&now);
            timenow = localtime(&now);
            snprintf(curfile,sizeof(curfile) -1,"%s.%.4d%.2d%.2d%.2d%.2d%.2d",pLog->log_name,
                    timenow->tm_year+1900,timenow->tm_mon+1,timenow->tm_mday,
                    timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
        }
        else
        {
            snprintf(curfile,PATH_LEN,"%s",pLog->log_name);
        }
#ifdef WIN32
        if(access(curfile,00 | 06) == 0 )
#else
        if(access(curfile, F_OK | R_OK | W_OK) == 0 )
#endif
            pLog->log_fd = open(curfile,O_RDWR | O_APPEND, 0644 );
        else
            pLog->log_fd = open(curfile, O_CREAT | O_RDWR | O_APPEND, 0644 );
        
        if(pLog->log_fd==-1)
        {
            fprintf(stderr,"Open log file %s fail: %s\n",pLog->log_name,strerror(errno));
            free(pLog);
            pLog = NULL;
            return NULL;
        }
    }
    return pLog;
}

/**
* DestroyLog 销毁日志结构指针
* @param pLog 日志结构的指针
*/
void DestroyLog(struct LOG *pLog)
{
    if(pLog)
    {
        close(pLog->log_fd);
        free (pLog);
        pLog = NULL;
    }    
}

/**
* ErrLog 写错误日志
* @param pLog 日志结构的指针
* @param fmt  日志的格式
* @param ...  可变参数
*/
void ErrLog(struct LOG *pLog , const char *fmt, ...)
{
    if(pLog == NULL || pLog->log_level < ERROR_LOG)
        return;

    char msg[BUFSIZ];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    _Error_Out(pLog, ERROR_LOG,msg);
}

/**
* WarnLog 写警告日志
* @param pLog 日志结构的指针
* @param fmt  日志的格式
* @param ...  可变参数
*/
void WarnLog(struct LOG *pLog , const char *fmt, ...)
{
    if(pLog == NULL || pLog->log_level < WARN_LOG)
        return;

    char msg[BUFSIZ];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    _Error_Out(pLog, WARN_LOG,msg);
}

/**
* InfoLog 写Info日志
* @param pLog 日志结构的指针
* @param fmt  日志的格式
* @param ...  可变参数
*/
void InfoLog(struct LOG *pLog , const char *fmt, ...)
{
    if(pLog == NULL || pLog->log_level < INFO_LOG)
        return;

    char msg[BUFSIZ];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    _Error_Out(pLog, INFO_LOG,msg);
}

/**
* DebugLogX 写Debug日志,加入了Debug的行号
* @param pLog 日志结构的指针
* @param fmt  日志的格式
* @param ...  可变参数
*/
void DebugLogX(struct LOG *pLog ,const char *file ,int line , const char *fmt,...)
{
    if(pLog == NULL || pLog->log_level < DEBUG_LOG)
        return;

    char msg[BUFSIZ];
    char out[BUFSIZ];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    snprintf(out,BUFSIZ-1,"%s_%d:%s",file,line,msg);
    _Error_Out(pLog, DEBUG_LOG,out);
}
