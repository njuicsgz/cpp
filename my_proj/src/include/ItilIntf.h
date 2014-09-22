#ifndef ITILF_H_H
#define ITILF_H_H

#include <iostream>
#include <string>
#include "http.h"
#include "json_inc/json.h"

using namespace std;

class CItilIntf
{
public:
    CItilIntf();
    CItilIntf(const string &sSvrIP, const unsigned int uiSvrPort);
    ~CItilIntf()
    {
    }
private:
    CItilIntf(const CItilIntf &itil)
    {
    }

    CItilIntf& operator =(const CItilIntf &itil)
    {
        return *this;
    }

    void setJsonRoot(Json::Value& root, const Json::Value& query,
            const Json::Value& retValue);
public:
    void SetSchemeId(const string& sSchemeId);
    void SetSyetemId(const string& sSystemId);
    bool Request(const Json::Value& queryCond,
            const Json::Value& expReturn, Json::Value& arrayObj,
            string& sErrMsg);
private:
    string m_sSvrIP;
    unsigned int m_uiSvrPort;

    string m_sUrl;
    string m_sSchemeId;
    string m_sSystemId;
};
#endif

