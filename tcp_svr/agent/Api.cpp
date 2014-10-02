#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <assert.h>
#include "stdarg.h"
#include "Api.h"

enum cmd
{
    CMD_TEST = 0x0001,
    CMD_LOGIN = 0x1000,
    CMD_HEARTBEAT,
    CMD_GETALLCLIENT = 0x1100,
    CMD_TASK1 = 0x2000,
};

enum error_info
{
    error_connet = -100000,
    error_send,
    error_recv,
    error_protocol,
    error_data,
    error_nodata,
};

CApi::CApi(const string &sReportSvrIP, unsigned wPort) :
        m_fdConnect(-1), m_bConnect(false), m_wPort(wPort), m_fRecvTimeout(
                60.0), m_fSendTimeout(60.0)
{
    memset(m_szServerIP, 0x00, sizeof(m_szServerIP));
    strncpy(m_szServerIP, sReportSvrIP.c_str(), sizeof(m_szServerIP) - 1);
}
CApi::~CApi()
{
    Close();
}

int CApi::SelectSingleRead(int fd, float timeout, fd_set* r_set)
{
    if (fd >= 1024)
        return -1;

    //	fd_set prepare
    FD_SET(fd, r_set);

    //	time prepare
    struct timeval tv;
    tv.tv_sec = (int) timeout;
    tv.tv_usec = (int) ((timeout - (int) timeout) * 1000000);

    //	select
    return select(fd + 1, r_set, NULL, NULL, &tv);
}

int CApi::SelectSingleRead(int fd, float timeout)
{
    fd_set r_set;
    FD_ZERO(&r_set);
    return SelectSingleRead(fd, timeout, &r_set);
}

int CApi::SelectSingleWrite(int fd, float timeout, fd_set* r_set)
{
    if (fd >= 1024)
        return -1;

    FD_SET(fd, r_set);

    struct timeval tv;
    tv.tv_sec = (int) timeout;
    tv.tv_usec = (int) ((timeout - (int) timeout) * 1000000);

    return select(fd + 1, NULL, r_set, NULL, &tv);
}

int CApi::SelectSingleWrite(int fd, float timeout)
{
    fd_set r_set;
    FD_ZERO(&r_set);
    return SelectSingleWrite(fd, timeout, &r_set);
}

int CApi::SetNonBlock(int fd)
{
    int val = fcntl(fd, F_GETFL, 0);
    if (val == -1)
    {
        return -1;
    }

    if (fcntl(fd, F_SETFL, val | O_NONBLOCK | O_NDELAY) == -1)
    {
        return -1;
    }
    return 0;
}
unsigned short CApi::GetSeq()
{
    static int nSeq = 0;
    ++nSeq;
    return nSeq % 0xffff;
}

int CApi::AsyncConnect(unsigned int ip, unsigned short port)
{
    fd_set readfd;
    fd_set writefd;

    struct sockaddr serveraddr;
    struct sockaddr_in *p = (struct sockaddr_in *) &serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));

    p->sin_family = AF_INET;
    unsigned * ptr = (unsigned*) &p->sin_addr;
    *ptr = ip;
    p->sin_port = htons(port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    //	set non-block
    SetNonBlock(fd);

    //	connect
    int ret = ::connect(fd, &serveraddr, sizeof(serveraddr));
    if (ret < 0)
    {
        if (errno != EINPROGRESS)
        {
            close(fd);
            return -1;
        }
    }
    if (ret == 0)
    {
        return fd;
    }

    FD_ZERO(&readfd);
    FD_ZERO(&writefd);
    FD_SET(fd, &readfd);
    FD_SET(fd, &writefd);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 200000;

    ret = select(fd + 1, &readfd, &writefd, NULL, &tv);
    if (ret > 0)
    {
        if (FD_ISSET(fd, &readfd) || FD_ISSET(fd, &writefd))
        {
            int error;
            int len = sizeof(error);
            int bok = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error,
                    (socklen_t*) &len);
            if (bok < 0)
            {
                close(fd);
                return -1;
            } else if (error)
            {
                close(fd);
                return -1;
            }

            return fd;
        }
    }
    close(fd);
    return -1;
}

unsigned int CApi::GetServerIP(const char * szServerName)
{

    unsigned int nIP = INADDR_NONE;
    char pszAddress[256] = { 0 };
    strncpy(pszAddress, szServerName, sizeof(pszAddress) - 1);

    bool bIPAddress = true;

    int nLength = strlen(pszAddress);
    for (int i = 0; i < nLength; i++)
    {
        if ((pszAddress[i] > '9' || pszAddress[i] < '0')
                && pszAddress[i] != '.')
        {
            bIPAddress = false;
            break;
        }
    }

    if (bIPAddress)
    {
        nIP = inet_addr(pszAddress);
    } else
    {
        struct hostent *hs;
        hs = (struct hostent *) ::gethostbyname(pszAddress);
        if (hs)
        {
            memcpy(&nIP, hs->h_addr_list[0], sizeof(u_long));
        }
    }

    if (nIP == INADDR_NONE)
    {

    }
    return nIP;
}

bool CApi::Open(const char *pstrIP, unsigned short nPort, float fRecvTimeout,
        float fSendTimeout)
{
    if (m_bConnect)
    {
        return m_bConnect;
    }
    if (pstrIP != NULL && nPort > 0)
    {
        memset(m_szServerIP, 0x00, sizeof(m_szServerIP));
        strncpy(m_szServerIP, pstrIP, sizeof(m_szServerIP) - 1);
        m_wPort = nPort;
        unsigned ip = GetServerIP(pstrIP);
        m_fdConnect = AsyncConnect(ip, nPort);
        if (m_fdConnect < 0)
        {
            return false;
        }
        m_bConnect = true;
        m_fSendTimeout = fSendTimeout;
        m_fRecvTimeout = fRecvTimeout;
        return true;
    } else
    {
        unsigned ip = GetServerIP(m_szServerIP);
        m_fdConnect = AsyncConnect(ip, m_wPort);
        if (m_fdConnect < 0)
        {
            return false;
        }
        m_bConnect = true;
        return true;
    }
}

void CApi::Close()
{
    if (m_fdConnect >= 0)
    {
        close(m_fdConnect);
        m_fdConnect = -1;
        m_bConnect = false;
    }
}

bool CApi::Send(Commpack &pack)
{
    void * pData;
    int nLen;
    pack.Output(pData, nLen);
    int nRemain = nLen;
    int nSend = 0;
    while (nRemain > 0)
    {
        int nRet = SelectSingleWrite(m_fdConnect, m_fRecvTimeout);
        if (nRet > 0)
        {
            nRet = send(m_fdConnect, (const char *) pData + nSend, nRemain, 0);
            if (nRet < 0)
            {
                return false;
            }
            nSend += nRet;
            nRemain -= nRet;
        }
    }
    return true;
}

bool CApi::Recv(Commpack *&pack)
{
    char buffer[5];
    int nRet = SelectSingleRead(m_fdConnect, m_fRecvTimeout);
    if (nRet <= 0)
    {
        return false;
    }
    int nRecv = recv(m_fdConnect, buffer, sizeof(buffer), 0);
    if (nRecv < 5)
    {
        return false;
    }
    unsigned int len = *((unsigned int*) (buffer + 1));
    len = ntohl(len);
    char * pBuffer = new char[len - 5];
    unsigned int nRemain = len - 5;
    unsigned int nRecved = 0;
    while (nRemain > 0)
    {
        nRet = SelectSingleRead(m_fdConnect, m_fRecvTimeout);
        if (nRet > 0)
        {
            int nRecv = recv(m_fdConnect, pBuffer + nRecved, nRemain, 0);
            if (nRecv <= 0)
            {
                if (errno == EAGAIN)
                {
                    continue;
                } else
                {
                    break;
                }
            }
            nRecved += nRecv;
            nRemain -= nRecv;
        } else
        {
            break;
        }
    }
    if (nRecved != len - 5)
    {
        delete[] pBuffer;
        return false;
    }
    unsigned char* pInputBuffer = new unsigned char[len];
    memcpy(pInputBuffer, buffer, 5);
    memcpy(pInputBuffer + 5, pBuffer, len - 5);
    delete[] pBuffer;
    pack = new Commpack(len + 64);
    bool b = pack->Input((void *) pInputBuffer, len);
    delete[] pInputBuffer;
    return b;
}

/*************************************** 
 测试协议(CMD_TEST)

 Client->Server协议: ulTestData

 Server->Client协议: ulResult ulTestResultData

 ****************************************/
int CApi::Test(unsigned int ulTestData, unsigned int &ulTestResultData)
{
    if (!Open())
    {
        return error_connet;
    }

    Commpack pack(1024);
    pack.SetCmd(CMD_TEST);
    pack.SetSeq(GetSeq());

    pack.AddUInt(ulTestData);

    if (!Send(pack))
    {
        Close();
        return error_send;
    }

    Commpack * pLongPackage = NULL;
    if (!Recv(pLongPackage))
    {
        if (pLongPackage)
        {
            delete pLongPackage;
            pLongPackage = NULL;
        }
        Close();
        return error_recv;
    }

    unsigned int nResult;
    if (!pLongPackage->GetUInt(nResult)
            || pLongPackage->GetUInt(ulTestResultData))
    {
        Close();
    }

    if (pLongPackage)
    {
        delete pLongPackage;
        pLongPackage = NULL;
    }
    return (int) nResult;
}

/*************************************** 
 测试协议(CMD_OnHeartBeat)

 Client->Server协议: ulTestData

 Server->Client协议: ulResult ulTestResultData

 ****************************************/
int CApi::OnHeartBeat(const string &strSessionKey)
{
    if (!Open())
    {
        return error_connet;
    }

    Commpack package(512);
    package.SetCmd(CMD_HEARTBEAT);
    package.SetSeq(GetSeq());

    unsigned int ulUserLen = strSessionKey.length() + 1;
    package.AddUInt(ulUserLen);
    package.AddData((unsigned char*) strSessionKey.c_str(), ulUserLen);

    if (!Send(package))
        return -1;
    else
        return 0;
}

int CApi::OnLogin(const string &strSessionKey)
{
    if (!Open())
    {
        return error_connet;
    }

    Commpack package(512);
    package.SetCmd(CMD_LOGIN);
    package.SetSeq(GetSeq());

    unsigned int ulUserLen = strSessionKey.length() + 1;
    package.AddUInt(ulUserLen);
    package.AddData((unsigned char*) strSessionKey.c_str(), ulUserLen);

    if (!Send(package))
    {
        Close();
        return error_send;
    }

    Commpack * pLongPackage = NULL;
    if (!Recv(pLongPackage))
    {
        if (pLongPackage)
        {
            delete pLongPackage;
            pLongPackage = NULL;
        }
        Close();
        return error_recv;
    }

    unsigned int nResult;
    if (!pLongPackage->GetUInt(nResult))
    {
        Close();
    }

    if (pLongPackage)
    {
        delete pLongPackage;
        pLongPackage = NULL;
    }
    return (int) nResult;
}

int CApi::OnGetAllClient(vector<string> &vecClient)
{
    if (!Open())
        return error_connet;

    Commpack package(512);
    package.SetCmd(CMD_GETALLCLIENT);
    package.SetSeq(GetSeq());

    if (!Send(package))
    {
        Close();
        return error_send;
    }

    Commpack * pLongPackage = NULL;
    if (!Recv(pLongPackage))
    {
        if (pLongPackage)
        {
            delete pLongPackage;
            pLongPackage = NULL;
        }
        Close();
        return error_recv;
    }

    unsigned int nResult;
    unsigned int ulCount;
    if (!pLongPackage->GetUInt(nResult) || nResult != 0
            || !pLongPackage->GetUInt(ulCount))
    {
        Close();
        if (pLongPackage)
        {
            delete pLongPackage;
            pLongPackage = NULL;
        }
        return (int) nResult;
    }

    for (unsigned int i = 0; i < ulCount; i++)
    {
        unsigned int ulLen;
        void * pData;
        if (!pLongPackage->GetUInt(ulLen)
                || !pLongPackage->GetData(pData, ulLen))
        {
            Close();
            if (pLongPackage)
            {
                delete pLongPackage;
                pLongPackage = NULL;
            }
            return error_data;
        }

        vecClient.push_back((char *) pData);
    }

    if (pLongPackage)
    {
        delete pLongPackage;
        pLongPackage = NULL;
    }
    return (int) nResult;
}

int CApi::OnTask1(const string &strIP, unsigned int ulTestData)
{
    if (!Open())
        return error_connet;

    Commpack package(512);
    package.SetCmd(CMD_TASK1);
    package.SetSeq(GetSeq());

    unsigned int ulLen = strIP.length() + 1;
    package.AddUInt(ulLen);
    package.AddData((unsigned char*) strIP.c_str(), ulLen);
    package.AddUInt(ulTestData);

    if (!Send(package))
    {
        Close();
        return error_send;
    }

    Commpack * pLongPackage = NULL;
    if (!Recv(pLongPackage))
    {
        if (pLongPackage)
        {
            delete pLongPackage;
            pLongPackage = NULL;
        }
        Close();
        return error_recv;
    }

    unsigned int nResult;
    if (!pLongPackage->GetUInt(nResult))
    {
        Close();
    }

    if (pLongPackage)
    {
        delete pLongPackage;
        pLongPackage = NULL;
    }
    return (int) nResult;
}

