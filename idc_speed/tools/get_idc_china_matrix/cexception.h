#ifndef  CEXCEPTION_H_ABt34523
#define  CEXCEPTION_H_ABt34523

#include <string>

using namespace std;

class CCommonException
{
public:
	CCommonException(string sErrMsg) {m_str= sErrMsg;};
	CCommonException(string sErrMsg, string sUrl) {m_str=sErrMsg; m_url=sUrl;};

	const char* GetErrMsg() {return m_str.c_str();};
	const char* GetUrl() {return m_url.c_str();};
public:
	string m_str;
	string m_url;
};

#endif /* CEXCEPTION_H_ABt34523 */


