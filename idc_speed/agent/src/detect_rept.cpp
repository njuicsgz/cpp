#include "detect_rept.h"
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

const unsigned G_PING_DELAY_MAX = 5000000;
const unsigned G_PING_DELAY_MIN = 5;
const unsigned G_VALID_BEST_IP_PERCETN = 85;

const string sDataDir = "../data/tmp/";
const string sBakDir = "../../bin";

int rm_timeout(const string &strAbsDirPath, const unsigned uTimeOutSec)
{
    if (0 == strAbsDirPath.length())
    {
        return -1;
    }

    //get work dir now
    char szPwd[256] = {0};
    if(NULL == getcwd(szPwd, sizeof(szPwd)-1))
    {
        Err("can't get current dir.");
        return -1;
    }

    DIR *pDir;
    struct dirent *ent;
    struct stat fs;
    int nDelSize = 0;

    time_t now = time(0);

    pDir = opendir(strAbsDirPath.c_str());
    if (NULL == pDir)
    {
        Err("error:can not open dir %s!", strAbsDirPath.c_str());
        return -1;
    }

    chdir(strAbsDirPath.c_str());

    while ((ent = readdir(pDir)) != NULL)
    {
        if (ent->d_type == DT_REG && 0 == stat(ent->d_name, &fs))
        {
            if (now - fs.st_mtime > uTimeOutSec)
            {
                remove(ent->d_name);
                nDelSize++;
            }
        }
    }

    chdir(szPwd);

    closedir(pDir);
    pDir = NULL;
    ent = NULL;

    return nDelSize;
}

void RandPickIP(vector<unsigned long> &vecIP, set<unsigned> &setIP,
        const unsigned uMaxNum)
{
    set<unsigned> tmp;
    unsigned index;
    const size_t uSize = setIP.size();
    set<unsigned>::iterator it;
    while (tmp.size() < uMaxNum && tmp.size() < uSize)
    {
        index = random() % uSize;

        it = setIP.begin();
        for(unsigned u = 0; u < index; u++)
            it++;

        if (tmp.find(*it) == tmp.end())
        {
            tmp.insert(*it);
            vecIP.push_back(*it);
        }
    }
}

string GetCurDate()
{
    time_t now = time(NULL);
    struct tm ptm;
    char szBuf[64] = {0};

    localtime_r(&now, &ptm);
    strftime(szBuf, sizeof(szBuf) - 1, "%Y_%m_%d_%H_%M", &ptm);

    return szBuf;
}

unsigned ComputeAvgDelay(vector<unsigned long> &vec)
{
    //sort first, then get the first G_VALID_BEST_IP_PERCETN best delay ip
    unsigned uSize = vec.size() * G_VALID_BEST_IP_PERCETN / 100;

    sort(vec.begin(), vec.end());

    unsigned sum = 0;
    for(size_t i = 0; i < uSize; i++)
    {
        sum += vec[i];
    }

    return uSize == 0 ? 0 : sum / uSize;
}

CDetectRept::CDetectRept(unsigned int uiRefreshTime,
        const unsigned uMaxCheckIPNum,
        map<SIPAttrShort, set<unsigned> > &mapAttr2IP, map<unsigned,
                SIPAttrShort> &mapIP2Attr, CMutex &mutex) :
    m_uIntervalTime(uiRefreshTime), m_uMaxCheckIPNum(uMaxCheckIPNum),
            m_mapAttr2IP(mapAttr2IP), m_mapIP2Attr(mapIP2Attr), m_mutex(mutex)
{
}

CDetectRept::~CDetectRept()
{
}

void CDetectRept::GetIPDetect(vector<unsigned long> &vecIP)
{
    vecIP.clear();

    map<SIPAttrShort, set<unsigned> >::iterator it;

    ml lock(this->m_mutex);
    for(it = m_mapAttr2IP.begin(); it != m_mapAttr2IP.end(); it++)
    {
        RandPickIP(vecIP, it->second, this->m_uMaxCheckIPNum);
    }
}

void CDetectRept::DetectRept(vector<unsigned long> &vecIP)
{
    unsigned ts = time(0);

    //1. fast ping ips
    string file1 = sDataDir + GetCurDate();
    CFastPing fp(10000000, file1.c_str());
    fp.FastPing(vecIP, 1);
    map<unsigned long, unsigned long> mapIP;
    fp.GetPingRes(mapIP);

    unsigned ts3 = time(0);

    Debug("batch ping over, ping ip:%u, spend time:%us",
            vecIP.size(), ts3-ts);

    /**
     * 2. process result:
     *      2.1 ip should has a IPAttr
     *      2.2 delay should belong to (G_PING_DELAY_MIN, G_PING_DELAY_MAX)
     *      2.3 then get best G_VALID_BEST_IP_PERCETN (85%)
     */
    map<SIPAttrShort, vector<unsigned long> > mapAttr2Delay;

    unsigned long ulIP;
    unsigned long ulDelay;
    map<unsigned long, unsigned long>::iterator it;
    for(it = mapIP.begin(); it != mapIP.end(); it++)
    {
        ulIP = it->first;
        ulDelay = it->second;
        if (this->m_mapIP2Attr.find(ulIP) != this->m_mapIP2Attr.end()
                && ulDelay > G_PING_DELAY_MIN && ulDelay < G_PING_DELAY_MAX)
        {
            mapAttr2Delay[this->m_mapIP2Attr[ulIP]].push_back(ulDelay);
        }
    }

    vector<SReptTask> vecRT;
    map<SIPAttrShort, vector<unsigned long> >::iterator it2;
    for(it2 = mapAttr2Delay.begin(); it2 != mapAttr2Delay.end(); it2++)
    {
        const SIPAttrShort &ias = it2->first;
        vector<unsigned long> &vec = it2->second;

        SReptTask rt;
        rt.uCountry = ias.uCountry;
        rt.uISP = ias.uIsp;
        rt.uProv = ias.uProvince;
        rt.uIPNum = vec.size();
        rt.uDelay = ComputeAvgDelay(vec);

        vecRT.push_back(rt);
    }

    //3. report to ipdc_svr, try 3 times
    CIPDCApi api(g_svrConfig.sIPDCSvrIP.c_str(), g_svrConfig.uIPDCSvrPort);

    size_t i = 0;
    for(i = 0; i < 3; i++)
    {
        if (0 == api.OnReptTask(vecRT, ts))
        {
            break;
        }

        sleep(10);
    }

    unsigned ts2 = time(0);
    if (3 == i)
    {
        Warn("OnReptTask 3 times error, ip[%u], send size[%u], spend time[%lus].",
                vecIP.size(), vecRT.size(), ts2-ts);

        sleep(60);
    }
    else
    {
        Info("OnReptTask OK, detect ip[%u], send size[%u], spend time[%lus].",
                vecIP.size(), vecRT.size(), ts2-ts);
    }

    //4. remove ping tmp file that timeout.
    int iRet = rm_timeout(sDataDir, g_svrConfig.uMaxPingFileRemainDay * 24
            * 3600);
    Debug("day timeout:%u, delete file count: %d", g_svrConfig.uMaxPingFileRemainDay, iRet);
}

void CDetectRept::Run()
{
    sleep(30);

    unsigned long ts_last = 0, ts_now = 0;

    for(;;)
    {
        ts_now = time(0);
        if (ts_now - ts_last < m_uIntervalTime)
        {
            sleep(5);
            continue;
        }

        ts_last = ts_now;

        vector<unsigned long> vecIP;
        GetIPDetect(vecIP);

        DetectRept(vecIP);
    }
}

