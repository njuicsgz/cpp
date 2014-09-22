/*
 * DBProcessor.h
 *
 *  Created on: 2012-10-30
 *      Author: allengao
 */

#ifndef DBPROCESSOR_H_
#define DBPROCESSOR_H_

#include "cmysql.h"
#include "Log.h"
#include "data_struct.h"

class CDBProcessor
{
public:
    CDBProcessor(const char* szHost, const char* szUser, const char* szPass)
    {
        if (0 != m_hMysql.Connect(szHost, szUser, szPass))
        {
            Err("mysql connect error, host[%s], usr[%s], pass[%s]",
                    szHost, szUser, szPass);
        }
    }
    ;
    ~CDBProcessor(void)
    {
    }
    ;

public:

    bool GetIPAttrByTS(vector<SIPAttr> &vecMod, vector<string> &vecDel,
            unsigned &uTSNew, const unsigned uTSOld);
    bool GetIPAttrByTS(vector<SIPAttr> &vecIA, unsigned &uTSNew,
            const unsigned uTSOld);

    bool IsExistReptTask(bool &bExist, const SReptTask &rt);
    bool InsReptTask(const SReptTask &rt);
    bool UpdReptTask(const SReptTask &rt);

    bool InsUpdReptTask(const SReptTask &rt);

    string GetErrMsg()
    {
        return m_sErrMsg;
    }

private:
    CDBProcessor(const CDBProcessor &);
    CDBProcessor & operator =(const CDBProcessor &);

private:
    string m_sErrMsg;
    CMysql m_hMysql;
};

#endif /* DBPROCESSOR_H_ */
