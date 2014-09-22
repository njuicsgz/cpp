#ifndef _C_SESSION_MGR_H_
#define _C_SESSION_MGR_H_

#include <map>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "guardian.h"

using namespace std;

extern CMutex g_mutexSession;
extern map<unsigned , string > g_mapFlow2IP;
extern map<string, unsigned > g_mapIP2Flow;

void MapIP2Flow( unsigned int ulFlow );

void GetAllSession( vector<string> &vecSession );

bool GetFlowByIP( const string &strIP , unsigned int &ulFlow );

bool GetIPByFlow(string &strIP, const unsigned uFlow);

void MapFlow2IP( const string &strIP, unsigned int ulFlow );

void CloseSession( unsigned int ulFlow );


#endif

