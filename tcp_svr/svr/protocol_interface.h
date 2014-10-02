#ifndef _PROTOCOL_INTERFACE_H_
#define _PROTOCOL_INTERFACE_H_

#include "message_queue.h"
#include "raw_cache.h"

class CProtocolInterface: public CRawCache
{
public:
    CProtocolInterface() :
            _access(0), _fd(-1), _send_offset(-1), _delete_flag(0)
    {
    }
    virtual ~CProtocolInterface()
    {
    }
    virtual int GetMsg(Message&) = 0;
    unsigned _flow;
    time_t _access;
    int _fd;
    Message _send_msg;
    int _send_offset;
    int _delete_flag;
};

class CProtocolFactory
{
public:
    virtual ~CProtocolFactory()
    {
    }
    virtual CProtocolInterface* CreateProtocolCache() = 0;
    virtual void DestroyProtocolCache(CProtocolInterface*) = 0;
};

class CProtocolNothing: public CProtocolInterface
{
public:
    virtual ~CProtocolNothing()
    {
    }
    virtual int GetMsg(Message& msg)
    {
        int ret = cached_len();
        if (ret > 0)
        {
            Message inside(_flow);
            inside.AttachMessage(head(), ret);
            skip(ret);
            msg = inside;
        }
        return ret;
    }
};

class CNothingFactory: public CProtocolFactory
{
public:
    virtual ~CNothingFactory()
    {
    }
    virtual CProtocolInterface* CreateProtocolCache()
    {
        return new CProtocolNothing();
    }
    virtual void DestroyProtocolCache(CProtocolInterface* p)
    {
        delete p;
    }
};

#endif

