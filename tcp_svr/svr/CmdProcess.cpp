#include "CmdProcess.h"
#include "limits.h"
#include "Helper.h"
#include "CSessionMgr.h"
#include "commpack.h"

enum cmd
{
    CMD_TEST = 0x0001,
    CMD_LOGIN = 0x1000,
    CMD_HEARTBEAT,
    CMD_GETALLCLIENT = 0x1100,
    CMD_TASK1 = 0x2000,
};

CCmdProcess::CCmdProcess(MQ& req, MQ& rsp) :
    m_ReqQ(req), m_RspQ(rsp)
{
}

CCmdProcess::~CCmdProcess()
{
}

void CCmdProcess::SendToClient(unsigned int flow, Commpack *pPack)
{
    void * pData;
    int nSize;
    pPack->Output(pData, nSize);

    Message msg(flow);
    msg.AttachMessage((char*) pData, nSize);
    if (!m_RspQ.Enqueue(msg))
    {
        Warn("enqueue failed flow=[%d]",flow);
    }
}

void CCmdProcess::Run()
{
    for(;;)
    {
        Message msg(ULONG_MAX);
        if (!m_ReqQ.Dequeue(msg))
        {
            continue;
        }
        unsigned flow = msg.Flow();
        int length = 0;
        char* sData = msg.GetMessage(length);
        if (sData == NULL)
        {
            continue;
        }
        OnCmdCome(flow, sData, length);
    }
}

void CCmdProcess::OnCmdCome(unsigned int flow, char *pData, int nLen)
{
    Commpack * pPack = new Commpack(nLen + 512);
    if (!pPack->Input((void *) pData, nLen))
    {
        delete pPack;
        return;
    }
    ProcessCmd(flow, pPack);
    delete pPack;
}

void CCmdProcess::ProcessCmd(unsigned int flow, Commpack *pPack)
{
    unsigned int ulCmd = pPack->GetCmd();
    switch (ulCmd)
    {
        case CMD_TEST:
            OnTest(flow, pPack);
            break;
        case CMD_LOGIN:
            OnLogin(flow, pPack);
            break;
        case CMD_HEARTBEAT:
            OnHeartbeat(flow, pPack);
            break;
        case CMD_GETALLCLIENT:
            OnGetAllClient(flow, pPack);
            break;
        case CMD_TASK1:
            OnTask1(flow, pPack);
            break;
        default:
            break;
    }
}

/*************************************** 
 测试协议(CMD_TEST)

 Client->Server协议: ulTestData

 Server->Client协议: ulResult ulTestResult

 ****************************************/

void CCmdProcess::OnTest(unsigned int flow, Commpack *pPack)
{
    unsigned int nCmd = pPack->GetCmd();
    unsigned short nSeq = pPack->GetSeq();
    Commpack pack;
    pack.SetCmd(nCmd);
    pack.SetSeq(nSeq);

    unsigned int ulTestData;

    if (pPack->GetUInt(ulTestData))
    {
        pack.AddUInt(0);
        pack.AddUInt(ulTestData * 2);
        SendToClient(flow, &pack);
    }
    else
    {
        pack.AddUInt(1000);
        SendToClient(flow, &pack);
    }
}

/*************************************** 
 登录协议(CMD_LOGIN)

 Client->Server协议: ulDataLen szData

 Server->Client协议: ulResult

 ****************************************/

void CCmdProcess::OnLogin(unsigned int flow, Commpack *pPack)
{
    unsigned int nCmd = pPack->GetCmd();
    unsigned short nSeq = pPack->GetSeq();
    Commpack pack;
    pack.SetCmd(nCmd);
    pack.SetSeq(nSeq);

    unsigned int ulLen;
    void * pData;
    if (!pPack->GetUInt(ulLen) || !pPack->GetData(pData, ulLen))
    {
        pack.AddUInt(1000);
        SendToClient(flow, &pack);
    }
    else
    {
        pack.AddUInt(0);
        SendToClient(flow, &pack);
    }

    //一旦用户登陆后，将用户登录IP关联到flow上
    MapIP2Flow(flow);
}

void CCmdProcess::OnHeartbeat(unsigned int flow, Commpack *pPack)
{
}

void CCmdProcess::OnGetAllClient(unsigned int flow, Commpack *pPack)
{
    unsigned int nCmd = pPack->GetCmd();
    unsigned short nSeq = pPack->GetSeq();
    Commpack pack;
    pack.SetCmd(nCmd);
    pack.SetSeq(nSeq);

    pack.AddUInt(0);

    unsigned int ulLen = 0;

    vector < string > vecSession;
    GetAllSession( vecSession);

    pack.AddUInt(vecSession.size());
    for(unsigned int i = 0; i < vecSession.size(); i++)
    {
        string strIP = vecSession[i];
        ulLen = strIP.length() + 1;
        pack.AddUInt(ulLen);
        pack.AddData((const void*) strIP.c_str(), ulLen);
    }

    SendToClient(flow, &pack);
}

void CCmdProcess::OnTask1(unsigned int flow, Commpack *pPack)
{
    unsigned int nCmd = pPack->GetCmd();
    unsigned short nSeq = pPack->GetSeq();
    Commpack pack;
    pack.SetCmd(nCmd);
    pack.SetSeq(nSeq);

    Commpack packForward;
    packForward.SetCmd(CMD_TASK1);
    packForward.SetSeq(nSeq);

    string strClientIP;
    unsigned int ulLen;
    void * pData;
    if (!pPack->GetUInt(ulLen) || !pPack->GetData(pData, ulLen))
    {
        pack.AddUInt(1000);
        SendToClient(flow, &pack);
        return;
    }
    strClientIP = (char *) pData;

    unsigned int ulTestData;
    if (!pPack->GetUInt(ulTestData))
    {
        pack.AddUInt(1000);
        SendToClient(flow, &pack);
        return;
    }

    packForward.AddUInt(ulTestData);

    unsigned int ulForwardFlow;
    if (!GetFlowByIP(strClientIP, ulForwardFlow))
    {
        pack.AddUInt(1000);
        SendToClient(flow, &pack);
        return;
    }

    SendToClient(ulForwardFlow, &packForward);

    pack.AddUInt(0);
    SendToClient(flow, &pack);
}

