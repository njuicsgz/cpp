#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <errno.h>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <time.h>
#include <algorithm>
#include "cmysql.h"
#include "FastPing.h"

using namespace std;

const unsigned G_CHINA_ID = 156;

const unsigned G_MAX_PING_NUM = 500;

const unsigned G_VALID_BEST_IP_PERCETN = 85;

const char G_COUNTRY_ISP_CONF_FILE[] = "../conf/country_isp.conf";

const char G_DB_HOST[] = "10.137.153.111";
const char G_DB_USER[] = "idcspeed";
const char G_DB_PASS[] = "idcspeed%10.137.153.111@2012";

const char szProvinceCN[][32] = {"NULL", "黑龙江", "吉林", "辽宁", "北京", "天津", "河北",
        "山东", "山西", "陕西", "内蒙古", "宁夏", "甘肃", "青海", "新疆", "西藏", "四川", "重庆",
        "湖北", "河南", "台湾", "安徽", "江苏", "上海", "浙江", "湖南", "江西", "福建", "贵州", "云南",
        "广东", "广西", "海南"};

const char szIspCN[][32] = {"NULL", "电信", "联通", "教育网", "移动", "长宽", "天威", "铁通",
        "电信通", "歌华有线", "东方有线"};

struct SIPAttr
{
    string sIP;
    unsigned uCountry;
    unsigned uIsp;
    unsigned uProv;

    SIPAttr()
    {
        uCountry = uIsp = uProv = 0;
    }

    SIPAttr(unsigned cty, unsigned isp, unsigned prov)
    {
        uCountry = cty;
        uIsp = isp;
        uProv = prov;
    }

    bool operator <(const SIPAttr& ia) const
    {
        return 10000 * uCountry + uIsp * 100 + uProv < ia.uCountry * 10000
                + ia.uIsp * 100 + ia.uProv;
    }
};

string getIPStr(unsigned long nIP)
{
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = nIP;
    char * pszIP = inet_ntoa(addr.sin_addr);
    string sIP("");
    if (pszIP)
        sIP = pszIP;
    return sIP;
}

bool getIPUL(unsigned long &ulIP, const string &sIP)
{
    ulIP = 0;
    struct in_addr addr = {0};
    if (0 == inet_aton(sIP.c_str(), &addr))
        return false;

    ulIP = addr.s_addr;
    return true;
}

struct SCountryInfo
{
    unsigned uID;
    char szShortName2[4];
    char szShortName3[4];
    char szChnName[128];
    char szFullName[128];

    SCountryInfo()
    {
        memset(this, 0x00, sizeof(SCountryInfo));
    }
};

unsigned int safe_atou(const char *s)
{
    if (NULL == s)
        return 0;

    unsigned int i = 0;
    while (isdigit(*s))
        i = i * 10 + (*s++) - '0';

    return i;
}

bool GetCountryFromDB(map<unsigned, SCountryInfo> &mapCN2Cty,
        CMysql &hLocalMysql)
{
    mapCN2Cty.clear();

    char szSql[128] = {0};
    snprintf(szSql, sizeof(szSql) - 1,
            "select * from db_idc_speed_2.tbl_country;");
    try
    {
        hLocalMysql.Query(szSql);
        hLocalMysql.StoreResult();

        char * pField = NULL;
        while (NULL != hLocalMysql.FetchRow())
        {
            SCountryInfo ci;
            pField = hLocalMysql.GetField("cid");
            ci.uID = safe_atou(pField);

            pField = hLocalMysql.GetField("name_short_2");
            if (pField)
                strncpy(ci.szShortName2, pField, sizeof(ci.szShortName2));

            pField = hLocalMysql.GetField("name_short_3");
            if (pField)
                strncpy(ci.szShortName3, pField, sizeof(ci.szShortName3));

            pField = hLocalMysql.GetField("name_cn");
            if (pField)
                strncpy(ci.szChnName, pField, sizeof(ci.szChnName));

            pField = hLocalMysql.GetField("name_en");
            if (pField)
                strncpy(ci.szFullName, pField, sizeof(ci.szFullName));

            mapCN2Cty[ci.uID] = ci;
        }
    } catch (CCommonException & ex)
    {
        return false;
    }

    return true;
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

bool Split2Vec(vector<unsigned> &vec, const string &s, const char *delim)
{
    size_t size = s.size() + 2;
    char *buf = NULL;
    if (!(buf = new (std::nothrow) char[size]))
        return false;

    snprintf(buf, size - 1, "%s", s.c_str());
    char *p = strtok(buf, delim);
    while (p)
    {
        vec.push_back(safe_atou(p));
        p = strtok(NULL, delim);
    }

    return true;
}

bool LoadCountryInfo(vector<SIPAttr> &vecTarget, const char *pfile)
{
    vecTarget.clear();

    ifstream fin(pfile, ifstream::in);
    if (!fin.good())
    {
        cout << "open file error: " << pfile << endl;
        return false;
    }

    char szBuf[4096];
    vector<unsigned> vecCountry, vecIsp;

    if (fin.getline(szBuf, sizeof(szBuf) - 1))
        Split2Vec(vecCountry, string(szBuf), ";");
    if (fin.getline(szBuf, sizeof(szBuf) - 1))
        Split2Vec(vecIsp, string(szBuf), ";");

    fin.close();

    for(size_t i = 0; i < vecCountry.size(); i++)
    {
        unsigned uCty = vecCountry[i];

        //not gen china
        if (G_CHINA_ID == uCty || 0 == uCty)
            continue;

        SIPAttr ip(uCty, 0, 0);
        vecTarget.push_back(ip);
    }

    unsigned uIspCnt = sizeof(szIspCN) / sizeof(szIspCN[0]);
    for(size_t j = 0; j < vecIsp.size(); j++)
    {
        unsigned isp = vecIsp[j];

        //not gen china
        if (isp >= uIspCnt)
            continue;

        for(size_t k = 1; k <= 32; k++)
        {
            SIPAttr ip(G_CHINA_ID, isp, k);
            vecTarget.push_back(ip);
        }
    }

    cout << "Load file: " << pfile << " ok, get country: " << vecCountry.size()
            << ", isp: " << vecIsp.size() << ", target total size: "
            << vecTarget.size() << endl;

    return true;
}

bool GetIPFromDB(vector<unsigned long> &vecIP,
        vector<unsigned long> &vecIPTotal, CMysql &hLocalMysql,
        const SIPAttr &ia)
{
    vecIP.clear();

    char szSql[128] = {0};
    snprintf(szSql, sizeof(szSql) - 1,
            "select ip from db_idc_speed_2.tbl_ip_attr "
                "where country=%u and isp=%u and prov=%u limit %u;",
            ia.uCountry, ia.uIsp, ia.uProv, G_MAX_PING_NUM);
    try
    {
        hLocalMysql.Query(szSql);
        hLocalMysql.StoreResult();

        char * pField = NULL;
        unsigned long ulIP;
        while (NULL != hLocalMysql.FetchRow())
        {
            pField = hLocalMysql.GetField("ip");
            if (pField)
            {
                if (getIPUL(ulIP, string(pField)))
                {
                    vecIP.push_back(ulIP);
                    vecIPTotal.push_back(ulIP);
                }
            }
        }
    } catch (CCommonException & ex)
    {
        return false;
    }

    return true;
}

string GetFileName(const string &s, const SIPAttr &ia)
{
    char szBuf[64];
    snprintf(szBuf, sizeof(szBuf) - 1, "../data/%s/%u_%u_%u_%s", s.c_str(),
            ia.uCountry, ia.uIsp, ia.uProv, GetCurDate().c_str());
    return string(szBuf);
}

void LogDetail(vector<unsigned> &vecDelay, const vector<unsigned long> &vecIP,
        map<unsigned long, unsigned long> &mapIP, const string &file2)
{
    ofstream fout(file2.c_str(), ifstream::out);
    if (!fout.good())
    {
        cout << "open file error: " << file2 << endl;
        return;
    }

    for(size_t i = 0; i < vecIP.size(); i++)
    {
        unsigned long ulIP = vecIP[i];

        if (mapIP.end() != mapIP.find(ulIP))
        {
            if (mapIP[ulIP] > 0)
            {
                vecDelay.push_back(mapIP[ulIP]);

                string sIP = getIPStr(ulIP);
                fout << sIP << "\t" << mapIP[ulIP] << endl;
            }
        }
    }

    fout.close();
}

unsigned ComputeAvgDelay(vector<unsigned> &vec)
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

void LogResult(map<SIPAttr, vector<unsigned long> > &mapTar2IPVec, map<
        unsigned long, unsigned long> &mapIP,
        map<unsigned, SCountryInfo> &mapCN2Cty)
{
    string sf_sum_global = "../data/" + GetCurDate() + ".global";
    string sf_sum_china = "../data/" + GetCurDate() + ".china";
    FILE *pFileGlb = fopen(sf_sum_global.c_str(), "w+");
    FILE *pFileChn = fopen(sf_sum_china.c_str(), "w+");
    if (!pFileGlb || !pFileChn)
    {
        cout << "open file error: " << sf_sum_global << " or " << sf_sum_china
                << endl;
        exit(-1);
    }

    map<SIPAttr, vector<unsigned long> >::iterator it;
    for(it = mapTar2IPVec.begin(); it != mapTar2IPVec.end(); it++)
    {
        const SIPAttr &ia = it->first;
        vector<unsigned long> &vecIP = it->second;

        if (0 == vecIP.size())
            continue;

        string sf_detail = GetFileName("detail", ia);

        vector<unsigned> vecDelay;
        LogDetail(vecDelay, vecIP, mapIP, sf_detail);

        //compute the avg delay, use algorithm defined by user
        unsigned uDelayAvg = ComputeAvgDelay(vecDelay);

        if (G_CHINA_ID == ia.uCountry)
        {
            fprintf(
                    pFileChn,
                    "%s_%s_%s\tavg_delay[%ums]\tIP_DB[%u], IP_PING[%u], IP_Valid[%u%%], IP_Select[%u%%]\n",
                    mapCN2Cty[ia.uCountry].szChnName, szIspCN[ia.uIsp],
                    szProvinceCN[ia.uProv], uDelayAvg / 1000, vecIP.size(),
                    vecDelay.size(), vecDelay.size() * 100 / vecIP.size(),
                    G_VALID_BEST_IP_PERCETN);
            /*
             printf(
             "%s_%s_%s\tavg_delay[%ums]\tIP_DB[%u], IP_PING[%u], IP_Valid[%u%%], IP_Select[%u%%]\n",
             mapCN2Cty[ia.uCountry].szChnName, szIspCN[ia.uIsp],
             szProvinceCN[ia.uProv], uDelayAvg / 1000, vecIP.size(),
             vecDelay.size(), vecDelay.size() * 100 / vecIP.size(),
             G_VALID_BEST_IP_PERCETN);
             */
        }
        else
        {
            fprintf(
                    pFileGlb,
                    "%s[%s]\tavg_delay[%ums]\tIP_DB[%u], IP_PING[%u], IP_Valid[%u%%], IP_Select[%u%%]\n",
                    mapCN2Cty[ia.uCountry].szChnName,
                    mapCN2Cty[ia.uCountry].szShortName2, uDelayAvg / 1000,
                    vecIP.size(), vecDelay.size(), vecDelay.size() * 100
                            / vecIP.size(), G_VALID_BEST_IP_PERCETN);
            /*
             printf(
             "%s[%s]\tavg_delay[%ums]\tIP_DB[%u], IP_PING[%u], IP_Valid[%u%%], IP_Select[%u%%]\n",
             mapCN2Cty[ia.uCountry].szChnName,
             mapCN2Cty[ia.uCountry].szShortName2, uDelayAvg / 1000,
             vecIP.size(), vecDelay.size(), vecDelay.size() * 100
             / vecIP.size(), G_VALID_BEST_IP_PERCETN);

             */
        }
    }

    fclose(pFileChn);
    fclose(pFileGlb);
}

int main(int argc, char *argv[])
{
    CMysql hMysql(G_DB_HOST, G_DB_USER, G_DB_PASS);
    hMysql.Query("set names latin1;");

    time_t t1 = time(0);

    //1. Load target entry (country-isp-pro) that will detect
    vector<SIPAttr> vecTarget;
    if (!LoadCountryInfo(vecTarget, G_COUNTRY_ISP_CONF_FILE))
        return -1;

    //2. get country info from db, in order to print chinese info
    map<unsigned, SCountryInfo> mapCN2Cty;
    if (!GetCountryFromDB(mapCN2Cty, hMysql))
    {
        cout << "GetCountryFromDB Error" << endl;
        return -1;
    }

    cout << "GetCountryFromDB OK, size: " << mapCN2Cty.size() << endl;

    //3. get target entry to be detected.
    vector<unsigned long> vecIPTotal;
    map<SIPAttr, vector<unsigned long> > mapTar2IPVec;

    for(size_t i = 0; i < vecTarget.size(); i++)
    {
        SIPAttr &ia = vecTarget[i];

        vector<unsigned long> vecIP;
        if (!GetIPFromDB(vecIP, vecIPTotal, hMysql, ia))
        {
            cout << "GetIPSrcFromDB Error" << endl;
        }

        if (0 == vecIP.size())
            continue;

        //3.2 store ips
        mapTar2IPVec[ia] = vecIP;
    }

    time_t t3 = time(0);
    cout << "get entry from db, spend: " << (t3 - t1) << "s" << endl;

    cout << "will detect entry: " << mapTar2IPVec.size() << ", total ips: "
            << vecIPTotal.size() << endl;

    //4. ping all ips
    string file1 = "../data/tmp/" + GetCurDate();
    CFastPing fp(10000000, file1.c_str());
    fp.FastPing(vecIPTotal, 1);
    map<unsigned long, unsigned long> mapIP;
    fp.GetPingRes(mapIP);

    cout << "Fast ping over." << endl;

    //5. record to file
    LogResult(mapTar2IPVec, mapIP, mapCN2Cty);

    //4. time output
    time_t t2 = time(0);
    cout << "detecting spend: " << (t2 - t1) << "s" << endl;

    return 0;
}
