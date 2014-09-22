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
const char COUNTRY_CONF[] = "../data/country.conf";
const char OUTPUT_RESULT[] = "./result.xls";
const char COUNTRY_ISP_CONF_FILE[] = "../data/country_isp.conf";

const char COUNTRY_INFO_FILE[] = "../data/country_iso_3166_1.txt";

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

string GetCurDate()
{
    time_t now = time(NULL);
    struct tm ptm;
    char szBuf[64] = {0};

    localtime_r(&now, &ptm);
    strftime(szBuf, sizeof(szBuf) - 1, "%Y-%m-%d %H:%M:%S", &ptm);

    return szBuf;
}

string StrLower(const string &str)
{
    string s1 = str;
    transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
    return s1;
}

string GetIspNotCN(const string &sCountry, const size_t index, map<string,
        vector<string> > &mapCtyIsp)
{
    string s = StrLower(sCountry);

    if (mapCtyIsp.find(s) == mapCtyIsp.end())
        return "NULL";

    vector < string > &vec = mapCtyIsp[s];
    if (index >= vec.size())
        return "NULL";
    else
        return vec[index];
}

void LoadConf(const char *pfile, vector<string> &vec)
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
        if (szBuf[strlen(szBuf) - 1] == '\n')
            szBuf[strlen(szBuf) - 1] = '\0';
        if (szBuf[strlen(szBuf) - 1] == '\r')
            szBuf[strlen(szBuf) - 1] = '\0';

        vec.push_back(szBuf);
    }

    fin.close();

    cout << "Load country conf size: " << vec.size() << endl;
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

void LoadCountryInfo(map<string, SCountryInfo> &mapCName2Cty, map<unsigned,
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
            mapCName2Cty[string(ci.szChnName)] = ci;
            mapID2Cty[ci.uID] = ci;
        }
    }

    fin.close();

    cout << "Load country size: " << mapCName2Cty.size() << endl;
}

void GetCtyID(map<unsigned, string> &mapCty, const vector<string> &vecCty, map<
        string, SCountryInfo> &mapCName2Cty)
{
    for(size_t i = 0; i < vecCty.size(); i++)
    {
        const string &s = vecCty[i];
        if (mapCName2Cty.find(s) != mapCName2Cty.end())
        {
            mapCty[mapCName2Cty[s].uID] = s;
        }
    }

    cout << "GetCtyID, country size: " << vecCty.size() << ", get id size: "
            << mapCty.size() << endl;
}

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
        CMysql &hMysql, map<unsigned, string> &mapCty,
        map<string, string> &mapIP2Idc, const string &sDateBegin,
        const string &sDateEnd)
{
    char szSql[1024] = {0};

    char szTmp[8] = {0};

    SIPAttrShort ias;
    int ip_num = 0;

    map<unsigned, string>::iterator it;
    map<string, string>::iterator it2;
    for(it = mapCty.begin(); it != mapCty.end(); it++)
    {
        ias.uCountry = it->first;

        for(it2 = mapIP2Idc.begin(); it2 != mapIP2Idc.end(); it2++)
        {
            const string &sIP = it2->first;

            snprintf(szSql, sizeof(szSql) - 1, "select * "
                "from db_idc_speed_2.tbl_delay_5min "
                "where day>='%s' and day<='%s' and ip='%s' and country=%u;",
                    sDateBegin.c_str(), sDateEnd.c_str(), sIP.c_str(),
                    ias.uCountry);
            try
            {
                hMysql.FreeResult();
                hMysql.Query(szSql);
                hMysql.StoreResult();

                char * pField = NULL;
                while (NULL != hMysql.FetchRow())
                {
                    pField = hMysql.GetField("isp");
                    ias.uIsp = safe_atou(pField);

                    SDelaySum &ds = mapMatrix[ias][sIP];

                    pField = hMysql.GetField("test_ip_num");
                    ip_num = safe_atou(pField);

                    for(int k = 1; k <= 288; k++)
                    {
                        snprintf(szTmp, sizeof(szTmp) - 1, "d%d", k);
                        pField = hMysql.GetField(szTmp);
                        if (safe_atou(pField) > 0)
                        {
                            ds.ullDelayTotal += atoi(pField) * ip_num;
                            ds.uCnt += ip_num;
                        }
                    }
                }

            } catch (exception ex)
            {
                cout << "error reason:" << ex.what() << endl;
                return false;
            }
        }
    }

    return true;
}

void OutPut(const char * pfile,
        map<SIPAttrShort, map<string, SDelaySum> > &mapMatrix, map<unsigned,
                string> &mapCty, map<string, string> &mapIP2Idc, map<unsigned,
                SCountryInfo> &mapID2Cty,
        map<string, vector<string> > &mapCtyIsp)
{
    ofstream fout(pfile, ifstream::out);
    if (!fout.good())
    {
        cout << "open file error: " << pfile << endl;
        return;
    }

    map<SIPAttrShort, map<string, SDelaySum> >::iterator it;

    //1. print first row: title
    fout << "Country\tISP\t";

    map<string, string>::iterator it_idc;
    for(it_idc = mapIP2Idc.begin(); it_idc != mapIP2Idc.end(); it_idc++)
    {
        fout << it_idc->second << "(" << it_idc->first << ")\t";
    }

    fout << endl;

    //2. print data
    for(it = mapMatrix.begin(); it != mapMatrix.end(); it++)
    {
        const SIPAttrShort &ias = it->first;
        map<string, SDelaySum> &mapIP2Delay = it->second;

        fout << mapCty[ias.uCountry] << "\t" << GetIspNotCN(
                (string) mapID2Cty[ias.uCountry].szShortName2, ias.uIsp,
                mapCtyIsp) << "\t";

        //display by idc_ip.conf, because some idc my not has ping_ok data
        map<string, SDelaySum>::iterator it2;
        for(it_idc = mapIP2Idc.begin(); it_idc != mapIP2Idc.end(); it_idc++)
        {
            if (mapIP2Delay.find(it_idc->first) != mapIP2Delay.end())
            {
                SDelaySum &ds = mapIP2Delay[it_idc->first];
                if (ds.uCnt)
                    ds.ullDelayAvg = ds.ullDelayTotal / ds.uCnt;

                fout << ds.ullDelayAvg / 1000 << "\t";
            }
            else
            {
                fout << "0\t";
            }
        }

        fout << endl;
    }

    fout.close();
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
        if (szBuff[strlen(szBuff) - 1] == '\n')
            szBuff[strlen(szBuff) - 1] = '\0';

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

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " sBeginDate sEndDate" << endl;
        return -1;
    }

    string sDateBegin = argv[1];
    string sDateEnd = argv[2];

    CMysql hMysql("localhost", "root", "");

    cout << "\n" << GetCurDate() << endl;

    //0.load country info
    map<string, SCountryInfo> mapCName2Cty;
    map<unsigned, SCountryInfo> mapID2Cty;
    LoadCountryInfo(mapCName2Cty, mapID2Cty, COUNTRY_INFO_FILE);

    //1. Load country, idc ip config
    vector < string > vecCountry;
    map < string, string > mapIP2Idc;

    LoadConf(COUNTRY_CONF, vecCountry);
    LoadConfIdcIP(IDC_IP_CONF, mapIP2Idc);

    map < string, vector<string> > mapCtyIsp;
    LoadCountryIspConf(COUNTRY_ISP_CONF_FILE, mapCtyIsp);

    //2. get country id
    map<unsigned, string> mapCty;
    GetCtyID(mapCty, vecCountry, mapCName2Cty);

    //3. Loop for every array
    map<SIPAttrShort, map<string, SDelaySum> > mapMatrix;
    if (!GetDelayMatrix(mapMatrix, hMysql, mapCty, mapIP2Idc, sDateBegin,
            sDateEnd))
    {
        cout << "GetDelayMatrix error, exit." << endl;
        return -1;
    }

    //4. output
    OutPut(OUTPUT_RESULT, mapMatrix, mapCty, mapIP2Idc, mapID2Cty, mapCtyIsp);

    return 0;
}
