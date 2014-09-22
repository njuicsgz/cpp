#include "sync_ip_mem.h"

CSyncIP::CSyncIP(const unsigned uIntervalTime, map<string, SIPAttr> &mapIPAttr,
        pthread_rwlock_t &lock) :
    m_uIntervalTime(uIntervalTime), m_uLastTimeStamp(0),
            m_mapIPAttr(mapIPAttr), m_lock(lock)
{
}

CSyncIP::~CSyncIP()
{
}

void CSyncIP::SyncIPAttr()
{
    //1. get ip attribute incrementally from DB
    CDBProcessor hDBProcessor(g_svrConfig.sDBHost.c_str(),
            g_svrConfig.sDBUser.c_str(), g_svrConfig.sDBPass.c_str());

    vector<SIPAttr> vecIA;

    unsigned uTSNew;
    if (!hDBProcessor.GetIPAttrByTS(vecIA, uTSNew, m_uLastTimeStamp))
    {
        Warn("[CSyncIP::SyncIPAttr] GetIPAttrByTS from db error.");
        return;
    }

    //2. update to memory
    pthread_rwlock_wrlock(&this->m_lock);

    for(size_t i = 0; i < vecIA.size(); i++)
    {
        SIPAttr &ia = vecIA[i];
        this->m_mapIPAttr[ia.szIP] = ia;
    }

    pthread_rwlock_unlock(&this->m_lock);

    Info("[CSyncIP::SyncIPAttr]old_ts[%u], new_ts[%u], get_size[%u], size_now[%u]",
            m_uLastTimeStamp, uTSNew, vecIA.size(),
            m_mapIPAttr.size());

    m_uLastTimeStamp = uTSNew;
}

void CSyncIP::Run()
{
    for(;;)
    {
        sleep(m_uIntervalTime);

        SyncIPAttr();
    }
}

