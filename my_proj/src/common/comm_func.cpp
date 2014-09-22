#include "comm_func.h"

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

string StrUpper(const string &str)
{
    string s1 = str;
    transform(s1.begin(), s1.end(), s1.begin(), ::toupper);
    return s1;
}

string StrLower(string &str)
{
    string s1 = str;
    transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
    return s1;
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
bool Split2Vec(const string &s, vector<string> &vec, const char *delim)
{
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

//将一个由分号间隔的字符串转换成单独的字符串
bool Split2Set(const string &s, set<string> &set1, const char *delim)
{
    size_t size = s.size() + 2;
    char *buf = NULL;
    if (!(buf = new (std::nothrow) char[size]))
        return false;

    snprintf(buf, size - 1, "%s", s.c_str());
    char *p = strtok(buf, delim);
    while (p)
    {
        set1.insert(string(p));
        p = strtok(NULL, delim);
    }

    return true;
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
    set<string> setIPList;
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
    set<string> setIPList;
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
    struct tm tm2;
    localtime_r(&time, &tm2);

    char szBuf[128] = {0};
    strftime(szBuf, sizeof(szBuf) - 1, "%Y-%m-%d %H:%M:%S", &tm2);

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
}

void my_set_intersection(set<string> &set_out, set<string> &set1,
        set<string> &set2)
{
    set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
            std::inserter(set_out, set_out.begin()));

    /**
     * set_difference/set_union/set_symmetric_difference
     */

}

/*
 * 若中间目录不存在，则创建
 * output[0:success -1:error]
 * int access( const  char *path,  int mode )
 * int mkdir (const char *filename, mode_t mode)
 */
void mkdirs(const char *dir)
{
    char tmp[1024];
    char *p;
    if (strlen(dir) == 0 || dir == NULL)
    {
        printf("strlen(dir) is 0 or dir is NULL.");
        return;
    }
    memset(tmp, 0, sizeof(tmp));
    strncpy(tmp, dir, strlen(dir));

    //以绝对路径创建目录树
    p = strchr(tmp + 1, '/');
    int statMd = 0;

    if (p)
    {
        *p = '\0';
        statMd = mkdir(tmp, 777);
        chdir(tmp);
    }
    else
    {
        statMd = mkdir(tmp, 777);
        chdir(tmp);
        return;
    }
    mkdirs(p + 1);
}

//循环扫描当前目录的每一个文件，删除其中超出天数的文件
void rm_timeout(const string &strAbsDirPath, const unsigned uTimeOutSec)
{
    if (0 == strAbsDirPath.length())
        return;

    DIR *pDir;
    struct dirent *ent;
    struct stat fs;

    time_t now = time(0);

    pDir = opendir(strAbsDirPath.c_str());
    if (NULL == pDir)
    {
        printf("error:can not open dir %s!", strAbsDirPath.c_str());
        return;
    }

    chdir(strAbsDirPath.c_str());

    while ((ent = readdir(pDir)) != NULL)
    {
        if (ent->d_type == DT_REG && 0 == stat(ent->d_name, &fs))
        {
            if (now - fs.st_mtime > uTimeOutSec)
                remove(ent->d_name);
        }
    }

    closedir(pDir);
    pDir = NULL;
    ent = NULL;
}

bool GetDstIPv4(unsigned &uIP, const char *host)
{
    char dns_buf[1024];
    struct hostent hostinfo, *phost;
    int rc;

    if (0 == gethostbyname_r(host, &hostinfo, dns_buf, 1024, &phost, &rc)
            && phost != NULL)
    {
        uIP = *(unsigned *) (hostinfo.h_addr);
        return true;
    }

    return false;
}
