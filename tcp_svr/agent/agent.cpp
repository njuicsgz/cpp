#include "helper.h"
#include "Log.h"
#include "thread_runner.h"
#include "ConnectThread.h"
#include <string>
#include <vector>
#include <map>

using namespace std;

const char CONFFILE[] = "../conf/agent.conf";

int main(int argc, char *argv[])
{
    InitDaemon();

    InitConfig(CONFFILE);

    G_pLog = CreateLog(g_svrConfig.strMainLogFileName.c_str(),
            g_svrConfig.iMainLogLevel, g_svrConfig.iMainLogFileSize,
            g_svrConfig.iMainLogFileCount);

    CConnectThread *pConnectThread = new CConnectThread;
    pConnectThread->Run();

    CThreadLauncher* laucher = new CThreadLauncher;
    laucher->LaunchThread(pConnectThread);

    for (;;)
    {
        sleep(30);
    }

    return 0;
}

