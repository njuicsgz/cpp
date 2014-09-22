#include "ConnectThread.h"
#include "commpack.h"
#include "helper.h"
#include "Api.h"
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

//错误命令字
enum
{
    ERROR_NO = 0,
    ERROR_NOUSER,
    ERROR_FD_DOWN,
    ERROR_FD_TIMEOUT,
    ERROR_SVRRETURN,
    ERROR_OPEN,
    ERROR_SEND,
    ERROR_RECV
};

enum cmd
{
    CMD_HEARTBEAT = 0x1000, CMD_TASK1 = 0x2000,
};

CConnectThread::CConnectThread()
{
}

CConnectThread::~CConnectThread()
{
}

int CConnectThread::SelectSingleRead(int fd, float timeout, fd_set* r_set)
{
    if (fd >= 1024)
        return -1;

    FD_SET(fd, r_set);

    struct timeval tv;
    tv.tv_sec = (int) timeout;
    tv.tv_usec = (int) ((timeout - (int) timeout) * 1000000);

    return select(fd + 1, r_set, NULL, NULL, &tv);
}

int CConnectThread::SelectSingleRead(int fd, float timeout)
{
    fd_set r_set;
    FD_ZERO(&r_set);
    return SelectSingleRead(fd, timeout, &r_set);
}

int CConnectThread::Recv(int iFD, Commpack *&pack, unsigned int ulTimeOut)
{
    char buffer[5];
    int iResult = 0;
    int nRet = SelectSingleRead(iFD, ulTimeOut);

    if (nRet == 0)
        return ERROR_FD_TIMEOUT;
    else if (nRet < 0)
        return ERROR_FD_DOWN;

    int nRecv = recv(iFD, buffer, sizeof(buffer), 0);
    if (nRecv <= 0)
        return ERROR_FD_DOWN;
    else if (nRecv < 5)
        return ERROR_SVRRETURN;

    unsigned int len = *((unsigned int*) (buffer + 1));
    len = ntohl(len);
    char * pBuffer = new char[len - 5];
    unsigned int nRemain = len - 5;
    unsigned int nRecved = 0;
    while (nRemain > 0)
    {
        nRet = SelectSingleRead(iFD, ulTimeOut);
        if (nRet == 0)
        {
            iResult = ERROR_FD_TIMEOUT;
            break;
        }
        else if (nRet < 0)
        {
            iResult = ERROR_FD_DOWN;
            break;
        }

        if (nRet > 0)
        {
            int nRecv = recv(iFD, pBuffer + nRecved, nRemain, 0);
            if (nRecv <= 0)
            {
                if (errno == EAGAIN)
                {
                    continue;
                }
                else
                {
                    iResult = ERROR_FD_DOWN;
                    break;
                    break;
                }
            }
            nRecved += nRecv;
            nRemain -= nRecv;
        }
        else
        {
            break;
        }
    }
    if (iResult != 0)
    {
        delete[] pBuffer;
        return iResult;
    }

    if (nRecved != len - 5)
    {
        delete[] pBuffer;
        return ERROR_SVRRETURN;
    }

    unsigned char* pInputBuffer = new unsigned char[len];
    memcpy(pInputBuffer, buffer, 5);
    memcpy(pInputBuffer + 5, pBuffer, len - 5);
    delete[] pBuffer;
    pack = new Commpack(len + 64);
    bool b = pack->Input((void *) pInputBuffer, len);
    delete[] pInputBuffer;

    return b ? ERROR_NO : ERROR_SVRRETURN;
}

void CConnectThread::Run()
{
    //循环执行，该线程不退出
    CApi stApi(g_svrConfig.strSvrIP.c_str(), g_svrConfig.iSvrPort);
    for(;;)
    {
        //登录服务器
        int iCount = 0;
        for(; iCount < 3; iCount++)
        {
            if (stApi.OnLogin("Version 1.0") == 0)
                break;

            Err("[CConnectThread::Run]:Login to %s:%d failed", g_svrConfig.strSvrIP.c_str(), g_svrConfig.iSvrPort );
            stApi.Close();
            sleep(10);
        }

        if (iCount == 3)
        {
            sleep(60);
            Err("[CConnectThread::Run]:Login to %s:%d failed 3 times", g_svrConfig.strSvrIP.c_str(), g_svrConfig.iSvrPort );
            continue;
        }

        Info("[CConnectThread::Run]:Login to %s:%d success", g_svrConfig.strSvrIP.c_str(), g_svrConfig.iSvrPort );

        //select在FD上等待数据
        while (true)
        {
            //发送心跳
            if (stApi.OnHeartBeat("Version 1.0") != 0)
            {
                Err("[CConnectThread::Run]:Heartbeat to %s:%d failed", g_svrConfig.strSvrIP.c_str(), g_svrConfig.iSvrPort );
                break;
            }

            Info("[CConnectThread::Run]:Heartbeat to %s:%d success", g_svrConfig.strSvrIP.c_str(), g_svrConfig.iSvrPort );

            //收包，等待10S收包，每次只收一个包
            Commpack * pLongPackage = NULL;
            int iRet = Recv(stApi.GetFD(), pLongPackage, 10);

            //FD出现异常，关闭FD，并重新连接
            if (iRet == ERROR_FD_DOWN)
            {
                if (pLongPackage != NULL)
                {
                    delete pLongPackage;
                }
                stApi.Close();

                Err("[CConnectThread::Run]:Recv from %s:%d failed", g_svrConfig.strSvrIP.c_str(), g_svrConfig.iSvrPort );
                break;
            }

            //超时
            else if (iRet == ERROR_FD_TIMEOUT)
            {
                if (pLongPackage != NULL)
                {
                    delete pLongPackage;
                }
                continue;
            }

            //分配空间，非法包等异常
            else if (iRet == ERROR_SVRRETURN)
            {
                if (pLongPackage != NULL)
                {
                    delete pLongPackage;
                }
                continue;
            }

            //正常包
            else if (iRet == ERROR_NO)
            {
                //任务下来了执行相应的任务
                OnCmdCome(pLongPackage);
                if (pLongPackage != NULL)
                {
                    delete pLongPackage;
                }
            }
        }
    }
}

void CConnectThread::OnCmdCome(Commpack *pLongPackage)
{
    //判断是哪种协议
    unsigned int ulCmd = pLongPackage->GetCmd();
    switch (ulCmd)
    {
        case CMD_TASK1:
            OnTask1(pLongPackage);
            break;
    }

    return;
}

/*************************************** 
 协议(CMD_TEST)

 Client->Server协议: ulTestData

 Server->Client协议: ulResult ulTestResult

 ****************************************/

void CConnectThread::OnTask1(Commpack *pPack)
{
    unsigned int nCmd = pPack->GetCmd();
    unsigned short nSeq = pPack->GetSeq();
    Commpack pack;
    pack.SetCmd(nCmd);
    pack.SetSeq(nSeq);

    unsigned int ulTestData;

    if (pPack->GetUInt(ulTestData))
    {
        printf("%u\n", ulTestData);
    }
    else
    {
        printf("GetData error\n");
    }
}
