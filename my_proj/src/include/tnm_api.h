#ifndef TNM2_API_H_2012_08_20
#define TNM2_API_H_2012_08_20

#include <iostream>
#include <string>
#include "http.h"
#include "json_inc/json.h"

using namespace std;

enum
{
    NET_PACKAGE_IN_ETH0 = 20,
    NET_PACKAGE_IN_ETH1 = 21,
    NET_PACKAGE_IN_ETH2 = 22,
    NET_PACKAGE_IN_ETH3 = 23,

    NET_PACKAGE_OUT_ETH0 = 30,
    NET_PACKAGE_OUT_ETH1 = 31,
    NET_PACKAGE_OUT_ETH2 = 32,
    NET_PACKAGE_OUT_ETH3 = 33,

    NET_FLOW_IN_ETH0 = 40,
    NET_FLOW_IN_ETH1 = 41,
    NET_FLOW_IN_ETH2 = 42,
    NET_FLOW_IN_ETH3 = 43,

    NET_FLOW_OUT_ETH0 = 50,
    NET_FLOW_OUT_ETH1 = 51,
    NET_FLOW_OUT_ETH2 = 52,
    NET_FLOW_OUT_ETH3 = 53,
};

class CTNMApi
{
public:
    CTNMApi();
    CTNMApi(const string &sSvrIP, const unsigned int uiSvrPort);
    ~CTNMApi()
    {
    }
private:
    CTNMApi(const CTNMApi &api)
    {
    }

    CTNMApi& operator =(const CTNMApi &api)
    {
        return *this;
    }

    void SetUrl(const string &sUrl);
    bool Request(const Json::Value& input, Json::Value& output);
    bool FomatReqest(Json::Value &ret, const string &sMethod, const vector<
            string> &vecIP, const unsigned uAttrId, const string &sDate,
            const unsigned uCnt);

public:
    bool GetEth0FlowIn(map<string, vector<unsigned> > &mapIP2FlowIn,
            string &sErrMsg, const vector<string> &vecIP, const string &sDate);

private:
    string m_sSvrIP;
    unsigned int m_uiSvrPort;

    string m_sUrl;
    string m_sErrMsg;
};
#endif

