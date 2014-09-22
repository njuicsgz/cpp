#ifndef _MONITOR_REPORT_H_
#define _MONITOR_REPORT_H_

#include "thread_runner.h"
#include "ipdc_api.h"
#include "data_struct.h"
#include "helper.h"
#include "guardian.h"
#include "FastPing.h"

#include <map>
#include <set>

using namespace std;

class CDetectRept: public CRunner
{
public:

    CDetectRept(const unsigned uIntervalTime, const unsigned uMaxCheckIPNum,
            map<SIPAttrShort, set<unsigned> > &mapAttr2IP, map<unsigned,
                    SIPAttrShort> &mapIP2Attr, CMutex &m_mutex);
    virtual ~CDetectRept();

    virtual void Run();

protected:
    void GetIPDetect(vector<unsigned long> &vecIP);
    void DetectRept(vector<unsigned long> &vecIP);

private:
    unsigned m_uIntervalTime;
    unsigned m_uMaxCheckIPNum;
    map<SIPAttrShort, set<unsigned> > &m_mapAttr2IP;
    map<unsigned, SIPAttrShort> &m_mapIP2Attr;
    CMutex &m_mutex;
};

#endif 
