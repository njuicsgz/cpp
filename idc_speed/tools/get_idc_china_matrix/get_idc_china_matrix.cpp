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
#include <algorithm>

#include "cmysql.h"

using namespace std;

const char IDC_IP_CONF[] = "../data/idc_ip.conf";
const char OUTPUT_RESULT[] = "./result.china";

const char szProvinceCN[][32] = {"NULL", "黑龙江", "吉林", "辽宁", "北京", "天津", "河北",
        "山东", "山西", "陕西", "内蒙古", "宁夏", "甘肃", "青海", "新疆", "西藏", "四川", "重庆",
        "湖北", "河南", "台湾", "安徽", "江苏", "上海", "浙江", "湖南", "江西", "福建", "贵州", "云南",
        "广东", "广西", "海南"};

const char szIspCN[][32] = {"NULL", "电信", "联通", "教育网", "移动", "长宽", "天威", "铁通",
        "电信通", "歌华有线", "东方有线"};

unsigned int safe_atou(const char *s)
{
    if (NULL == s)
        return 0;

    unsigned int i = 0;
    while (isdigit(*s))
        i = i * 10 + (*s++) - '0';

    return i;
}

string GetCurDate()
{
    time_t now = time(NULL);
    struct tm ptm;
    char szBuf[64] = {0};

    localtime_r(&now, &ptm);
    strftime(szBuf, sizeof(szBuf) - 1, "%Y-%m-%d %H:%M:%S", &ptm);

    return szBuf;
}

void LoadConfIdcIP(const char *pfile, map<string, string> &mapIP2Idc)
{
    ifstream fin(pfile, ifstream::in);
    if (!fin.good())
    {
        cout << "open file error: " << pfile << endl;
        return;
    }

    char szBuf[1024];
    char szIdc[64];
    char szIP[32];
    while (fin.getline(szBuf, sizeof(szBuf) - 1))
    {
        if (2 == sscanf(szBuf, "%s %s", szIdc, szIP))
        {
            mapIP2Idc[szIP] = szIdc;
        }
    }

    fin.close();

    cout << "Load Idc-IP size: " << mapIP2Idc.size() << endl;
}

struct SIPAttrShort
{
    unsigned short uCountry;
    unsigned char uProvince;
    unsigned char uIsp;

    SIPAttrShort()
    {
        memset(this, 0x00, sizeof(SIPAttrShort));
    }

    SIPAttrShort(const unsigned short country, const unsigned char isp,
            const unsigned char prov)
    {
        uCountry = country;
        uProvince = prov;
        uIsp = isp;
    }

    unsigned serial() const
    {
        return uCountry * 10000 + uIsp * 100 + uProvince;
    }

    bool operator <(const SIPAttrShort &ias) const
    {
        return serial() < ias.serial();
    }
};

struct SDelaySum
{
    unsigned uCnt;
    unsigned long long ullDelayTotal;
    unsigned long long ullDelayAvg;

    SDelaySum()
    {
        memset(this, 0x00, sizeof(SDelaySum));
    }
};

bool GetDelayMatrix(map<SIPAttrShort, map<string, SDelaySum> > &mapMatrix,
        CMysql &hMysql, map<string, string> &mapIP2Idc,
        const string &sDateBegin, const string &sDateEnd, const bool IsBusy)
{
    char szSql[1024] = {0};

    char szTmp[8] = {0};

    int d_begin = IsBusy ? 240 : 1;
    int d_end = IsBusy ? 264 : 288;

    map<string, string>::iterator it2;
    for(it2 = mapIP2Idc.begin(); it2 != mapIP2Idc.end(); it2++)
    {
        const string &sIP = it2->first;

        snprintf(szSql, sizeof(szSql) - 1, "select * "
            "from db_idc_speed_2.tbl_delay_5min "
            "where day>='%s' and day<='%s' and ip='%s' and country=156;",
                sDateBegin.c_str(), sDateEnd.c_str(), sIP.c_str());
        try
        {
            hMysql.FreeResult();
            hMysql.Query(szSql);
            hMysql.StoreResult();

            char * pField = NULL;
            while (NULL != hMysql.FetchRow())
            {
                SIPAttrShort ips;

                pField = hMysql.GetField("country");
                ips.uCountry = safe_atou(pField);

                pField = hMysql.GetField("isp");
                ips.uIsp = safe_atou(pField);

                pField = hMysql.GetField("prov");
                ips.uProvince = safe_atou(pField);

                if (ips.uIsp == 0 || ips.uProvince == 0)
                    continue;

                SDelaySum &ds = mapMatrix[ips][sIP];

                for(int k = d_begin; k <= d_end; k++)
                {
                    snprintf(szTmp, sizeof(szTmp) - 1, "d%d", k);
                    pField = hMysql.GetField(szTmp);
                    if (pField && atoi(pField) > 0)
                    {
                        ds.ullDelayTotal += atoi(pField);
                        ds.uCnt++;
                    }
                }
            }
        } catch (exception ex)
        {
            cout << "error reason:" << ex.what() << endl;
            return false;
        }
    }

    return true;
}

void OutPut(const char * pfile,
        map<SIPAttrShort, map<string, SDelaySum> > &mapMatrix, map<string,
                string> &mapIP2Idc)
{
    ofstream fout(pfile, ifstream::out);
    if (!fout.good())
    {
        cout << "open file error: " << pfile << endl;
        return;
    }

    map<string, SDelaySum>::iterator it2;
    map<SIPAttrShort, map<string, SDelaySum> >::iterator it;

    for(it = mapMatrix.begin(); it != mapMatrix.end(); it++)
    {
        fout << "isp_province/idc\t";
        map<string, SDelaySum> &mapIP2Delay = it->second;
        for(it2 = mapIP2Delay.begin(); it2 != mapIP2Delay.end(); it2++)
        {
            const string &sIP = it2->first;
            fout << mapIP2Idc[sIP] << "\t";
        }

        fout << endl;
        break;
    }

    for(it = mapMatrix.begin(); it != mapMatrix.end(); it++)
    {
        const SIPAttrShort &ips = it->first;
        map<string, SDelaySum> &mapIP2Delay = it->second;

        fout << szIspCN[ips.uIsp] << "_" << szProvinceCN[ips.uProvince] << "\t";

        for(it2 = mapIP2Delay.begin(); it2 != mapIP2Delay.end(); it2++)
        {
            const SDelaySum &ds = it2->second;

            if (ds.uCnt)
                fout << ds.ullDelayTotal / ds.uCnt << "\t";
        }

        fout << endl;
    }

    fout.close();
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cout << "Usage: " << argv[0] << " sBeginDate sEndDate busy/all" << endl;
        return -1;
    }

    string sDateBegin = argv[1];
    string sDateEnd = argv[2];
    bool IsBusy = 0 == strcmp("busy", argv[3]) ? true : false;

    CMysql hMysql("localhost", "root", "");

    cout << "\n" << GetCurDate() << endl;

    //1. Load country, idc ip config
    map < string, string > mapIP2Idc;
    LoadConfIdcIP(IDC_IP_CONF, mapIP2Idc);

    //2. Loop for every array
    map<SIPAttrShort, map<string, SDelaySum> > mapMatrix;
    if (!GetDelayMatrix(mapMatrix, hMysql, mapIP2Idc, sDateBegin, sDateEnd,
            IsBusy))
    {
        cout << "GetDelayMatrix error, exit." << endl;
        return -1;
    }

    //4. output
    OutPut(OUTPUT_RESULT, mapMatrix, mapIP2Idc);

    return 0;
}
