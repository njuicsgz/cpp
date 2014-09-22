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

struct SvrConfig
{
    string sIPDCSvrIP;
    unsigned uIPDCSvrPort;

    unsigned uIPAttrSyncTimeItvlSec;
    unsigned uDetectItvlSec;

    unsigned uMaxIPNumPerAttr;

    //日志配置
    string strMainLogFileName;
    int iMainLogLevel;
    int iMainLogFileSize;
    int iMainLogFileCount;

    unsigned uMaxPingFileRemainDay;

    SvrConfig()
    {
        strMainLogFileName = "../log/agent.log";
        iMainLogLevel = 5;
        iMainLogFileSize = 1024 * 1024 * 10;
        iMainLogFileCount = 3;

        uDetectItvlSec = 300;
        uIPAttrSyncTimeItvlSec = 300;
        uMaxIPNumPerAttr = 500;
    }
    ;
};

extern SvrConfig g_svrConfig;

void InitConfig(const char *pszConfigFileName);

void InitDaemon();

#endif

