#include "Conf.h"

struct TestConf
{
    //sever config
    char szItilIP[64];
    unsigned int uiItilPort;

    //log config
    char szLogFileName[128];
    unsigned int uiLogLevel;
    unsigned int uiLogFileSize;
    unsigned int uiLogFileCount;

    //mysql handler
    char szDBHost[64];
    char szDBUsr[64];
    char szDBPwd[64];

    TestConf()
    {
        memset(this, 0x00, sizeof(TestConf));
    }
};

TestConf g_TestConf;

inline void GetString(char *pszDst, const size_t uiLen, const string &sLabl)
{
    char *pszTemp = GetConf(sLabl.c_str());
    if (pszTemp != NULL)
        strncpy(pszDst, pszTemp, uiLen);
}

inline void GetUInt(unsigned int &nDst, const string &sLabl)
{
    char *pszTemp = GetConf(sLabl.c_str());
    if (pszTemp != NULL && atoi(pszTemp))
        nDst = atoi(pszTemp);
}

bool InitConf(const char *pszFileName)
{
    if (0 != LoadConf(pszFileName) )
        return false;

    GetString(g_TestConf.szItilIP, sizeof(g_TestConf.szItilIP) - 1, "szItilIP");
    GetString(g_TestConf.szDBHost, sizeof(g_TestConf.szDBHost) - 1, "szDBHost");
    GetString(g_TestConf.szDBUsr, sizeof(g_TestConf.szDBUsr) - 1, "szDBUsr");
    GetString(g_TestConf.szDBPwd, sizeof(g_TestConf.szDBPwd) - 1, "szDBPwd");
    GetString(g_TestConf.szLogFileName, sizeof(g_TestConf.szLogFileName) - 1,
            "szLogFileName");

    GetUInt(g_TestConf.uiItilPort, "uiItilPort");
    GetUInt(g_TestConf.uiLogFileSize, "uiLogFileSize");
    GetUInt(g_TestConf.uiLogFileCount, "uiLogFileCount");
    GetUInt(g_TestConf.uiLogLevel, "uiLogLevel");

    return true;
}
