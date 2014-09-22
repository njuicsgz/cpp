#include "ItilIntf.h"

bool ParseJson(const string& sValue, Json::Value& arrayObj, string& sErrMsg)
{
    Json::Reader reader;
    Json::Value value;

    try
    {
        if (!reader.parse(sValue, value))
        {
            sErrMsg = reader.getFormatedErrorMessages();
            return false;
        }
    } catch (const std::exception &e)
    {
        sErrMsg = "Unexpected exception caugth:";
        sErrMsg += e.what();
        return false;
    }

    int returnCode = value["dataSet"]["header"]["returnCode"].asUInt();
    int errorCode = value["dataSet"]["header"]["errorCode"].asUInt();

    if (returnCode != 0 || errorCode != 0)
    {
        sErrMsg = value["dataSet"]["header"]["errorInfo"].asString();
        return false;
    }

    arrayObj = value["dataSet"]["data"];
    return true;
}

void CItilIntf::setJsonRoot(Json::Value& root, const Json::Value& query,
        const Json::Value& retValue)
{
    Json::Value sub_root0, sub_root1, sub_root2, sub_root4;

    // 初始参数1
    sub_root1["schemeId"] = this->m_sSchemeId;

    sub_root1["type"] = "Json";
    sub_root1["version"] = "1.0";
    sub_root1["dataFormat"] = "dict";

    //初始参数2,系统分配
    sub_root2["systemId"] = this->m_sSystemId;
    sub_root2["sceneId"] = "1";
    sub_root2["requestModule"] = "";
    sub_root2["operator"] = "";

    //返回控制
    sub_root4["returnTotalRows"] = 1; // 1 显示总量信息
    sub_root4["startIndex"] = 0;
    sub_root4["pageSize"] = 8000;

    // 根参数
    sub_root1["requestInfo"] = sub_root2;
    sub_root1["resultColumn"] = retValue; //返回结果列表
    sub_root1["pagingInfo"] = sub_root4;
    sub_root1["orderBy"] = "";
    sub_root1["conditionLogical"] = "";

    sub_root1["searchCondition"] = query; //查询列表

    //内容
    sub_root0["content"] = sub_root1;

    root["params"] = sub_root0;

    return;
}

CItilIntf::CItilIntf() :
    m_sSvrIP("172.16.8.71"), m_uiSvrPort(80), m_sUrl("/api/query/get"),
            m_sSchemeId("Server"), m_sSystemId("200912232")
{
}

CItilIntf::CItilIntf(const string &sSvrIP, const unsigned int uiSvrPort) :
    m_sSvrIP(sSvrIP), m_uiSvrPort(uiSvrPort), m_sUrl("/api/query/get"),
            m_sSchemeId("Server"), m_sSystemId("200912232")
{
}

void CItilIntf::SetSchemeId(const string& sSchemeId)
{
    m_sSchemeId = sSchemeId;
    return;
}

void CItilIntf::SetSyetemId(const string& sSystemId)
{
    m_sSystemId = sSystemId;
    return;
}

bool CItilIntf::Request(const Json::Value& queryCond,
        const Json::Value& expReturn, Json::Value& arrayObj, string& sErrMsg)
{
    Json::Value root;

    setJsonRoot(root, queryCond, expReturn);

    char* pszRecv = NULL;

    Json::FastWriter writer;
    string out = writer.write(root);

    if (!PostDataToHttp(m_sUrl.c_str(), out.c_str(), m_sSvrIP.c_str(),
            m_uiSvrPort, &pszRecv))
    {
        return false;
    }

    bool bRet = ParseJson(pszRecv, arrayObj, sErrMsg);

    if (NULL != pszRecv)
    {
        free(pszRecv);
    }

    return bRet;
}

