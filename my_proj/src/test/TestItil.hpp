#include "ItilIntf.h"
#include "json_inc/json.h"

int GetIdcByIPFromItil(const char *pszItilIP, const unsigned short usItilPort,
        const string &sIp)
{
    CItilIntf itil(pszItilIP, usItilPort);

    Json::Value query_cond, query_column, query_ret;

    query_cond["serverIP"] = "10.168.128.139";

    query_column["serverLanIP"] = "";
    query_column["serverIdc"] = "";
    query_column["serverIdcId"] = "";

    string sErrMsg;
    if (!itil.Request(query_cond, query_column, query_ret, sErrMsg))
    {
        Err("SendJsonToItil, reason is [%s]", sErrMsg.c_str());
        return -1;
    }

    string sIdc;
    for(size_t i = 0; i < query_ret.size(); i++)
    {
        sIdc = query_ret[i]["serverIdc"].asString();
    }

    return 0;
}

struct SDeptBu
{
    unsigned uDeptId;
    unsigned uBu1Id;
    unsigned uBu2Id;
    unsigned uBu3Id;

    string sDeptName;
    string sBu1Name;
    string sBu2Name;
    string sBu3Name;
};

bool GetDeptBuFromItil(vector<SDeptBu> &vecDeptBu, string &sErrMsg,
        const char *pszItilIP, const unsigned short usItilPort)
{
    vecDeptBu.clear();
    CItilIntf itil(pszItilIP, usItilPort);

    itil.SetSchemeId("Business");

    Json::Value query_cond, query_column, query_ret;

    //query_cond["serverIP"] = "10.168.128.139";

    query_column["deptId"] = "";
    query_column["deptName"] = "";
    query_column["bs1NameId"] = "";
    query_column["bs1Name"] = "";
    query_column["bs2NameId"] = "";
    query_column["bs2Name"] = "";
    query_column["bs3NameId"] = "";
    query_column["bs3Name"] = "";

    if (!itil.Request(query_cond, query_column, query_ret, sErrMsg))
    {
        return false;
    }

    SDeptBu db;
    for(size_t i = 0; i < query_ret.size(); i++)
    {
        db.uDeptId = query_ret[i]["deptId"].asUInt();
        db.uBu1Id = query_ret[i]["bs1NameId"].asUInt();
        db.uBu2Id = query_ret[i]["bs2NameId"].asUInt();
        db.uBu3Id = query_ret[i]["bs3NameId"].asUInt();

        db.sDeptName = query_ret[i]["deptName"].asString();
        db.sBu1Name = query_ret[i]["bs1Name"].asString();
        db.sBu2Name = query_ret[i]["bs2Name"].asString();
        db.sBu3Name = query_ret[i]["bs3Name"].asString();

        vecDeptBu.push_back(db);
    }

    return true;
}
