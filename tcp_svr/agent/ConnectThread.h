#ifndef _CONNECT_THREAD_H_
#define _CONNECT_THREAD_H_

#include <map>
#include <vector>
#include <list>
#include <set>
#include "thread_runner.h"
#include <netinet/in.h>
#include <netdb.h>
using namespace std;

class Commpack;
class CConnectThread: public CRunner
{
public:
    CConnectThread();
    virtual ~CConnectThread();
    virtual void Run();

    void OnTask1(Commpack *pPack);

protected:
    int Recv(int iFD, Commpack *&pack, unsigned int ulTimeOut);

    int SelectSingleRead(int fd, float timeout);
    int SelectSingleRead(int fd, float timeout, fd_set* r_set);
    void OnCmdCome(Commpack *pLongPackage);
};

#endif 
