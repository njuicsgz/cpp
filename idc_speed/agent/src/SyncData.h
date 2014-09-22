#ifndef _SYNC_DATA_H_
#define _SYNC_DATA_H_

#include "thread_runner.h"
#include "ipdc_api.h"
#include "helper.h"
#include "guardian.h"

#include <map>
#include <set>

using namespace std;

class SyncData: public CRunner
{
public:

    SyncData(const unsigned uIntervalTime,
            map<SIPAttrShort, set<unsigned> > &mapAttr2IP, map<unsigned,
                    SIPAttrShort> &mapIP2Attr, CMutex &mutex);
    virtual ~SyncData();

    virtual void Run();
protected:
    void SyncIPAttr();

private:
    unsigned m_uIntervalTime;
    unsigned m_uLastTimeStamp;
    map<SIPAttrShort, set<unsigned> > &m_mapAttr2IP;
    map<unsigned, SIPAttrShort> &m_mapIP2Attr;
    CMutex &m_mutex;
};

#endif 
