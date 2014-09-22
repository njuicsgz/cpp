#include "runner.h"
#include "protocol_long.h"
#include "CmdProcess.h"
#include "Helper.h"
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <string>

using namespace std;

static const char SVRCONF_FILE[] = "../conf/svr.conf";

int main(int argc, char * argv[])
{
    //后台执行
    InitDaemon();

    //初始化配置文件
    InitConfig(SVRCONF_FILE);

    // 初始化日志选项
    G_pLog = CreateLog(g_svrConfig.strMainLogFileName.c_str(),
            g_svrConfig.iMainLogLevel, g_svrConfig.iMainLogFileSize,
            g_svrConfig.iMainLogFileCount);

    try
    {
        CEPoller* client_epoller = new CEPoller();
        client_epoller->create(g_svrConfig.iMainMaxClient);
        CThreadLauncher* laucher = new CThreadLauncher;

        //accept线程
        MQ* req_mq = GetMQ(g_svrConfig.iMainMessageQueueSize);
        MQ* rsp_mq = GetMQ(g_svrConfig.iMainMessageQueueSize * 2);

        CProtocolFactory* pf = new CLongFactory();
        CachedConn* client_connections = new CachedConn(*pf);
        CReqAcceptor* acp = new CReqAcceptor(*client_epoller,
                *client_connections, *rsp_mq);
        acp->Init(g_svrConfig.strMainServerIP,
                (unsigned short) g_svrConfig.iMainServerPort);
        laucher->LaunchThread(acp);

        //recv线程
        CReceiver* req_receiver = new CReceiver(*client_epoller,
                *client_connections, *req_mq);
        laucher->LaunchThread(req_receiver);

        //处理线程
        CCmdProcess *cmd_process = new CCmdProcess(*req_mq, *rsp_mq);
        laucher->LaunchThread(cmd_process);

        //send线程
        CRspSender* rsp_sender = new CRspSender(*client_connections, *rsp_mq,
                *client_epoller);
        laucher->LaunchThread(rsp_sender);

        Info("[Main] Start success");

        //检查超时连接
        for(;;)
        {
            deque<unsigned> timeout_flow;
            client_connections->CheckTimeout(time(0)
                    - g_svrConfig.iMainClientTimeOut, timeout_flow);
            for(deque<unsigned>::iterator it = timeout_flow.begin(); it
                    != timeout_flow.end(); it++)
            {
                client_connections->CloseFlow(*it);
            }
            sleep(g_svrConfig.iMainCheckInterval);
        }
    }

    catch (exception& ex)
    {
        Err("Main function excption: ,errInfo[%s]",ex.what());
        return -1;
    }
    return 0;
}

