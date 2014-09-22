
#ifndef _CACHED_CONN_H_
#define _CACHED_CONN_H_

#include <time.h>
#include <map>
#include <deque>

#include "guardian.h"
#include "protocol_interface.h"
#include "sock.h"
using namespace network;



//////////////////////////////////////////////////////////////////////////
enum CachedConn_Type
{
	CachedConn_Type_ForClient=0,
	CachedConn_Type_ForServer=1
};
struct CFDAttr;
class CRunStatistic;
class CConnManager;
class CachedConn
{
public:
	int CloseConnect(unsigned flow);
	CRunStatistic * GetRunStatistic();
	void SetRunStatistic(CRunStatistic * pRunStatistic);
	CachedConn(CProtocolFactory& pf) : _pf(pf){}
	~CachedConn(){}
	
	void AddConn(int fd, unsigned flow);	
	unsigned CloseConn(int fd);
	int CloseFlow(unsigned flow);		
	int Recv(int fd, unsigned& flow);
	int GetMessage(int fd, Message& msg);

	int Send(unsigned flow, const char* sData, size_t iDataLen, int& fd);

	int Send(unsigned flow, big_fd_set& fd_set, const char* sData, size_t iDataLen);

	int Send(int fd, unsigned& flow);

	int FD(unsigned flow);

	void CheckTimeout(time_t access_deadline, std::deque<unsigned>& timeout_flow);
	void SetConnType(int iConnType);

protected:
	unsigned Close(int fd);
	
	typedef std::map<int, CProtocolInterface*>::iterator iterator;
	std::map<int, CProtocolInterface*> _fd_attr;
	std::map<unsigned, int> _flow_2_fd;

	CMutex _mutex;
	CProtocolFactory& _pf;	
	int _iConnType;

};

//////////////////////////////////////////////////////////////////////////
#endif//_CACHED_CONN_H_
///:~





















