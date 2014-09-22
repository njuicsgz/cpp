#ifndef _CMD_PROCESS_H_
#define _CMD_PROCESS_H_

#include "message_queue.h"
#include "thread_runner.h"
#include <map>
#include <vector>
#include <list>
#include <set>
using namespace std;
class Commpack;
class CCmdProcess: public CRunner 
{
public:

	void SendToClient(unsigned int flow,Commpack *pPack);
	void ProcessCmd(unsigned int flow,Commpack * pPack);
	void OnCmdCome(unsigned int flow,char *pData,int nLen);
	CCmdProcess(MQ& req,MQ& rsp);
	virtual ~CCmdProcess();
	virtual void Run();

public:
	void OnTest(unsigned int flow, Commpack *pPack);
	void OnLogin(unsigned int flow, Commpack *pPack);
	void OnHeartbeat(unsigned int flow, Commpack *pPack);
	void OnGetAllClient(unsigned int flow, Commpack *pPack);
	void OnTask1(unsigned int flow, Commpack *pPack);

protected:
	MQ& m_ReqQ;
	MQ& m_RspQ;
};

#endif 
