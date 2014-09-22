#include <stdlib.h>
#include <stdio.h>
#include "Api.h"

void Usage(const char *pszProgramName)
{
	printf("%s TestSvr TestPort TestType Arg...\n", pszProgramName);
	printf("TestType List:\n");
	printf("\tTest ulTestData\n");
	printf("\tTask1 strIP ulTestData\n");
	printf("\tOnGetAllClient\n");
	exit(-1);
}

void ExitPrintf(const char *pszTestType, const char *pszSvr, unsigned int ulPort, int iRet)
{
	printf("Test api %s from %s:%u error, retcode = %d\n", pszTestType, pszSvr, ulPort, iRet);
	exit( -1 );
}

void SuccessExit(const char *pszTestType, const char *pszSvr, unsigned int ulPort, int iRet)
{
	printf("Test api %s from %s:%u success, retcode = %d\n", pszTestType, pszSvr, ulPort, iRet);
	exit( -1 );
}

int main(int argc , char * argv[] )
{
	if( argc < 3 )
		Usage(argv[0]);

	string strSvr = argv[1];
	unsigned int ulPort = atol( argv[2]) ;
	string strTestType = argv[3];
	
	CApi stApi( strSvr.c_str(), ulPort );
	int iRet;
	if( strTestType == "Test" )
	{
		if( argc < 4 )
			Usage(argv[0]);

		unsigned int ulTestData = atol( argv[4] );
		unsigned int ulTestResultData;
		
		if( ( iRet = stApi.Test(ulTestData, ulTestResultData) ) != 0  )
			ExitPrintf(strTestType.c_str(), strSvr.c_str(), ulPort,iRet);


		printf("Return data: %u\n", ulTestResultData);
		SuccessExit(strTestType.c_str(), strSvr.c_str(), ulPort,iRet);
	}
	else if( strTestType == "Task1" )
	{
		if( argc < 5 )
            Usage(argv[0]);

		string strIP = argv[4];
        unsigned int ulTestData = atol( argv[5] );

        if( ( iRet = stApi.OnTask1(strIP, ulTestData) ) != 0  )
            ExitPrintf(strTestType.c_str(), strSvr.c_str(), ulPort,iRet);

        SuccessExit(strTestType.c_str(), strSvr.c_str(), ulPort,iRet);
	}
	else if( strTestType == "OnGetAllClient" )
	{
		vector<string> vecClient;
		if( ( iRet = stApi.OnGetAllClient(vecClient) ) != 0  )
            ExitPrintf(strTestType.c_str(), strSvr.c_str(), ulPort,iRet);

		for( unsigned int i = 0 ; i< vecClient.size() ; i++ )
		{
			printf("%s\n", vecClient[i].c_str());
		}
        SuccessExit(strTestType.c_str(), strSvr.c_str(), ulPort,iRet);
	}
	
	return 0;
}
