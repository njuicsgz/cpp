#ifndef GSLB_COMM_H_20120224
#define GSLB_COMM_H_20120224

#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

string itoStr(int i);
string utos(unsigned int u);
string safe_ptos(const char *p);
int safe_atoi(const char *pszStr);
unsigned int safe_atou(const char *pszStr);

bool DigitStr(const string &s);

string Set2Str(const set<string> &Set);
string Vec2Str(const vector<string> &vec);

void splitSemicolon(const string &strSrc, vector<string> &vec_strDes,
        const char *delim);
void splitSemicolon(const string &strSrc, set<string> &set_strDes,
        const char *delim);

string getIPStr(unsigned long nIP);
bool getIPUL(unsigned long &ulIP, const string &sIP);

bool IsValidIpv6Address(string &sUserip);
bool IsValidIpv4Address(string &sUserip);

string UTF8toGBK(const string& strIn);
string GBKtoUTF8(const string& strIn);

string GetDataTime(time_t time);

#endif

