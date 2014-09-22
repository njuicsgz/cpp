#include "SyncData.h"

SyncData::SyncData(const unsigned uIntervalTime, map<SIPAttrShort,
        set<unsigned> > &mapAttr2IP, map<unsigned, SIPAttrShort> &mapIP2Attr,
        CMutex &mutex) :
    m_uIntervalTime(uIntervalTime), m_uLastTimeStamp(0), m_mapAttr2IP(
            mapAttr2IP), m_mapIP2Attr(mapIP2Attr), m_mutex(mutex)
{
}

SyncData::~SyncData()
{
}

void SyncData::SyncIPAttr()
{
    vector<SIPAttr> vecMod;
    vector<unsigned> vecDel;
    unsigned uTSNew;

    CIPDCApi api(g_svrConfig.sIPDCSvrIP.c_str(), g_svrConfig.uIPDCSvrPort);
    if (api.GetIPAttrByTS(vecMod, vecDel, uTSNew, this->m_uLastTimeStamp) != 0)
    {
        Err("GetMonConf error, last_timestamp[%u]", m_uLastTimeStamp);
        return;
    }

    ml lock(m_mutex);

    SIPAttrShort is;
    for(size_t i = 0; i < vecMod.size(); i++)
    {
        SIPAttr &ia = vecMod[i];

        is.uCountry = ia.uCountry;
        is.uIsp = ia.uIsp;
        is.uProvince = ia.uProvince;

        this->m_mapAttr2IP[is].insert(ia.uOptFlag);
        this->m_mapIP2Attr[ia.uOptFlag] = is;
    }

    map<SIPAttrShort, set<unsigned> >::iterator it;
    for(size_t j = 0; j < vecDel.size(); j++)
    {
        for(it = m_mapAttr2IP.begin(); it != m_mapAttr2IP.end(); it++)
        {
            (it->second).erase(vecDel[j]);
        }

        this->m_mapIP2Attr.erase(vecDel[j]);
    }

    Info("sync end, old_ts[%u], new_ts[%u], mod_size[%u], del_size[%u], map_size_now[%u], ip_total[%u]",
            m_uLastTimeStamp, uTSNew, vecMod.size(), vecDel.size(),
            m_mapAttr2IP.size(), m_mapIP2Attr.size());

    m_uLastTimeStamp = uTSNew;
}

void SyncData::Run()
{
    for(;;)
    {
        SyncIPAttr();

        sleep(m_uIntervalTime);
    }
}

