#ifndef _FAST_PING_H_
#define _FAST_PING_H_

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <map>
#include "Epoller.h"

using namespace std;

class CFastPing
{
public:
    CFastPing(int nTimeout, const char *pszFile);
    ~CFastPing();

    int CreateNonBlockSocket(int family, int type, int protocal);
    bool FastPing(const vector<unsigned long> &vecIP, bool bDelay = false);
    void SendPing(const vector<unsigned long> &vecIP);
    static void* RecvReply(void *p);

    void GetPingRes(map<unsigned long, unsigned long> & mapIP)
    {
        mapIP = m_mapIP;
    }

private:
    bool m_bDelay;
    int m_iFD;
    int m_iLastTime;
    int m_iEpollWait;
    CEPoller m_stEpoller;
    FILE *m_iFile;
    map<unsigned long, unsigned long> m_mapIP;
};
#endif
