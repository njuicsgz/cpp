#ifndef _DMP_TASK_SVR_API_H_
#define _DMP_TASK_SVR_API_H_

#include "commpack.h"
#include "data_struct.h"
#include <vector>
#include <set>
#include <map>
#include <string>
using namespace std;

class CIPDCApi
{
public:
    int Test(unsigned int ulTestData, unsigned int &ulTestResultData);

    int GetFD()
    {
        return m_fdConnect;
    }

    int CloseRet(Commpack * &pPkg, const int iRetValue);

    int OnLogin(const string &strSessionKey);
    int OnHeartBeat(const string &strSessionKey);
    int OnTask1(const string &strIP, unsigned int ulTestData);
    int OnGetAllClient(vector<string> &vecClient);

    //for detect agent
    int GetIPAttrByTS(vector<SIPAttr> &vecMod, vector<unsigned> &vecDel,
            unsigned &uTSNew, const unsigned uTSOld);
    int OnReptTask(const vector<SReptTask> &vecRT, const unsigned uTimestamp);

public:
    CIPDCApi(const string &sReportSvrIP, unsigned wPort);
    ~CIPDCApi();
    void Close();
    bool Open(const char * pstrIP = NULL, unsigned short wPort = 0,
            float fRecvTimeout = 120.0, float fSendTimeout = 120.0);

protected:
    bool Recv(Commpack *&pack);
    bool Send(Commpack &pack);
    unsigned int GetServerIP(const char * szServerName);
    int AsyncConnect(unsigned int ip, unsigned short port);
    int SetNonBlock(int fd);
    int SelectSingleRead(int fd, float timeout);
    int SelectSingleRead(int fd, float timeout, fd_set* r_set);
    int SelectSingleWrite(int fd, float timeout);
    int SelectSingleWrite(int fd, float timeout, fd_set* r_set);

    unsigned short GetSeq();
protected:
    int m_fdConnect;
    bool m_bConnect;
    char m_szServerIP[255];
    unsigned short m_wPort;
    float m_fRecvTimeout;
    float m_fSendTimeout;
};
#endif

