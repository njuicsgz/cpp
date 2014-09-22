#include "CSessionMgr.h"
#include "Log.h"

/**
 * g_mapFlow2IP：存储所有的TCP连接，一个Flow可以对应到多个IP。
 * g_mapIP2Flow：仅存储长连接
 *      OnLogin的时候加入map；
 *      连接错误或者关闭的时候踢掉；
 */

CMutex g_mutexSession;
map<unsigned, string> g_mapFlow2IP;
map<string, unsigned> g_mapIP2Flow;

void MapIP2Flow(unsigned int ulFlow)
{
    ml lock(g_mutexSession);

    map<unsigned, string>::iterator it = g_mapFlow2IP.find(ulFlow);
    if (it != g_mapFlow2IP.end())
    {
        g_mapIP2Flow[it->second] = ulFlow;

        Debug("MapIP2Flow OK, ip->flow: %s->%u", (it->second).c_str(), ulFlow);
    }
    else
    {
        Debug("MapIP2Flow fail, no record in g_mapFlow2IP. flow:%u", ulFlow);
    }
}

void GetAllSession(vector<string> &vecSession)
{
    ml lock(g_mutexSession);
    map<string, unsigned>::iterator it = g_mapIP2Flow.begin();
    for(; it != g_mapIP2Flow.end(); ++it)
    {
        vecSession.push_back(it->first);
    }
}

bool GetFlowByIP(const string &strIP, unsigned int &ulFlow)
{
    ml lock(g_mutexSession);

    //debug
    map<string, unsigned>::iterator it2;
    for(it2 = g_mapIP2Flow.begin(); it2 != g_mapIP2Flow.end(); it2++)
    {
        Debug("ip->flow: %s->%u", (it2->first).c_str(), it2->second);
    }

    if (0 == g_mapIP2Flow.size())
        Debug("GetFlowByIP 0 == g_mapIP2Flow.size()");

    //debug--end

    map<string, unsigned>::iterator it = g_mapIP2Flow.find(strIP);
    if (it == g_mapIP2Flow.end())
        return false;

    ulFlow = it->second;

    return true;
}

bool GetIPByFlow(string &strIP, const unsigned uFlow)
{
    ml lock(g_mutexSession);

    //debug
    map<unsigned, string>::iterator it2;
    for(it2 = g_mapFlow2IP.begin(); it2 != g_mapFlow2IP.end(); it2++)
    {
        Debug("flow->IP: %u->%s", it2->first, (it2->second).c_str());
    }

    //debug--end

    map<unsigned, string>::iterator it = g_mapFlow2IP.find(uFlow);
    if (it == g_mapFlow2IP.end())
        return false;

    strIP = it->second;

    return true;
}

void MapFlow2IP(const string &strIP, unsigned int ulFlow)
{
    ml lock(g_mutexSession);
    g_mapFlow2IP[ulFlow] = strIP;

    Debug("MapFlow2IP flow->ip: %u->%s", ulFlow, strIP.c_str());
}

void CloseSession(unsigned int ulFlow)
{
    ml lock(g_mutexSession);

    map<unsigned, string>::iterator it = g_mapFlow2IP.find(ulFlow);
    //assert(it != g_mapFlow2IP.end());
    if(g_mapFlow2IP.end() == it)
        return;

    map<string, unsigned>::iterator it_ip = g_mapIP2Flow.begin();
    for(it_ip = g_mapIP2Flow.begin(); it_ip != g_mapIP2Flow.end(); it_ip++)
    {
        //flow->ip 双映射的时候才删除
        if (it_ip->first == it->second && it_ip->second == it->first)
            g_mapIP2Flow.erase(it_ip);
    }

    g_mapFlow2IP.erase(it);

    Debug("close session, flow->ip: %u->%s", ulFlow, it->second.c_str());
}
