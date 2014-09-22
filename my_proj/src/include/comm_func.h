#ifndef GSLB_COMM_H_20120224
#define GSLB_COMM_H_20120224

#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

using namespace std;

string itoStr(int i);
string utos(unsigned int u);
string safe_ptos(const char *p);
int safe_atoi(const char *pszStr);
unsigned int safe_atou(const char *pszStr);

bool DigitStr(const string &s);

string StrUpper(const string &str);
string StrLower(const string &str);

string Set2Str(const set<string> &Set);
string Vec2Str(const vector<string> &vec);

bool Split2Vec(const string &s, vector<string> &vec, const char *delim);
bool Split2Set(const string &s, set<string> &set1, const char *delim);

string getIPStr(unsigned long nIP);
bool getIPUL(unsigned long &ulIP, const string &sIP);

bool IsValidIpv6Address(string &sUserip);
bool IsValidIpv4Address(string &sUserip);

string UTF8toGBK(const string& strIn);
string GBKtoUTF8(const string& strIn);

string GetDataTime(time_t time);

void LoadCountryInfo(map<string, SCountryInfo> &mapSN2Cty, map<unsigned,
        SCountryInfo> &mapID2Cty, const char *pfile);

void my_set_intersection(set<string> &set_out, set<string> &set1,
        set<string> &set2);

void mkdirs(const char *dir);
void rm_timeout(const string &strAbsDirPath, const unsigned uTimeOutSec);

bool GetDstIPv4(unsigned &uIP, const char *host);

#endif

