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
#include <time.h>

#include "cmysql.h"

using namespace std;

const char FINAL_GLOBAL_IP_FILE[] =
        "/usr/local/idc_speed_v3/tools/gen_ip_attr/data/final_global_ip.oversea";

const char COUNTRY_INFO_FILE[] =
        "/usr/local/idc_speed_v3/tools/gen_ip_attr/data/country_iso_3166_1.txt";

const char COUNTRY_ISP_CONF_FILE[] =
        "/usr/local/idc_speed_v3/tools/gen_ip_attr/data/country_isp.conf";

const char G_DB_HOST[] = "localhost";
const char G_DB_USER[] = "";
const char G_DB_PASS[] = "";

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

string GetCurDate()
{
    time_t now = time(NULL);
    struct tm ptm;
    char szBuf[64] = {0};

    localtime_r(&now, &ptm);
    strftime(szBuf, sizeof(szBuf) - 1, "%Y-%m-%d %H:%M:%S", &ptm);

    return szBuf;
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

    cout << "Load country size: " << mapSN2Cty.size() << endl;
}

bool SetCountryInfo2DB(map<unsigned, SCountryInfo> &mapID2Cty,
        CMysql &hLocalMysql)
{
    char szSql[2048] = {0};
    map<unsigned, SCountryInfo>::iterator it;
    for(it = mapID2Cty.begin(); it != mapID2Cty.end(); it++)
    {
        SCountryInfo &ci = it->second;
        snprintf(szSql, sizeof(szSql) - 1,
                "insert into db_idc_speed_2.tbl_country "
                    "values('%u','%s','%s','%s','%s');", ci.uID,
                ci.szShortName2, ci.szShortName3, ci.szChnName, ci.szFullName);
        try
        {
            hLocalMysql.Query(szSql);
        } catch (CCommonException & ex)
        {
            cout << "sql error: " << ex.GetErrMsg() << endl;
            return false;
        }
    }

    return true;
}

unsigned GetIspNotCN(const string &sCountry, const string &sIsp, map<string,
        vector<string> > &mapCtyIsp)
{
    if (mapCtyIsp.find(sCountry) == mapCtyIsp.end())
        return 0;

    vector < string > &vec = mapCtyIsp[sCountry];
    for(size_t i = 0; i < vec.size(); i++)
    {
        if (sIsp == vec[i])
            return i + 1;
    }

    return 0;
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

bool Split2Vec(const string &s, vector<string> &vec, const char *delim)
{
    vec.clear();

    size_t size = s.size() + 2;
    char *buf = NULL;
    if (!(buf = new (std::nothrow) char[size]))
        return false;

    snprintf(buf, size - 1, "%s", s.c_str());
    char *p = strtok(buf, delim);
    while (p)
    {
        vec.push_back(string(p));
        p = strtok(NULL, delim);
    }

    return true;
}

bool LoadCountryIspConf(const char *pszFileName,
        map<string, vector<string> > &mapCtyIsp)
{
    FILE *fpRead = fopen(pszFileName, "r");
    if (fpRead == NULL)
    {
        printf("Open file %s Error\n", pszFileName);
        return -1;
    }

    char szBuff[4096];
    vector<string> vec, vec2;
    while (fgets(szBuff, sizeof(szBuff) - 1, fpRead))
    {
        if(szBuff[strlen(szBuff)-1] == '\n')
            szBuff[strlen(szBuff)-1] ='\0';

        Split2Vec(string(szBuff), vec, ":");
        if (2 != vec.size() || 2 != vec[0].size() || 0 == vec[1].size())
            continue;

        Split2Vec(vec[1], vec2, ";");

        mapCtyIsp[vec[0]] = vec2;
    }

    fclose(fpRead);

    cout << "LoadCountryIspConf OK, country size: " << mapCtyIsp.size() << endl;

    return 0;
}

int GetIPInfo(const char *pszFileName, map<unsigned int, IPAttr> &mapIPInfo,
        map<string, SCountryInfo> &mapSN2Cty,
        map<string, vector<string> > &mapCtyIsp)
{
    FILE *fpRead = fopen(pszFileName, "r");
    if (fpRead == NULL)
    {
        printf("Open file %s Error\n", pszFileName);
        return -1;
    }

    IPAttr ip;
    char szIP[64], szCountry[64], szAddr[64], szISP[64];
    unsigned int ulIP;
    char szBuff[1024];
    while (fgets(szBuff, sizeof(szBuff), fpRead))
    {
        memset(&ip, 0x00, sizeof(IPAttr));

        if (sscanf(szBuff, "%s\t%s\t%s\t%s", szIP, szCountry, szAddr, szISP)
                != 4)
            continue;

        ulIP = inet_addr(szIP);
        ulIP = ulIP << 8;

        strncpy(ip.szIP, szIP, sizeof(ip.szIP) - 1);
        ip.uCountry = GetCountryID(szCountry, mapSN2Cty);
        if (0 != strcmp(szCountry, "cn"))
        {
            ip.uIsp = GetIspNotCN(string(szCountry), string(szISP), mapCtyIsp);
        }
        else
        {
            ip.uProvince = GetProID(szAddr);
            ip.uIsp = GetIspID(szISP);
        }

        mapIPInfo[ulIP] = ip;
    }

    fclose(fpRead);

    return 0;
}

struct SIPSrc
{
    char szIP[16];
    unsigned int uCountry;
    unsigned int uIsp;
    unsigned int ulPro;
    char szIPSrc[64];
    char szIPTS[32];
    unsigned char ucFlag;

    SIPSrc()
    {
        memset(this, 0x00, sizeof(SIPSrc));
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

bool GetIPSrcFromDB(vector<SIPSrc> &vecIPSrc, CMysql &hLocalMysql)
{
    char szSql[] = "select * from db_idc_speed_2.tbl_ip_source;";

    try
    {
        hLocalMysql.Query(szSql);
        hLocalMysql.StoreResult();

        SIPSrc ip;
        char * pField = NULL;
        while (NULL != hLocalMysql.FetchRow())
        {
            pField = hLocalMysql.GetField("ip");
            if (pField)
                strncpy(ip.szIP, pField, sizeof(ip.szIP));
            pField = hLocalMysql.GetField("ip_src");
            if (pField)
                strncpy(ip.szIPSrc, pField, sizeof(ip.szIPSrc));
            pField = hLocalMysql.GetField("ip_ts");
            if (pField)
                strncpy(ip.szIPTS, pField, sizeof(ip.szIPTS));

            pField = hLocalMysql.GetField("country");
            ip.uCountry = safe_atou(pField);
            pField = hLocalMysql.GetField("isp");
            ip.uIsp = safe_atou(pField);
            pField = hLocalMysql.GetField("prov");
            ip.ulPro = safe_atou(pField);
            pField = hLocalMysql.GetField("ip_flag");
            ip.ucFlag = safe_atou(pField);

            vecIPSrc.push_back(ip);
        }
    } catch (CCommonException & ex)
    {
        return false;
    }

    return true;
}

/**
 * if ip from source table has attribution, then insert into IPAttr;
 * else get ip attribution first;
 */
void GetIPAttrFromSrc(vector<IPAttr> &vecIPAttr,
        const vector<SIPSrc> &vecIPSrc, map<unsigned int, IPAttr> &mapIP)
{
    vecIPAttr.clear();

    unsigned int ulIP = 0;
    IPAttr ip_attr;
    for(size_t i = 0; i < vecIPSrc.size(); i++)
    {
        const SIPSrc &ip = vecIPSrc[i];

        if (ip.uCountry == 0 && ip.uIsp == 0 && ip.ulPro == 0)
        {
            ulIP = inet_addr(ip.szIP);
            ulIP = ulIP << 8;
            if (mapIP.find(ulIP) != mapIP.end())
            {
                IPAttr &ipt = mapIP[ulIP];
                strncpy(ipt.szIP, ip.szIP, sizeof(ipt.szIP));
                vecIPAttr.push_back(ipt);
            }
        }
        else
        {
            strncpy(ip_attr.szIP, ip.szIP, sizeof(ip_attr.szIP));
            ip_attr.uCountry = ip.uCountry;
            ip_attr.uIsp = ip.uIsp;
            ip_attr.uProvince = ip.ulPro;

            vecIPAttr.push_back(ip_attr);
        }
    }
}

bool GetIPFromDB(set<unsigned int> &setIPAttr, CMysql &hLocalMysql)
{
    char szSql[] = "select * from db_idc_speed_2.tbl_ip_attr;";

    try
    {
        hLocalMysql.Query(szSql);
        hLocalMysql.StoreResult();
        char * pField = NULL;
        unsigned int ulIP;
        while (NULL != hLocalMysql.FetchRow())
        {
            pField = hLocalMysql.GetField("ip");
            if (pField)
            {
                ulIP = inet_addr(pField);
                ulIP = ulIP << 8;

                setIPAttr.insert(ulIP);
            }
        }
    } catch (CCommonException & ex)
    {
        return false;
    }

    return true;
}

bool GetIPAttrFromDB(map<unsigned int, IPAttr> &mapIPAttr, CMysql &hLocalMysql)
{
    char szSql[] = "select * from db_idc_speed_2.tbl_ip_attr;";

    try
    {
        hLocalMysql.Query(szSql);
        hLocalMysql.StoreResult();

        IPAttr ip;
        unsigned int ulIP = 0;
        char * pField = NULL;
        while (NULL != hLocalMysql.FetchRow())
        {
            pField = hLocalMysql.GetField("ip");
            if (!pField)
                continue;

            strncpy(ip.szIP, pField, sizeof(ip.szIP));

            pField = hLocalMysql.GetField("country");
            ip.uCountry = safe_atou(pField);
            pField = hLocalMysql.GetField("isp");
            ip.uIsp = safe_atou(pField);
            pField = hLocalMysql.GetField("prov");
            ip.uProvince = safe_atou(pField);

            ulIP = inet_addr(ip.szIP);
            ulIP = ulIP << 8;
            mapIPAttr[ulIP] = ip;
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

bool SetIPAttrToDB(vector<IPAttr> &vecIP, map<unsigned int, IPAttr> &mapIPAttr,
        CMysql &hLocalMysql)
{
    //1. get set_del that will generate the del set
    set < string > set_del;
    map<unsigned int, IPAttr>::iterator it;
    for(it = mapIPAttr.begin(); it != mapIPAttr.end(); it++)
        set_del.insert((it->second).szIP);

    unsigned uTS = time(0);
    //2. update or insert into db with flag = 0
    char szTmp[1024];
    unsigned int ulIP, ulNoChg = 0;
    vector<string> vecUpd, vecIns;
    for(size_t i = 0; i < vecIP.size(); i++)
    {
        IPAttr &ip = vecIP[i];

        ulIP = inet_addr(ip.szIP);
        ulIP = ulIP << 8;
        if (mapIPAttr.find(ulIP) != mapIPAttr.end())
        {
            IPAttr &ip2 = mapIPAttr[ulIP];
            if (ip.uCountry != ip2.uCountry || ip.uIsp != ip2.uIsp
                    || ip.uProvince != ip2.uProvince)
            {
                snprintf(
                        szTmp,
                        sizeof(szTmp) - 1,
                        "update db_idc_speed_2.tbl_ip_attr set opt_flag=0, country=%u, isp=%u, prov=%u, mod_ts=%u where ip='%s';",
                        ip.uCountry, ip.uIsp, ip.uProvince, uTS, ip.szIP);
                vecUpd.push_back(string(szTmp));
            }
            else
            {
                ulNoChg++;
            }

            set_del.erase(ip.szIP);
        }
        else
        {
            snprintf(
                    szTmp,
                    sizeof(szTmp) - 1,
                    "insert into db_idc_speed_2.tbl_ip_attr values('%s', '%u', '%u', '%u',0,'%u');",
                    ip.szIP, ip.uCountry, ip.uIsp, ip.uProvince, uTS);
            vecIns.push_back(string(szTmp));
        }
    }

    if (!ExcSQL(vecUpd, hLocalMysql) || !ExcSQL(vecIns, hLocalMysql))
    {
        cout << "ExcSQL for update or insert db_idc_speed_2.tbl_ip_attr Error"
                << endl;
        return false;
    }

    cout << "db_idc_speed_2.tbl_ip_attr, Update: " << vecUpd.size()
            << ", Insert: " << vecIns.size();

    //3. delete vector
    vector < string > vecDel;
    set<string>::iterator it_set;
    for(it_set = set_del.begin(); it_set != set_del.end(); it_set++)
    {
        snprintf(
                szTmp,
                sizeof(szTmp) - 1,
                "update db_idc_speed_2.tbl_ip_attr set opt_flag=1, mod_ts=%u where ip='%s';",
                uTS, (*it_set).c_str());
        vecDel.push_back(string(szTmp));
    }

    if (!ExcSQL(vecDel, hLocalMysql))
    {
        cout << "ExcSQL for deletet db_idc_speed_2.tbl_ip_attr Error." << endl;
        return false;
    }

    cout << ", Delete: " << vecDel.size() << ", NoChange: " << ulNoChg << endl;

    //4. 修改一次最大时间戳，以便agent进行同步的时候，最多重复拉取一条记录即可
    if (vecIP.size() > 0)
    {
        vecUpd.clear();
        snprintf(
                szTmp,
                sizeof(szTmp) - 1,
                "update db_idc_speed_2.tbl_ip_attr set mod_ts=%u where ip='%s';",
                uTS + 1, vecIP[0].szIP);

        vecUpd.push_back(szTmp);
        ExcSQL(vecUpd, hLocalMysql);
    }

    return true;
}

int main(int argc, char *argv[])
{
    cout << GetCurDate() << endl;

    CMysql hMysql(G_DB_HOST, G_DB_USER, G_DB_PASS);

    //0.load country info, country_isp conf
    map<string, SCountryInfo> mapSN2Cty;
    map<unsigned, SCountryInfo> mapID2Cty;
    LoadCountryInfo(mapSN2Cty, mapID2Cty, COUNTRY_INFO_FILE);

    map < string, vector<string> > mapCtyIsp;
    LoadCountryIspConf(COUNTRY_ISP_CONF_FILE, mapCtyIsp);

    //0.5 run once
    /*
     if (SetCountryInfo2DB(mapID2Cty, hMysql))
     cout << "set to db ok, size: " << mapID2Cty.size() << endl;
     else
     cout << "set to db fail" << endl;
     return 0;
     */

    //1. Load final_global_ip address
    map<unsigned int, IPAttr> mapIP;
    if (0 != GetIPInfo(FINAL_GLOBAL_IP_FILE, mapIP, mapSN2Cty, mapCtyIsp))
        return -1;

    cout << "Load final_global_ip OK, get ip num: " << mapIP.size() << endl;

    //2. get source ip from tbl_ip_source
    vector<SIPSrc> vecIPSrc;
    if (!GetIPSrcFromDB(vecIPSrc, hMysql))
    {
        cout << "GetIPSrcFromDB Error" << endl;
        return -1;
    }
    else
        cout << "GetIPSrcFromDB OK, size:" << vecIPSrc.size() << endl;

    //3. generate ip attribution
    vector<IPAttr> vecIPAttr;
    GetIPAttrFromSrc(vecIPAttr, vecIPSrc, mapIP);

    cout << "GetIPAttrFromSrc size: " << vecIPAttr.size() << endl;

    //4. get ip from tbl_ip_attr
    map<unsigned int, IPAttr> mapIPAttr;
    if (!GetIPAttrFromDB(mapIPAttr, hMysql))
    {
        cout << "GetIPAttrFromDB Error" << endl;
        return -1;
    }
    cout << "GetIPAttrFromDB tbl_ip_attr, old size: " << mapIPAttr.size()
            << endl;

    //the new size can't less than 3/4
    if (mapIPAttr.size() > vecIPAttr.size() && mapIPAttr.size() * 3 > 4
            * vecIPAttr.size())
    {
        cout << "the new ip_attr size is less than 3/4 old, so don't update!"
                << endl;
        return -1;
    }

    //5. update table tbl_ip_attr
    if (!SetIPAttrToDB(vecIPAttr, mapIPAttr, hMysql))
    {
        cout << "SetIPAttrToDB Error" << endl;
        return -1;
    }

    return 0;
}
