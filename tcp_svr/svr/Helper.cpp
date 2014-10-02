#include "Helper.h"
#include <stdlib.h>
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

SvrConfig g_svrConfig;

void InitConfig(const char *pszConfigFileName)
{
    LoadConf(pszConfigFileName);

    char *pszTemp;

    pszTemp = GetConf("iMainCheckInterval");
    if (pszTemp && atoi(pszTemp))
        g_svrConfig.iMainCheckInterval = atoi(pszTemp);

    pszTemp = GetConf("iMainClientTimeOut");
    if (pszTemp && atoi(pszTemp))
        g_svrConfig.iMainClientTimeOut = atoi(pszTemp);

    pszTemp = GetConf("iMainLogFileCount");
    if (pszTemp && atoi(pszTemp))
        g_svrConfig.iMainLogFileCount = atoi(pszTemp);

    pszTemp = GetConf("iMainLogFileSize");
    if (pszTemp && atoi(pszTemp))
        g_svrConfig.iMainLogFileSize = atoi(pszTemp);

    pszTemp = GetConf("iMainLogLevel");
    if (pszTemp && atoi(pszTemp))
        g_svrConfig.iMainLogLevel = atoi(pszTemp);

    pszTemp = GetConf("iMainMaxClient");
    if (pszTemp && atoi(pszTemp))
        g_svrConfig.iMainMaxClient = atoi(pszTemp);

    pszTemp = GetConf("iMainMessageQueueSize");
    if (pszTemp && atoi(pszTemp))
        g_svrConfig.iMainMessageQueueSize = atoi(pszTemp);

    pszTemp = GetConf("iMainServerPort");
    if (pszTemp && atoi(pszTemp))
        g_svrConfig.iMainServerPort = atoi(pszTemp);

    pszTemp = GetConf("strMainLogFileName");
    if (pszTemp)
        g_svrConfig.strMainLogFileName = pszTemp;

    pszTemp = GetConf("strMainServerIP");
    if (pszTemp)
        g_svrConfig.strMainServerIP = pszTemp;
}

//后台执行
void InitDaemon()
{
    rlimit rlim, rlim_new;
    if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
    {
        rlim_new.rlim_cur = rlim_new.rlim_max = 100000;
        if (setrlimit(RLIMIT_NOFILE, &rlim_new) != 0)
        {
            Warn("%s", "[Main]:Setrlimit file Fail, use old rlimit!\n");
            rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
            setrlimit(RLIMIT_NOFILE, &rlim_new);
        }
    }

    if (getrlimit(RLIMIT_CORE, &rlim) == 0)
    {
        rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_CORE, &rlim_new) != 0)
        {
            Warn("%s", "[Main]:Setrlimit core Fail, use old rlimit!\n");
            rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
            setrlimit(RLIMIT_CORE, &rlim_new);
        }
    }

    signal(SIGPIPE, SIG_IGN);
    pid_t pid;

    if ((pid = fork()) != 0)
    {
        exit(0);
    }

    setsid();
    signal( SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal( SIGTERM, SIG_IGN);

    struct sigaction sig;
    sig.sa_handler = SIG_IGN;
    sig.sa_flags = 0;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGHUP, &sig, NULL);
    if ((pid = fork()) != 0)
    {
        exit(0);
    }
    umask(0);
    setpgrp();
}
