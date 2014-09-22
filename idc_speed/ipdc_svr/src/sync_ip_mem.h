#ifndef _SYNC_DATA_H_
#define _SYNC_DATA_H_

#include "thread_runner.h"
#include "DBProcessor.h"
#include "Helper.h"

#include <map>
#include <set>

using namespace std;

class CSyncIP: public CRunner
{
public:

    CSyncIP(const unsigned uIntervalTime, map<string, SIPAttr> &mapIPAttr,
            pthread_rwlock_t &lock);
    virtual ~CSyncIP();

    virtual void Run();
protected:
    void SyncIPAttr();

private:
    unsigned m_uIntervalTime;
    unsigned m_uLastTimeStamp;

    map<string, SIPAttr> &m_mapIPAttr;

    pthread_rwlock_t &m_lock;
};

#endif 
