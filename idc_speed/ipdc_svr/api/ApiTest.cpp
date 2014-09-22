#include <stdlib.h>
#include <stdio.h>
#include "ipdc_api.h"

void Usage(const char *pszProgramName)
{
    printf("%s TestSvr TestPort TestType Arg...\n", pszProgramName);
    printf("TestType List:\n");
    printf("\tTest ulTestData\n");
    printf("\tTask1 strIP ulTestData\n");
    printf("\tGetIPAttrByTS ulOldTS\n");
    printf("\tOnReptTask ulTimestamp\n");
    exit(-1);
}

void ExitPrintf(const char *pszTestType, const char *pszSvr,
        unsigned int ulPort, int iRet)
{
    printf("Test api %s from %s:%u error, retcode = %d\n", pszTestType, pszSvr,
            ulPort, iRet);
    exit(-1);
}

void SuccessExit(const char *pszTestType, const char *pszSvr,
        unsigned int ulPort, int iRet)
{
    printf("Test api %s from %s:%u success, retcode = %d\n", pszTestType,
            pszSvr, ulPort, iRet);
    exit(-1);
}

int main(int argc, char * argv[])
{
    if (argc < 3)
        Usage(argv[0]);

    string strSvr = argv[1];
    unsigned int ulPort = atol(argv[2]);
    string strTestType = argv[3];

    CIPDCApi stApi(strSvr.c_str(), ulPort);
    int iRet;
    if (strTestType == "Test")
    {
        if (argc < 4)
            Usage(argv[0]);

        unsigned int ulTestData = atol(argv[4]);
        unsigned int ulTestResultData;

        if ((iRet = stApi.Test(ulTestData, ulTestResultData)) != 0)
            ExitPrintf(strTestType.c_str(), strSvr.c_str(), ulPort, iRet);

        printf("Return data: %u\n", ulTestResultData);
        SuccessExit(strTestType.c_str(), strSvr.c_str(), ulPort, iRet);
    }
    else if (strTestType == "Task1")
    {
        if (argc < 5)
            Usage(argv[0]);

        string strIP = argv[4];
        unsigned int ulTestData = atol(argv[5]);

        if ((iRet = stApi.OnTask1(strIP, ulTestData)) != 0)
            ExitPrintf(strTestType.c_str(), strSvr.c_str(), ulPort, iRet);

        SuccessExit(strTestType.c_str(), strSvr.c_str(), ulPort, iRet);
    }
    else if (strTestType == "GetIPAttrByTS")
    {
        if (argc < 4)
            Usage(argv[0]);

        vector<SIPAttr> vecMod;
        vector<unsigned> vecDel;
        unsigned uTSNew;
        if ((iRet = stApi.GetIPAttrByTS(vecMod, vecDel, uTSNew,
                (unsigned) atoi(argv[4]))) != 0)
            ExitPrintf(strTestType.c_str(), strSvr.c_str(), ulPort, iRet);

        printf("vecMod size: %u, vecDel size: %u, uTSNew: %u\n", vecMod.size(),
                vecDel.size(), uTSNew);

        SuccessExit(strTestType.c_str(), strSvr.c_str(), ulPort, iRet);
    }
    else if (strTestType == "OnReptTask")
    {
        vector<SReptTask> vecRT;
        if ((iRet = stApi.OnReptTask(vecRT, 0)) != 0)
            ExitPrintf(strTestType.c_str(), strSvr.c_str(), ulPort, iRet);

        SuccessExit(strTestType.c_str(), strSvr.c_str(), ulPort, iRet);
    }
    else
    {
        printf("commod is not valid.\n");
    }

    return 0;
}
