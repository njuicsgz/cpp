#include "helper.h"
#include <stdlib.h>

//程序配置
SvrConfig g_svrConfig;

inline void GetValueInt(int &iValue ,const char *pszName)
{
	char *pszTemp = GetConf(pszName);
	if( pszTemp != NULL)
		iValue = atoi(pszTemp);
}

inline void GetValueString(string &strValue,const char *pszName)
{
	char *pszTemp = GetConf(pszName);
	if(pszTemp != NULL)
		strValue = pszTemp;
}

void InitConfig(const char *pszConfigFileName )
{
	LoadConf(pszConfigFileName);

	GetValueString(g_svrConfig.strSvrIP,"SvrIP");
	GetValueInt(g_svrConfig.iSvrPort,"SvrPort");

	GetValueString(g_svrConfig.strMainLogFileName,"MainLogFileName");
	GetValueInt(g_svrConfig.iMainLogLevel,"MainLogLevel");
	GetValueInt(g_svrConfig.iMainLogFileSize,"MainLogFileSize");
	GetValueInt(g_svrConfig.iMainLogFileCount,"MainLogFileCount");
}

//后台执行
void InitDaemon() 
{
	rlimit rlim,rlim_new;
	if (getrlimit(RLIMIT_NOFILE, &rlim)==0)
	{
		rlim_new.rlim_cur = rlim_new.rlim_max = 100000;
		if (setrlimit(RLIMIT_NOFILE, &rlim_new)!=0)
		{
			Warn("%s","[Main]:Setrlimit file Fail, use old rlimit!\n");
			rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
			setrlimit(RLIMIT_NOFILE, &rlim_new);
		}
	}
	
	if (getrlimit(RLIMIT_CORE, &rlim)==0)
	{
		rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_CORE, &rlim_new)!=0)
		{
			Warn("%s","[Main]:Setrlimit core Fail, use old rlimit!\n");
			rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
			setrlimit(RLIMIT_CORE, &rlim_new);
		}
	}

	signal(SIGPIPE, SIG_IGN);
	pid_t pid;

	if ((pid = fork() ) != 0 )
	{
		exit( 0);
	}

	setsid();
	signal( SIGINT,  SIG_IGN);
	signal( SIGHUP,  SIG_IGN);
	signal( SIGQUIT, SIG_IGN);
	signal( SIGPIPE, SIG_IGN);
	signal( SIGTTOU, SIG_IGN);
	signal( SIGTTIN, SIG_IGN);
	signal( SIGCHLD, SIG_IGN);
	signal( SIGTERM, SIG_IGN);

	struct sigaction sig;
	sig.sa_handler = SIG_IGN;
	sig.sa_flags = 0;
	sigemptyset( &sig.sa_mask);
	sigaction( SIGHUP,&sig,NULL);
	if ((pid = fork() ) != 0 )
	{
		exit(0);
	}
	umask(0);
	setpgrp();
}

