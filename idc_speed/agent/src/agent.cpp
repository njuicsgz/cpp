#include "helper.h"
#include "Log.h"
#include "thread_runner.h"
#include "ConnectThread.h"
#include "guardian.h"
#include "SyncData.h"
#include "detect_rept.h"
#include <string>
#include <vector>
#include <map>

using namespace std;

const char CONFFILE[] = "../conf/idcspeed_agt.conf";

int main(int argc, char *argv[])
{
    //后台执行
    InitDaemon();

    //初始化配置文件
    InitConfig(CONFFILE);

    //初始化日志
    G_pLog = CreateLog(g_svrConfig.strMainLogFileName.c_str(),
            g_svrConfig.iMainLogLevel, g_svrConfig.iMainLogFileSize,
            g_svrConfig.iMainLogFileCount);

    CThreadLauncher* laucher = new CThreadLauncher;

    //创建心跳及接受任务线程
    CConnectThread *pConnectThread = new CConnectThread;
    laucher->LaunchThread(pConnectThread);

    //监控配置同步线程
    CMutex mutex;
    map<SIPAttrShort, set<unsigned> > mapAttr2IP;
    map<unsigned, SIPAttrShort> mapIP2Attr;
    SyncData *sync = new SyncData(g_svrConfig.uIPAttrSyncTimeItvlSec,
            mapAttr2IP, mapIP2Attr, mutex);
    laucher->LaunchThread(sync);

    //批量探测并上报ReptSvr线程
    CDetectRept *dr = new CDetectRept(g_svrConfig.uDetectItvlSec,
            g_svrConfig.uMaxIPNumPerAttr, mapAttr2IP, mapIP2Attr, mutex);
    laucher->LaunchThread(dr);

    //主线程不退出
    for(;;)
    {
        sleep(30);
    }

    return 0;
}

