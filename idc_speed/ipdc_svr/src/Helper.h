#ifndef _HELPER_H_
#define _HELPER_H_

#include <stdio.h>
#include <string>
#include "Conf.h"
#include "Log.h"

using namespace std;

typedef struct tagSvrConfig
{
//服务器的主配置
	int iMainServerPort;
	string strMainServerIP;
	int iMainClientTimeOut;
	int iMainMaxClient;
	int iMainMessageQueueSize;
	int iMainCheckInterval;

	int iMainLogLevel;
	int iMainLogFileSize;
	int iMainLogFileCount;
	string strMainLogFileName;

	int iProcessNum;

	int iSyncIPItvlSec;

	string sDBHost;
	string sDBUser;
	string sDBPass;

	tagSvrConfig()
	{
		iMainServerPort = 8082;
		strMainServerIP = "172.16.80.20";
		iMainClientTimeOut = 10;
		iMainMaxClient = 20000;
		iMainMessageQueueSize = 50000;
		iMainCheckInterval = 5;
		iMainLogLevel = 0;
		iMainLogFileSize = 102400;
		iMainLogFileCount = 3;
		strMainLogFileName = "./logs/svr.log";

		iProcessNum = 1;
		iSyncIPItvlSec = 60;
	}
}SvrConfig;

extern SvrConfig g_svrConfig;

void InitConfig(const char *pszConfigFileName);
void InitDaemon() ;

#endif

