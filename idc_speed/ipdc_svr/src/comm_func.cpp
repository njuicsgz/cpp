#include "comm_func.h"
#include <time.h>

//------------------------------------基本函数------------------------//
string itoStr(int i)
{
    char buf[32] = {0};
    snprintf(buf, sizeof(buf) - 1, "%d", i);
    return buf;
}

string utos(unsigned int u)
{
    char buf[32] = {0};
    snprintf(buf, sizeof(buf) - 1, "%u", u);
    return buf;
}

int safe_atoi(const char *pszStr)
{
    if (NULL == pszStr)
        return 0;

    return atoi(pszStr);
}

unsigned int safe_atou(const char *s)
{
    if (NULL == s)
        return 0;

    unsigned int i = 0;
    while (isdigit(*s))
        i = i * 10 + (*s++) - '0';

    return i;
}

string safe_ptos(const char *p)
{
    return NULL == p ? "" : p;
}

bool DigitStr(const string &s)
{
    for(size_t i = 0; i < s.size(); i++)
    {
        if (!isdigit(s.at(i)))
            return false;
    }

    return true;
}

string Set2Str(const set<string> &Set)
{
    string s("");
    set<string>::const_iterator it;
    for(it = Set.begin(); it != Set.end(); it++)
    {
        s += *it + ";";
    }
    return s;
}

string Vec2Str(const vector<string> &vec)
{
    string s("");
    vector<string>::const_iterator it;
    for(it = vec.begin(); it != vec.end(); it++)
    {
        s += *it + ";";
    }
    return s;
}

//将一个由分号间隔的字符串转换成单独的字符串
void splitSemicolon(const string &strSrc, vector<string> &vec_strDes,
        const char *delim)
{
    char buf[2048];
    char *p;
    string tmp;

    snprintf(buf, sizeof(buf) - 1, "%s", strSrc.c_str());
    p = strtok(buf, delim);
    if (p)
    {
        tmp = p;
        vec_strDes.push_back(tmp);
    }
    while (p)
    {
        p = strtok(NULL, delim);
        if (p)
        {
            tmp = p;
            vec_strDes.push_back(tmp);
        }
    }
}

//将一个由分号间隔的字符串转换成单独的字符串
void splitSemicolon(const string &strSrc, set<string> &set_strDes,
        const char *delim)
{
    char buf[2048];
    char *p;
    string tmp;

    snprintf(buf, sizeof(buf) - 1, "%s", strSrc.c_str());
    p = strtok(buf, delim);
    if (p)
    {
        tmp = p;
        set_strDes.insert(tmp);
    }
    while (p)
    {
        p = strtok(NULL, delim);
        if (p)
        {
            tmp = p;
            set_strDes.insert(tmp);
        }
    }
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

bool getIPUL(unsigned long &ulIP, const string &sIP)
{
    ulIP = 0;
    struct in_addr addr = {0};
    if (0 == inet_aton(sIP.c_str(), &addr))
        return false;

    ulIP = addr.s_addr;
    return true;
}

bool IsValidIpv6Address(string &sUserip)
{
    set < string > setIPList;
    splitSemicolon(sUserip, setIPList, ";");

    set<string>::iterator it;
    for(it = setIPList.begin(); it != setIPList.end(); it++)
    {
        struct sockaddr addr;
        if (inet_pton(AF_INET6, (*it).c_str(), (void *) &addr) <= 0)
            return false;
    }
    return true;
}

bool IsValidIpv4Address(string &sUserip)
{
    set < string > setIPList;
    splitSemicolon(sUserip, setIPList, ";");

    set<string>::iterator it;
    for(it = setIPList.begin(); it != setIPList.end(); it++)
    {
        struct sockaddr addr;
        if (inet_pton(AF_INET, (*it).c_str(), (void *) &addr) <= 0)
            return false;
    }
    return true;
}

char *ConvertEnc(char *pszEncFrom, char *pszEncTo, const char * pszIn,
        char *pszOut, int nOutLen)
{
    int len = strlen(pszIn);
    char *sin, *sout;

    int lenin, lenout, ret;

    lenin = len + 1;
    lenout = 2 * len + 1;

    if (lenout > nOutLen)
    {
        return NULL;
    }

    iconv_t c_pt;
    if ((c_pt = iconv_open(pszEncTo, pszEncFrom)) == (iconv_t) - 1)
    {
        //printf("iconv_open false: %s ==> %s\n", encFrom, encTo);
        return NULL;
    }

    iconv(c_pt, NULL, NULL, NULL, NULL);
    memset(pszOut, 0, lenout);

    sin = (char *) pszIn;
    sout = pszOut;

    ret = iconv(c_pt, &sin, (size_t *) &lenin, &sout, (size_t *) &lenout);
    if (ret == -1)
    {
        iconv_close(c_pt);
        return NULL;
    }

    iconv_close(c_pt);

    return pszOut;
}

string UTF8toGBK(const string& strIn)
{
    static char szUTF8In[4096];
    static char szGBOut[4096];

    memset(szUTF8In, 0x00, sizeof(szUTF8In));
    memset(szGBOut, 0x00, sizeof(szGBOut));

    strncpy(szUTF8In, strIn.c_str(), strIn.length() + 1);

    ConvertEnc("utf-8", "gbk", szUTF8In, szGBOut, sizeof(szGBOut) - 1);

    return szGBOut;
}

string GBKtoUTF8(const string& strIn)
{
    static char szUTF8In[4096];
    static char szGBOut[4096];

    memset(szUTF8In, 0x00, sizeof(szUTF8In));
    memset(szGBOut, 0x00, sizeof(szGBOut));

    strncpy(szUTF8In, strIn.c_str(), strIn.length() + 1);

    ConvertEnc("gbk", "utf-8", szUTF8In, szGBOut, sizeof(szGBOut) - 1);

    return szGBOut;
}

string GetDataTime(time_t time)
{
    struct tm *ptm;
    localtime_r(&time, ptm);

    char szBuf[128] = {0};
    strftime(szBuf, sizeof(szBuf) - 1, "%Y-%m-%d %H:%M:%S", ptm);

    return szBuf;
}
