#ifndef _CMD_PROCESS_H_
#define _CMD_PROCESS_H_

#include "message_queue.h"
#include "thread_runner.h"
#include "DBProcessor.h"
#include "Helper.h"
#include <map>
#include <vector>
#include <list>
#include <set>
#include <time.h>
#include <algorithm>

using namespace std;
class Commpack;
class CCmdProcess: public CRunner
{
public:

    void SendToClient(unsigned int flow, Commpack *pPack);
    void ProcessCmd(unsigned int flow, Commpack * pPack);
    void OnCmdCome(unsigned int flow, char *pData, int nLen);
    CCmdProcess(MQ& req, MQ& rsp, map<string, SIPAttr> &mapIPAttr,
            pthread_rwlock_t &lock);
    virtual ~CCmdProcess();
    virtual void Run();

public:
    void OnTest(unsigned int flow, Commpack *pPack);
    void OnLogin(unsigned int flow, Commpack *pPack);
    void OnHeartbeat(unsigned int flow, Commpack *pPack);
    void OnGetAllClient(unsigned int flow, Commpack *pPack);
    void OnTask1(unsigned int flow, Commpack *pPack);

    void GetIPByTS(unsigned int flow, Commpack *pPack);
    void OnReptTask(unsigned int flow, Commpack *pPack);

protected:
    static unsigned GetTaskID()
    {
        static unsigned tid = 0;

        unsigned utime = time(0);
        return ++tid > utime ? tid : utime;
    }

protected:
    MQ& m_ReqQ;
    MQ& m_RspQ;

    CDBProcessor *m_pDBProcessor;

    map<string, SIPAttr> &m_mapIPAttr;
    pthread_rwlock_t &m_lock;
};

#endif 
