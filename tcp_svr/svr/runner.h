#ifndef _RUNNER_H_
#define _RUNNER_H_

#include "sock.h"
#include "epoller.h"
#include "message_queue.h"
#include "cached_conn.h"
#include "thread_runner.h"
#include <string>
using namespace std;

extern CMutex g_mutexIP2Flow;
extern map<string, unsigned> g_mapIP2Flow;

//////////////////////////////////////////////////////////////////////////

class CReqAcceptor: public CRunner
{
public:
    void OnConnect(CSockAttacher &sock, int flow);
    CReqAcceptor(CEPoller& epoller, CachedConn& collection, MQ& Q);
    virtual ~CReqAcceptor();

    void Init(const std::string& sHost, unsigned short port);
    virtual void Run();

protected:
    network::CSocket _listen;
    CEPoller& _epoller;
    CachedConn& _conns;
    unsigned _flow;
    MQ& _Q;
};

//////////////////////////////////////////////////////////////////////////

class CReceiver: public CRunner
{
public:
    void SetReceiveType(int iReceiveType);
    CReceiver(CEPoller& epoller, CachedConn& collection, MQ& Q);
    virtual ~CReceiver();
    virtual void Run();

protected:
    CEPoller& _epoller;
    CachedConn& _conns;
    MQ& _Q;
    int _iReceiveType;
    ;
    static const int C_EPOLL_WAIT = 200;
};

//////////////////////////////////////////////////////////////////////////

class CRspSender: public CRunner
{
public:
    CRspSender(CachedConn& collection, MQ& Q, CEPoller& epoller);
    virtual ~CRspSender();
    virtual void Run();

protected:
    CachedConn& _conns;
    MQ& _Q;
    CEPoller& _epoller;
};

//////////////////////////////////////////////////////////////////////////
#endif//_FRONT_RUNNER_H_
///:~
