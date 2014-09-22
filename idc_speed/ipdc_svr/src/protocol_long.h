#ifndef _PROTOCOL_QZONE_H_
#define _PROTOCOL_QZONE_H_

#include "guardian.h"
#include "protocol_interface.h"
#include <string>
using namespace std;

class CProtocolLong : public CProtocolInterface
{
public:
	CProtocolLong()
	{
	}
	virtual ~CProtocolLong()
	{
	
	}
	virtual int GetMsg(Message& req);
};

class CLongFactory : public CProtocolFactory
{
public:
	CLongFactory()
	{}
	virtual ~CLongFactory(){}

	virtual CProtocolInterface* CreateProtocolCache()
	{return new CProtocolLong();}
	virtual void DestroyProtocolCache(CProtocolInterface* p){delete p;}

};


#endif

