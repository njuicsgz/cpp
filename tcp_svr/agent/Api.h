#ifndef _EXC_DEAL_API_H_
#define _EXC_DEAL_API_H_

#include "commpack.h"
#include <vector>
#include <set>
#include <map>
#include <string>
using namespace std;

class CApi
{
public:
	int Test( unsigned int ulTestData, unsigned int &ulTestResultData);

	int GetFD()
	{
		return m_fdConnect;
	}

	int OnLogin(const string &strSessionKey);
	int OnHeartBeat(const string &strSessionKey);
	int OnTask1(const string &strIP, unsigned int ulTestData);
	int OnGetAllClient(vector<string> &vecClient);
	
	CApi(const string &sReportSvrIP,unsigned wPort);
	~CApi();
	void Close();
	bool Open(const char * pstrIP=NULL,unsigned short wPort=0,float fRecvTimeout=20.0,float fSendTimeout=20.0);
	
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
	float  m_fRecvTimeout;
	float m_fSendTimeout;
};
#endif

