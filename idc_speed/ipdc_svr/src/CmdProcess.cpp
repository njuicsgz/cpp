#include "CmdProcess.h"
#include "Helper.h"
#include "CSessionMgr.h"
#include "commpack.h"
#include "comm_func.h"

enum cmd
{
    CMD_TEST = 0x0001,
    CMD_LOGIN = 0x1000,
    CMD_HEARTBEAT,
    CMD_GETALLCLIENT = 0x1100,
    CMD_TASK1 = 0x2000,

    CMD_AGENT_GET_IP_BY_TS = 0x3301,
    CMD_AGENT_REPT_TASK = 0x3302,
};

CCmdProcess::CCmdProcess(MQ& req, MQ& rsp, map<string, SIPAttr> &mapIPAttr,
        pthread_rwlock_t &lock) :
    m_ReqQ(req), m_RspQ(rsp), m_mapIPAttr(mapIPAttr), m_lock(lock)
{
    m_pDBProcessor = new CDBProcessor(g_svrConfig.sDBHost.c_str(),
            g_svrConfig.sDBUser.c_str(), g_svrConfig.sDBPass.c_str());
}

CCmdProcess::~CCmdProcess()
{
    if (m_pDBProcessor)
    {
        delete m_pDBProcessor;
        m_pDBProcessor = NULL;
    }
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

    Debug("new_cmd: %x", ulCmd);

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
        case CMD_AGENT_GET_IP_BY_TS:
            GetIPByTS(flow, pPack);
            break;
        case CMD_AGENT_REPT_TASK:
            OnReptTask(flow, pPack);
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

void CCmdProcess::GetIPByTS(unsigned int flow, Commpack *pPack)
{
    unsigned int nCmd = pPack->GetCmd();
    unsigned short nSeq = pPack->GetSeq();
    Commpack pack;
    pack.SetCmd(nCmd);
    pack.SetSeq(nSeq);

    unsigned uTSOld;
    if (!pPack->GetUInt(uTSOld))
    {
        pack.AddUInt(1000);
        SendToClient(flow, &pack);
        Warn("[GetIPByTS] error protocal, return 1000.");
        return;
    }

    time_t t1 = time(0);

    //1. get incremental change from memory by timestamp
    vector<SIPAttr> vecMod;
    vector < string > vecDel;
    unsigned uTSNew = uTSOld;

    pthread_rwlock_rdlock(&this->m_lock);

    map<string, SIPAttr>::iterator it;
    for(it = this->m_mapIPAttr.begin(); it != this->m_mapIPAttr.end(); it++)
    {
        SIPAttr &ia = it->second;

        if (ia.uModTS >= uTSOld)
        {
            if (0 == ia.uOptFlag)
            {
                vecMod.push_back(ia);
            }
            else
            {
                vecDel.push_back(ia.szIP);
            }
        }

        uTSNew = max(uTSNew, ia.uModTS);
    }

    pthread_rwlock_unlock(&this->m_lock);

    time_t t2 = time(0);

    Debug("select:%lus", t2-t1);

    //2. new a package, for enought size. compress data.
    Commpack pack_new(vecMod.size() * 8 + vecDel.size() * 4 + 512);
    pack_new.SetCmd(nCmd);
    pack_new.SetSeq(nSeq);

    pack_new.AddUInt(0);
    pack_new.AddUInt(uTSNew);

    pack_new.AddUInt(vecMod.size());
    for(size_t i = 0; i < vecMod.size(); i++)
    {
        SIPAttr &ia = vecMod[i];
        pack_new.AddUInt(inet_addr(ia.szIP));
        pack_new.AddUShort(ia.uCountry);
        pack_new.AddByte(ia.uIsp);
        pack_new.AddByte(ia.uProvince);
    }

    pack_new.AddUInt(vecDel.size());
    for(size_t j = 0; j < vecDel.size(); j++)
    {
        pack_new.AddUInt(inet_addr(vecDel[j].c_str()));
    }

    time_t t3 = time(0);
    Debug("add :%lus", t3-t2);

    SendToClient(flow, &pack_new);

    time_t t4 = time(0);
    Debug("send:%lus", t4-t3);
}

void GetDayMin(string &sDay, unsigned &uDayPoint, const unsigned uTimestamp)
{
    struct tm now;
    time_t tt = uTimestamp;
    localtime_r(&tt, &now);

    char szBuf[128] = {0};
    strftime(szBuf, sizeof(szBuf) - 1, "%Y-%m-%d", &now);

    sDay = szBuf;
    uDayPoint = 1 + (now.tm_hour * 60 + now.tm_min) / 5;
}

void CCmdProcess::OnReptTask(unsigned int flow, Commpack *pPack)
{
    unsigned int nCmd = pPack->GetCmd();
    unsigned short nSeq = pPack->GetSeq();
    Commpack pack;
    pack.SetCmd(nCmd);
    pack.SetSeq(nSeq);

    //1. get data from client
    unsigned uClientTS = 0;
    unsigned uSize = 0;
    if (!pPack->GetUInt(uClientTS) || !pPack->GetUInt(uSize))
    {
        pack.AddUInt(1000);
        SendToClient(flow, &pack);
        return;
    }

    string sIP;
    if (!GetIPByFlow(sIP, flow))
    {
        pack.AddUInt(1000);
        SendToClient(flow, &pack);

        Warn("[CCmdProcess::OnReptTask] GetIPByFlow error, return 1000.");
        return;
    }

    string sDay("");
    unsigned uDayPoint = 0;
    GetDayMin(sDay, uDayPoint, uClientTS);

    vector<SReptTask> vecRT;
    for(size_t i = 0; i < uSize; i++)
    {
        SReptTask rt;
        if (!pPack->GetUInt(rt.uCountry) || !pPack->GetUInt(rt.uISP)
                || !pPack->GetUInt(rt.uProv) || !pPack->GetUInt(rt.uIPNum)
                || !pPack->GetUInt(rt.uDelay))
        {
            pack.AddUInt(1000);
            SendToClient(flow, &pack);
            return;
        }

        rt.sDay = sDay;
        rt.uDayPoint = uDayPoint;
        rt.sIP = sIP;

        vecRT.push_back(rt);
    }

    //2. set data to db
    for(size_t j = 0; j < vecRT.size(); j++)
    {
        if (!m_pDBProcessor->InsUpdReptTask(vecRT[j]))
        {
            Err("[CCmdProcess::OnReptTask] InsUpdReptTask error!"
                    " Err[%s]", m_pDBProcessor->GetErrMsg().c_str());
        }
    }

    pack.AddUInt(0);
    SendToClient(flow, &pack);

    Info("[CCmdProcess::OnReptTask] ip:[%s], day[%s], point[%u], size[%u]",
            sIP.c_str(), sDay.c_str(), uDayPoint, vecRT.size());
}
