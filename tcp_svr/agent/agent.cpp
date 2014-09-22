#include "helper.h"
#include "Log.h"
#include "thread_runner.h"
#include "ConnectThread.h"
#include <string>
#include <vector>
#include <map>

using namespace std;

const char CONFFILE[] = "../conf/agent.conf";

int main(int argc , char *argv[])
{
	//后台执行
	InitDaemon();

	//初始化配置文件
	InitConfig( CONFFILE ) ;

	//初始化日志
	G_pLog = CreateLog (g_svrConfig.strMainLogFileName.c_str(), g_svrConfig.iMainLogLevel, g_svrConfig.iMainLogFileSize, g_svrConfig.iMainLogFileCount);

	//创建心跳及接受任务线程
	CConnectThread *pConnectThread = new CConnectThread;
	pConnectThread->Run();

	CThreadLauncher* laucher = new CThreadLauncher;
	laucher->LaunchThread( pConnectThread );

	//主线程不退出
	for(;;)
	{
		sleep(30);
	}

	return 0;
}



