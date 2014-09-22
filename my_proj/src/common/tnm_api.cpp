#include "tnm_api.h"

const string G_BASIC_ATTR = "getdata.get_servers_basic_attr";

bool ParseJson(const string& sValue, Json::Value& output, string& sErrMsg)
{
    Json::Reader reader;

    try
    {
        if (!reader.parse(sValue, output))
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

    return true;
}

CTNMApi::CTNMApi() :
    m_sSvrIP("172.16.8.109"), m_uiSvrPort(80), m_sUrl(
            "/new_api/getdata.get_servers_basic_attr")
{
}

CTNMApi::CTNMApi(const string &sSvrIP, const unsigned int uiSvrPort) :
    m_sSvrIP(sSvrIP), m_uiSvrPort(uiSvrPort), m_sUrl(
            "/new_api/getdata.get_servers_basic_attr")
{
}

void CTNMApi::SetUrl(const string &sUrl)
{
    this->m_sUrl = sUrl;
}

bool CTNMApi::Request(const Json::Value& input, Json::Value& output)
{
    Json::FastWriter writer;
    string out = writer.write(input);

    char* pszRecv = NULL;
    if (!PostDataToHttp(m_sUrl.c_str(), out.c_str(), m_sSvrIP.c_str(),
            m_uiSvrPort, &pszRecv))
    {
        return false;
    }

    bool bRet = ParseJson(pszRecv, output, m_sErrMsg);

    if (NULL != pszRecv)
    {
        free(pszRecv);
    }

    return bRet;
}

bool CTNMApi::FomatReqest(Json::Value &ret, const string &sMethod,
        const vector<string> &vecIP, const unsigned uAttrId,
        const string &sDate, const unsigned uCnt)
{
    SetUrl("/new_api/" + sMethod);

    //参数由method和params两部分组成，缺一不可
    Json::Value params;
    params["method"] = sMethod;

    Json::Value sub_params;
    sub_params["attr"] = uAttrId;
    sub_params["date"] = sDate;

    Json::Value json_iplist(Json::arrayValue);
    vector<string>::const_iterator it;
    for(it = vecIP.begin(); it != vecIP.end(); ++it)
    {
        json_iplist.append(Json::Value(*it));
    }
    sub_params["iplist"] = json_iplist;

    params["params"] = sub_params;

    if (!Request(params, ret))
    {
        return false;
    }

    return 0;
}

bool CTNMApi::GetEth0FlowIn(map<string, vector<unsigned> > &mapIP2FlowIn, string &sErrMsg, const vector<string> &vecIP,
        const string &sDate)
{
    Json::Value ret;
    if(!FomatReqest(ret, G_BASIC_ATTR, vecIP, NET_FLOW_IN_ETH0, sDate, 0))
    {
        sErrMsg = m_sErrMsg;
        return false;
    }

    Json::Value::Members members(ret.getMemberNames());
    Json::Value::Members::iterator it;
    for(it = members.begin();it != members.end();++it)
    {
        cout << "IP:"<< *it <<" CPU load:" << (ret[*it]).toStyledString() <<endl;

        vector<unsigned> vec;
        Json::Value tmp = ret[*it];

        if(tmp.isArray())
        {
            for(size_t i = 0; i < tmp.size(); i++)
            {
                vec.push_back(tmp[i].asUInt());
            }
        }
        mapIP2FlowIn[*it] = vec;
    }

    return true;
}
