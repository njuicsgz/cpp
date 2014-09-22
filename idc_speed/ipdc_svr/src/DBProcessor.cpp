/*
 * DBProcessor.cpp
 *
 *  Created on: 2012-10-30
 *      Author: allengao
 */
#include "comm_func.h"
#include "DBProcessor.h"

bool CDBProcessor::GetIPAttrByTS(vector<SIPAttr> &vecMod,
        vector<string> &vecDel, unsigned &uTSNew, const unsigned uTSOld)
{
    vecMod.clear();
    vecDel.clear();
    char szSql[256] = {0};
    snprintf(szSql, sizeof(szSql) - 1,
            "select * from db_idc_speed_2.tbl_ip_attr where mod_ts >= %u ",
            uTSOld);
    try
    {
        m_hMysql.Query(szSql);
        m_hMysql.StoreResult();

        SIPAttr ia;
        char * pField = NULL;
        string sTmp("");
        uTSNew = uTSOld;
        while (NULL != m_hMysql.FetchRow())
        {
            pField = m_hMysql.GetField("ip");
            sTmp = safe_ptos(pField);
            snprintf(ia.szIP, sizeof(ia.szIP), "%s", sTmp.c_str());

            pField = m_hMysql.GetField("country");
            ia.uCountry = safe_atou(pField);

            pField = m_hMysql.GetField("isp");
            ia.uIsp = safe_atou(pField);

            pField = m_hMysql.GetField("prov");
            ia.uProvince = safe_atou(pField);

            pField = m_hMysql.GetField("mod_ts");
            ia.uModTS = safe_atou(pField);
            uTSNew = max(uTSNew, ia.uModTS);

            pField = m_hMysql.GetField("opt_flag");
            ia.uOptFlag = safe_atou(pField);

            if (ia.uOptFlag)
                vecDel.push_back(sTmp);
            else
                vecMod.push_back(ia);
        }
    } catch (CCommonException & ex)
    {
        this->m_sErrMsg = ex.GetErrMsg();
        return false;
    }
    return true;
}

bool CDBProcessor::GetIPAttrByTS(vector<SIPAttr> &vecIA, unsigned &uTSNew,
        const unsigned uTSOld)
{
    vecIA.clear();

    char szSql[256] = {0};
    snprintf(szSql, sizeof(szSql) - 1,
            "select * from db_idc_speed_2.tbl_ip_attr where mod_ts >= %u ",
            uTSOld);
    try
    {
        m_hMysql.Query(szSql);
        m_hMysql.StoreResult();

        SIPAttr ia;
        char * pField = NULL;
        string sTmp("");
        uTSNew = uTSOld;
        while (NULL != m_hMysql.FetchRow())
        {
            pField = m_hMysql.GetField("ip");
            sTmp = safe_ptos(pField);
            snprintf(ia.szIP, sizeof(ia.szIP), "%s", sTmp.c_str());

            pField = m_hMysql.GetField("country");
            ia.uCountry = safe_atou(pField);

            pField = m_hMysql.GetField("isp");
            ia.uIsp = safe_atou(pField);

            pField = m_hMysql.GetField("prov");
            ia.uProvince = safe_atou(pField);

            pField = m_hMysql.GetField("mod_ts");
            ia.uModTS = safe_atou(pField);
            uTSNew = max(uTSNew, ia.uModTS);

            pField = m_hMysql.GetField("opt_flag");
            ia.uOptFlag = safe_atou(pField);

            vecIA.push_back(ia);
        }
    } catch (CCommonException & ex)
    {
        this->m_sErrMsg = ex.GetErrMsg();
        return false;
    }
    return true;
}

bool CDBProcessor::IsExistReptTask(bool &bExist, const SReptTask &rt)
{
    char szBuf[1024] = {0};
    snprintf(
            szBuf,
            sizeof(szBuf) - 1,
            "select count(1) as cnt "
                "from db_idc_speed_2.tbl_delay_5min "
                "where day = '%s' and ip='%s' and country=%u and isp=%u and prov=%u;",
            rt.sDay.c_str(), rt.sIP.c_str(), rt.uCountry, rt.uISP, rt.uProv);
    try
    {
        m_hMysql.Query(szBuf);
        m_hMysql.StoreResult();
        if (NULL != m_hMysql.FetchRow())
        {
            char *pField = m_hMysql.GetField("cnt");
            unsigned cnt = safe_atou(pField);

            bExist = cnt > 0 ? true : false;
        }

    } catch (CCommonException & ex)
    {
        this->m_sErrMsg = ex.GetErrMsg();
        return false;
    }

    return true;
}

bool CDBProcessor::InsReptTask(const SReptTask &rt)
{
    char szBuf[1024] = {0};
    snprintf(
            szBuf,
            sizeof(szBuf) - 1,
            "insert into db_idc_speed_2.tbl_delay_5min(day, ip, country, isp, prov, test_ip_num, d%u) "
                "values('%s', '%s', %u, %u, %u, %u, %u);", rt.uDayPoint,
            rt.sDay.c_str(), rt.sIP.c_str(), rt.uCountry, rt.uISP, rt.uProv,
            rt.uIPNum, rt.uDelay);
    try
    {
        m_hMysql.Query(szBuf);
    } catch (CCommonException & ex)
    {
        this->m_sErrMsg = ex.GetErrMsg();
        return false;
    }

    return true;
}

bool CDBProcessor::UpdReptTask(const SReptTask &rt)
{
    char szBuf[1024] = {0};
    snprintf(
            szBuf,
            sizeof(szBuf) - 1,
            "update db_idc_speed_2.tbl_delay_5min "
                "set d%u=%u "
                "where day = '%s' and ip='%s' and country=%u and isp=%u and prov=%u;",
            rt.uDayPoint, rt.uDelay, rt.sDay.c_str(), rt.sIP.c_str(),
            rt.uCountry, rt.uISP, rt.uProv);
    try
    {
        m_hMysql.Query(szBuf);

    } catch (CCommonException & ex)
    {
        this->m_sErrMsg = ex.GetErrMsg();
        return false;
    }

    return true;
}

bool CDBProcessor::InsUpdReptTask(const SReptTask &rt)
{
    char szBuf[4096] = {0};
    snprintf(
            szBuf,
            sizeof(szBuf) - 1,
            "insert into db_idc_speed_2.tbl_delay_5min(day, ip, country, isp, prov, test_ip_num, d%u) "
                "values('%s', '%s', %u, %u, %u, %u, %u) on duplicate key update d%u=%u;", rt.uDayPoint,
            rt.sDay.c_str(), rt.sIP.c_str(), rt.uCountry, rt.uISP, rt.uProv,
            rt.uIPNum, rt.uDelay, rt.uDayPoint, rt.uDelay);
    try
    {
        m_hMysql.Query(szBuf);
    } catch (CCommonException & ex)
    {
        this->m_sErrMsg = ex.GetErrMsg();
        return false;
    }

    return true;
}
