#include "CSessionMgr.h"

CMutex g_mutexSession;
map<unsigned , string > g_mapFlow2IP;
map<string, unsigned > g_mapIP2Flow;

void MapIP2Flow( unsigned int ulFlow)
{
	ml lock(g_mutexSession);

	map<unsigned , string >::iterator it = g_mapFlow2IP.find( ulFlow );
	if( it != g_mapFlow2IP.end() )
	{
		g_mapIP2Flow[ it->second ] = ulFlow;
	}
}

void GetAllSession( vector<string> &vecSession )
{
	ml lock(g_mutexSession);
	map<string, unsigned >::iterator it =  g_mapIP2Flow.begin();
	for( ; it != g_mapIP2Flow.end() ; ++it )
	{
		vecSession.push_back( it->first );
	}
}

bool GetFlowByIP( const string &strIP , unsigned int &ulFlow )
{
	ml lock(g_mutexSession);
	
	map<string, unsigned >::iterator it = g_mapIP2Flow.find( strIP );
	if( it == g_mapIP2Flow.end() )
		return false;

	ulFlow = it->second;

	return true;
}

void MapFlow2IP( const string &strIP, unsigned int ulFlow )
{
	ml lock(g_mutexSession);
	g_mapFlow2IP[ ulFlow ] = strIP;
}

void CloseSession( unsigned int ulFlow )
{
	ml lock(g_mutexSession);
	
	map<unsigned , string >::iterator it = g_mapFlow2IP.find( ulFlow );
	assert(it != g_mapFlow2IP.end());

	string strIP = it->second;
	map<string, unsigned >::iterator it_ip = g_mapIP2Flow.find( strIP );
	if( it_ip != g_mapIP2Flow.end() )
	{
		g_mapIP2Flow.erase( it_ip );
	}

	g_mapFlow2IP.erase( it );
}
