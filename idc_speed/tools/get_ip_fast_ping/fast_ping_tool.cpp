#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <errno.h>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <time.h>
#include "FastPing.h"

using namespace std;

string g_sDate("");

//IP长整到string的转换
string getIPStr(unsigned long nIP)
{
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = nIP;
    char * pszIP = inet_ntoa(addr.sin_addr);
    string sIP("");
    if (pszIP)
        sIP = pszIP;
    return sIP;
}

bool GetIP(vector<unsigned long> &vecIP, const char *pFile)
{
    FILE *fp = fopen(pFile, "r");
    if (fp == NULL)
    {
        cout << "Error when open file: " << pFile << endl;
        return false;
    }

    char szBuff[1024];
    char szIP[1024];
    string strIP;
    char szTmp[32];
    while (fgets(szBuff, sizeof(szBuff), fp))
    {
        sscanf(szBuff, "%s\t", szIP);
        strIP = szIP;
        strIP = strIP.substr(0, strIP.length() - 1);

        for(size_t i = 1; i <= 255; i++)
        {
            snprintf(szTmp, sizeof(szTmp) -1, "%s%u", strIP.c_str(),i);

            vecIP.push_back(inet_addr(szTmp));
        }
    }

    return true;
}

string GetCurDate()
{
    time_t now = time(NULL);
    struct tm ptm;
    char szBuf[64] = {0};

    localtime_r(&now, &ptm);
    strftime(szBuf, sizeof(szBuf) - 1, "%Y_%m_%d_%H_%M", &ptm);

    return szBuf;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./%s ip_file\n", argv[0]);
        return 0;
    }

    //1. Load G_PING_IP_SPACE
    vector<unsigned long> vecIP;
    if (!GetIP(vecIP, argv[1]))
        return -1;

    g_sDate = GetCurDate();

    cout << "\n" << g_sDate << endl;
    cout << "Load " << argv[1] << " OK, get ip num: " << vecIP.size() << endl;

    //3. get available ip from fast ping
    string sFile("../data/result/" + g_sDate);

    time_t t1 = time(0);

    CFastPing fp(10000000, sFile.c_str());
    fp.FastPing(vecIP, 1);
    map<unsigned long, unsigned long> mapIP;
    fp.GetPingRes(mapIP);

    time_t t2 = time(0);

    cout << "Get FastPing size: " << mapIP.size() << ", spend minutes: " << (t2
            - t1) / 60 << endl;

    return 0;
}
