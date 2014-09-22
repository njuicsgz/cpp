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

#include "cmysql.h"
#include "FastPing.h"

using namespace std;

string g_sDate("");

//每次删除操作时，不可以删除超过总量G_MAX_DEL_TIMES分之一的IP
const unsigned G_MAX_DEL_TIMES = 3;

const unsigned G_PING_DELAY_MAX = 2000000;
const unsigned G_PING_DELAY_MIN = 10;

const unsigned G_ENTRY_IP_NUM_MIN = 100;

const char G_IP_SRC_PING_OK[] = "ping_ok_ip";

const char G_PING_IP_SPACE[] =
        "/usr/local/idc_speed_v3/tools/upd_ip_ping_to_db/data/final_global_ip";

const char COUNTRY_INFO_FILE[] =
        "/usr/local/idc_speed_v3/tools/upd_ip_ping_to_db/data/country_iso_3166_1.txt";

const char G_DB_HOST[] = "10.137.153.111";
const char G_DB_USER[] = "idcspeed";
const char G_DB_PASS[] = "idcspeed%10.137.153.111@2012";

const char szProvinceCN[][32] = {"NULL", "黑龙江", "吉林", "辽宁", "北京", "天津", "河北",
        "山东", "山西", "陕西", "内蒙古", "宁夏", "甘肃", "青海", "新疆", "西藏", "四川", "重庆",
        "湖北", "河南", "台湾", "安徽", "江苏", "上海", "浙江", "湖南", "江西", "福建", "贵州", "云南",
        "广东", "广西", "海南"};

const char szProvinceEN[][32] = {"NULL", "heilongjiang", "jilin", "liaoning",
        "beijing", "tianjin", "hebei", "shandong", "shanxi", "shannxi",
        "neimeng", "ningxia", "gansu", "qinhai", "xinjiang", "xizang",
        "sichuan", "chongqing", "hubei", "henan", "taiwan", "anhui", "jiangsu",
        "shanghai", "zhejiang", "hunan", "jiangxi", "fujian", "guizhou",
        "yunnan", "guangdong", "guangxi", "hainan"};

const char szIspCN[][32] = {"NULL", "电信", "联通", "教育网", "移动", "长宽", "天威", "铁通",
        "电信通", "歌华有线", "东方有线"};
const char szIspEN[][32] = {"NULL", "CHINANET", "CNCGROUP", "CERNET", "CMNET",
        "GREATWALLNET", "TOPWAYNET", "TIETONG", "BEIJINGDXT", "GEHUANET",
        "DONGFANGYX"};

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
        return uCountry * 10000 + uProvince * 100 + uIsp;;
    }

    bool operator <(const SIPAttrShort &ias) const
    {
        return serial() < ias.serial();
    }
};

struct IPAttr
{
    char szIP[16];
    unsigned int uCountry;
    unsigned int uProvince;
    unsigned int uIsp;
};

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

string StrUpper(const string &str)
{
    string s1 = str;
    transform(s1.begin(), s1.end(), s1.begin(), ::toupper);
    return s1;
}

void LoadCountryInfo(map<string, SCountryInfo> &mapSN2Cty, map<unsigned,
        SCountryInfo> &mapID2Cty, const char *pfile)
{
    ifstream fin(pfile, ifstream::in);
    if (!fin.good())
    {
        cout << "open file error: " << pfile << endl;
        return;
    }

    char szBuf[1024];
    while (fin.getline(szBuf, sizeof(szBuf) - 1))
    {
        SCountryInfo ci;
        if (5 == sscanf(szBuf, "%[^,],%[^,],%[^,],%u,%*[^,],%s", ci.szFullName,
                ci.szShortName2, ci.szShortName3, &ci.uID, ci.szChnName))
        {
            mapSN2Cty[string(ci.szShortName2)] = ci;
            mapID2Cty[ci.uID] = ci;
        }
    }

    fin.close();

    cout << "4.1 Load country size: " << mapSN2Cty.size() << endl;
}

unsigned GetCountryID(const string &sShortName,
        map<string, SCountryInfo> &mapSN2Cty)
{
    string s = StrUpper(sShortName);
    if (mapSN2Cty.end() == mapSN2Cty.find(s))
        return 0;
    else
        return mapSN2Cty[s].uID;
}

unsigned GetProID(const char *pszName)
{
    for(unsigned int i = 0; i < sizeof(szProvinceCN) / sizeof(szProvinceCN[0]); i++)
    {
        if (strstr(pszName, szProvinceCN[i]) != NULL)
            return i;
    }

    for(unsigned int i = 0; i < sizeof(szProvinceEN) / sizeof(szProvinceEN[0]); i++)
    {
        if (strstr(pszName, szProvinceEN[i]) != NULL)
            return i;
    }

    return 0;
}

unsigned GetIspID(const char *pszName)
{
    for(unsigned int i = 0; i < sizeof(szIspCN) / sizeof(szIspCN[0]); i++)
    {
        if (strstr(pszName, szIspCN[i]) != NULL)
            return i;
    }

    for(unsigned int i = 0; i < sizeof(szIspEN) / sizeof(szIspEN[0]); i++)
    {
        if (strstr(pszName, szIspEN[i]) != NULL)
            return i;
    }

    return 0;
}

int GetIPInfo(const char *pszFileName, map<unsigned int, IPAttr> &mapIPInfo,
        map<string, SCountryInfo> &mapSN2Cty)
{

    FILE *fpRead = fopen(pszFileName, "r");
    if (fpRead == NULL)
    {
        printf("Open file %s Error\n", pszFileName);
        return -1;
    }

    IPAttr ip;
    char szIP[32], szCountry[32], szAddr[32], szISP[32];
    unsigned int ulIP;
    char szBuff[256];
    while (fgets(szBuff, sizeof(szBuff), fpRead))
    {
        memset(&ip, 0x00, sizeof(IPAttr));

        if (sscanf(szBuff, "%s\t%s\t%s\t%s", szIP, szCountry, szAddr, szISP)
                != 4)
            continue;

        ulIP = inet_addr(szIP);
        ulIP = ulIP << 8;

        strncpy(ip.szIP, szIP, sizeof(ip.szIP));
        ip.uCountry = GetCountryID(szCountry, mapSN2Cty);
        ip.uProvince = GetProID(szAddr);
        ip.uIsp = GetIspID(szISP);

        mapIPInfo[ulIP] = ip;
    }

    fclose(fpRead);

    return 0;
}

//IP长整到string的转换
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

const char szIspCN[][32] = {"NULL", "电信", "联通", "教育网", "移动", "长宽", "天威", "铁通",
        "电信通", "歌华有线", "东方有线"};

bool GetIPSrcFromDB(set<string> &setIPSrc, CMysql &hLocalMysql,
        const string &sIPSrc)
{
    char szSql[128] = {0};
    snprintf(szSql, sizeof(szSql) - 1,
            "select * from db_idc_speed_2.tbl_ip_source where ip_src = '%s';",
            sIPSrc.c_str());
    try
    {
        hLocalMysql.Query(szSql);
        hLocalMysql.StoreResult();

        char * pField = NULL;
        while (NULL != hLocalMysql.FetchRow())
        {
            pField = hLocalMysql.GetField("ip");
            if (pField)
                setIPSrc.insert(pField);
        }
    } catch (CCommonException & ex)
    {
        return false;
    }

    return true;
}

bool ExcSQL(vector<string> &vecSql, CMysql &hLocalMysql)
{
    for(size_t i = 0; i < vecSql.size(); i++)
    {
        try
        {
            hLocalMysql.Query((char *) vecSql[i].c_str());
        } catch (CCommonException & ex)
        {
            return false;
        }
    }

    return true;
}

bool UpdIPSrcToDB(set<string> &setIP, map<unsigned long, unsigned long> &mapIP,
        CMysql &hLocalMysql)
{
    //1. insert into db
    string sIP("");
    char szTmp[1024];
    unsigned long ulIP, ulNoChg = 0;
    vector < string > vecIns;

    map<unsigned long, unsigned long>::iterator it_map;
    for(it_map = mapIP.begin(); it_map != mapIP.end(); it_map++)
    {
        ulIP = it_map->first;
        sIP = getIPStr(ulIP);

        //insert VALID ip
        if (setIP.find(sIP) == setIP.end() && it_map->second < G_PING_DELAY_MAX
                && it_map->second > G_PING_DELAY_MIN)
        {
            snprintf(
                    szTmp,
                    sizeof(szTmp) - 1,
                    "insert into db_idc_speed_2.tbl_ip_source values('%s', '0','0','0','%s','%s',0);",
                    sIP.c_str(), G_IP_SRC_PING_OK, g_sDate.c_str());
            vecIns.push_back(string(szTmp));
        }
    }

    if (!ExcSQL(vecIns, hLocalMysql))
    {
        cout << "5.ExcSQL for insert db_idc_speed_2.tbl_ip_source Error"
                << endl;
        return false;
    }

    cout << "5.db_idc_speed_2.tbl_ip_source,  Insert: " << vecIns.size();

    //2. delete vector, the ip only from G_IP_SRC_PING_OK
    vector < string > vecDel;
    set<string>::iterator it_set;
    for(it_set = setIP.begin(); it_set != setIP.end(); it_set++)
    {
        ulIP = inet_addr((*it_set).c_str());
        if (mapIP.find(ulIP) == mapIP.end())
        {
            snprintf(
                    szTmp,
                    sizeof(szTmp) - 1,
                    "delete from db_idc_speed_2.tbl_ip_source where ip='%s' and ip_src = '%s';",
                    (*it_set).c_str(), G_IP_SRC_PING_OK);
            vecDel.push_back(string(szTmp));
        }
        else
        {
            ulNoChg++;
        }
    }

    if (vecDel.size() * G_MAX_DEL_TIMES > setIP.size())
    {
        printf("del/total: [%u/%u], can't delete so much ip once."
            " the max num is 1/%u\n", vecDel.size(), setIP.size(),
                G_MAX_DEL_TIMES);
        return true;
    }

    if (!ExcSQL(vecDel, hLocalMysql))
    {
        cout << "5.ExcSQL for deletet db_idc_speed_2.tbl_ip_source Error."
                << endl;
        return false;
    }

    cout << ", Delete: " << vecDel.size() << ", NoChange: " << ulNoChg << endl;

    return true;
}

bool GetIP(vector<unsigned long> &vecIP, const char *pFile)
{
    FILE *fp = fopen(pFile, "r");
    if (fp == NULL)
    {
        cout << "Error when open file: " << pFile << endl;
        return false;
    }

    char szBuff[1024];
    char szIP[1024];
    string strIP;
    while (fgets(szBuff, sizeof(szBuff), fp))
    {
        sscanf(szBuff, "%s\t", szIP);
        strIP = szIP;
        strIP = strIP.substr(0, strIP.length() - 1);
        strIP += "1";

        vecIP.push_back(inet_addr(strIP.c_str()));
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

void GetPingIPAgain(map<unsigned long, unsigned long> &mapIP)
{
    //4.1 load country attribute
    map<string, SCountryInfo> mapSN2Cty;
    map<unsigned, SCountryInfo> mapID2Cty;
    LoadCountryInfo(mapSN2Cty, mapID2Cty, COUNTRY_INFO_FILE);

    //4.2 Load final_global_ip address
    map<unsigned int, IPAttr> mapIPAttr;
    if (0 != GetIPInfo(G_PING_IP_SPACE, mapIPAttr, mapSN2Cty))
        return;

    cout << "4.2 Load final_global_ip OK, get ip num: " << mapIPAttr.size()
            << endl;

    //4.3 get IPAttr entry num
    size_t cnt = 0;
    unsigned long ulIP, ulIP2;

    map<SIPAttrShort, set<unsigned long> > mapEntry2IPSet;
    map<unsigned long, unsigned long>::iterator it;
    for(it = mapIP.begin(); it != mapIP.end(); it++)
    {
        if (it->second < G_PING_DELAY_MAX && it->second > G_PING_DELAY_MIN)
        {
            ulIP = it->first;
            ulIP2 = ulIP << 8;
            if (mapIPAttr.find(ulIP2) != mapIPAttr.end())
            {
                IPAttr &ia = mapIPAttr[ulIP2];

                SIPAttrShort ias(ia.uCountry, ia.uIsp, ia.uProvince);
                mapEntry2IPSet[ias].insert(ulIP);

                cnt++;
            }
        }
    }

    cout << "4.3 Detect second, get entry: " << mapEntry2IPSet.size()
            << ", ip_num: " << cnt << endl;

    //4.4 get vecIP that will detect all C segment again
    cnt = 0;
    vector<unsigned long> vecIP;
    set<unsigned long>::iterator it_set;
    map<SIPAttrShort, set<unsigned long> >::iterator it2;
    for(it2 = mapEntry2IPSet.begin(); it2 != mapEntry2IPSet.end(); it2++)
    {
        set<unsigned long> &setIP = it2->second;
        if (setIP.size() < G_ENTRY_IP_NUM_MIN)
        {
            cnt++;
            for(it_set = setIP.begin(); it_set != setIP.end(); it_set++)
            {
                ulIP = *it_set;

                for(int i = 1; i < 254; i++)
                {
                    ulIP2 = htonl(ntohl(ulIP) + i);
                    vecIP.push_back(ulIP2);
                }
            }
        }
    }

    cout << "4.4 Detect second, entry_less_100: " << cnt << ", ip.1 :"
            << vecIP.size() / 253 << ", will detect ip: " << vecIP.size()
            << endl;

    //4.5 ping again
    string sFile("../data/result/" + g_sDate + ".second");

    CFastPing fp(10000000, sFile.c_str());
    fp.FastPing(vecIP, 1);
    map<unsigned long, unsigned long> mapIP2;
    fp.GetPingRes(mapIP2);

    //4.8 add to mapIP
    cnt = 0;
    map<unsigned long, unsigned long>::iterator it3;
    for(it3 = mapIP2.begin(); it3 != mapIP2.end(); it3++)
    {
        if (it3->second < G_PING_DELAY_MAX && it3->second > G_PING_DELAY_MIN)
        {
            mapIP[it3->first] = it3->second;
            cnt++;
        }
    }

    cout << "4.5 Detect second, get availabe new IP: " << cnt << endl;
}

int main(int argc, char *argv[])
{
    CMysql hMysql(G_DB_HOST, G_DB_USER, G_DB_PASS);

    //1. Load G_PING_IP_SPACE
    vector<unsigned long> vecIP;
    if (!GetIP(vecIP, G_PING_IP_SPACE))
        return -1;

    g_sDate = GetCurDate();

    cout << "\n" << g_sDate << endl;
    cout << "1.Load " << G_PING_IP_SPACE << " OK, get ip num: " << vecIP.size()
            << endl;

    //2. get source ip from tbl_ip_source
    set < string > setIPSrc;
    if (!GetIPSrcFromDB(setIPSrc, hMysql, G_IP_SRC_PING_OK))
    {
        cout << "2.GetIPSrcFromDB Error" << endl;
        return -1;
    }
    else
        cout << "2.GetIPSrcFromDB OK, size:" << setIPSrc.size() << endl;

    //3. get available ip from fast ping

    string sFile("../data/result/" + g_sDate);

    time_t t1 = time(0);

    CFastPing fp(10000000, sFile.c_str());
    fp.FastPing(vecIP, 1);
    map<unsigned long, unsigned long> mapIP;
    fp.GetPingRes(mapIP);

    time_t t2 = time(0);

    cout << "3.Get FastPing First, size: " << mapIP.size()
            << ", spend minutes: " << (t2 - t1) / 60 << endl;

    //4. detect again for those entry that IP number < G_ENTRY_IP_NUM_MIN
    time_t t3 = time(0);

    GetPingIPAgain(mapIP);

    cout << "4.Get FastPing Second over,size: " << mapIP.size()
            << ", spend minutes: " << (t3 - t2) / 60 << endl;

    //5. update table tbl_ip_source
    if (!UpdIPSrcToDB(setIPSrc, mapIP, hMysql))
    {
        cout << "5.UpdIPSrcToDB Error" << endl;
        return -1;
    }

    return 0;
}
