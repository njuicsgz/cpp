#ifndef _HELPER_H_
#define _HELPER_H_

#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Conf.h"
#include "Log.h"

using namespace std;

typedef struct tagSvrConfig
{
    //服务器IP
    string strSvrIP;

    //服务器端口
    int iSvrPort;

    //日志配置
    string strMainLogFileName;
    int iMainLogLevel;
    int iMainLogFileSize;
    int iMainLogFileCount;

    tagSvrConfig()
    {
        strSvrIP = "1.1.1.1";
        iSvrPort = 8082;

        strMainLogFileName = "../log/agent.log";
        iMainLogLevel = 5;
        iMainLogFileSize = 1024 * 1024 * 10;
        iMainLogFileCount = 3;
    }
    ;

} SvrConfig;

extern SvrConfig g_svrConfig;

void InitConfig(const char *pszConfigFileName);

void InitDaemon();

#endif

