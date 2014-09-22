#include "HttpSocket.h" 
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#include <stdlib.h>
#define MAXHEADERSIZE 1024
CHttpSocket::CHttpSocket()
{
    m_s = NULL;
    m_phostent = NULL;
    m_port = 80;
    m_bConnected = false;
    for(int i = 0; i < 256; i++)
        m_ipaddr[i] = '\0';
    memset(m_requestheader, 0, MAXHEADERSIZE);
    memset(m_ResponseHeader, 0, MAXHEADERSIZE);
    m_nCurIndex = 0;
    m_bResponsed = false;
    m_nResponseHeaderSize = -1;
}
CHttpSocket::~CHttpSocket()
{
    CloseSocket();
}
bool CHttpSocket::Socket()
{
    if (m_bConnected)
        return false;
    struct protoent *ppe;
    ppe = getprotobyname("tcp");
    m_s = socket(AF_INET, SOCK_STREAM, ppe->p_proto);
    if (m_s == -1)
    {
        return false;
    }
    return true;
}
bool CHttpSocket::Connect(const char *szHostName, int nPort)
{
    if (szHostName == NULL)
        return false;
    if (m_bConnected)
    {
        CloseSocket();
        return false;
    }
    m_port = nPort;
    m_phostent = gethostbyname(szHostName);
    if (m_phostent == NULL)
    {
        return false;
    }
    struct in_addr ip_addr;
    memcpy(&ip_addr, m_phostent->h_addr_list[0], 4);
    struct sockaddr_in destaddr;
    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(80);
    destaddr.sin_addr = ip_addr;
    int ret;
    if ((ret = connect(m_s, (sockaddr*) &destaddr, sizeof(sockaddr))) != 0)
    {
        return false;
    }
    m_bConnected = true;
    return true;
}
const char *CHttpSocket::FormatRequestHeader(const char *pServer,
        const char *pObject, long &Length, char *pCookie, char *pReferer,
        long nFrom, long nTo, int nServerType)
{
    char szPort[10];
    char szTemp[20];
    sprintf(szPort, "%d", m_port);
    memset(m_requestheader, '\0', 1024);
    strcat(m_requestheader, "GET ");
    strcat(m_requestheader, pObject);
    strcat(m_requestheader, " HTTP/1.1");
    strcat(m_requestheader, "\r\n");
    strcat(m_requestheader, "Host:");
    strcat(m_requestheader, pServer);
    strcat(m_requestheader, "\r\n");
    if (pReferer != NULL)
    {
        strcat(m_requestheader, "Referer:");
        strcat(m_requestheader, pReferer);
        strcat(m_requestheader, "\r\n");
    }
    strcat(m_requestheader, "Accept:*/*");
    strcat(m_requestheader, "\r\n");
    strcat(m_requestheader,
            "User-Agent:Mozilla/4.0 (compatible; MSIE 5.00; Windows 98)");
    strcat(m_requestheader, "\r\n");
    strcat(m_requestheader, "Connection:Keep-Alive");
    strcat(m_requestheader, "\r\n");
    if (pCookie != NULL)
    {
        strcat(m_requestheader, "Set Cookie:0");
        strcat(m_requestheader, pCookie);
        strcat(m_requestheader, "\r\n");
    }
    if (nFrom > 0)
    {
        //strcat(m_requestheader,"Range: bytes=");
        //_ltoa(nFrom,szTemp,10);
        //strcat(m_requestheader,szTemp);
        //strcat(m_requestheader,"-");
        //if(nTo > nFrom)
        //{
        //_ltoa(nTo,szTemp,10);
        //strcat(m_requestheader,szTemp);
        //}
        //strcat(m_requestheader,"\r\n");
    }
    strcat(m_requestheader, "\r\n");
    Length = strlen(m_requestheader);
    return m_requestheader;
}
bool CHttpSocket::SendRequest(const char *pRequestHeader, long Length)
{
    if (!m_bConnected)
        return false;

    if (pRequestHeader == NULL)
        pRequestHeader = m_requestheader;
    if (Length == 0)
        Length = strlen(m_requestheader);

    if (send(m_s, pRequestHeader, Length, 0) == -1)
    {
        return false;
    }
    int nLength;
    GetResponseHeader(nLength);
    return true;
}
long CHttpSocket::Receive(char* pBuffer, long nMaxLength)
{
    if (!m_bConnected)
        return NULL;
    long nLength;
    nLength = recv(m_s, pBuffer, nMaxLength, 0);
    if (nLength <= 0)
    {
        CloseSocket();
    }
    return nLength;
}
bool CHttpSocket::CloseSocket()
{
    if (m_s != NULL)
    {
        if (close(m_s) == -1)
        {
            return false;
        }
    }
    m_s = NULL;
    m_bConnected = false;
    return true;
}
int CHttpSocket::GetRequestHeader(char *pHeader, int nMaxLength) const
{
    int nLength;
    if (int(strlen(m_requestheader)) > nMaxLength)
    {
        nLength = nMaxLength;
    }
    else
    {
        nLength = strlen(m_requestheader);
    }
    memcpy(pHeader, m_requestheader, nLength);
    return nLength;
}
bool CHttpSocket::SetTimeout(int nTime, int nType)
{
    if (nType == 0)
    {
        nType = SO_RCVTIMEO;
    }
    else
    {
        nType = SO_SNDTIMEO;
    }
    int dwErr;
    dwErr = setsockopt(m_s, SOL_SOCKET, nType, (char*) &nTime, sizeof(nTime));
    if (dwErr)
    {
        return false;
    }
    return true;
}
const char* CHttpSocket::GetResponseHeader(int &nLength)
{
    if (!m_bResponsed)
    {
        char c = 0;
        int nIndex = 0;
        bool bEndResponse = false;
        while (!bEndResponse && nIndex < MAXHEADERSIZE)
        {
            recv(m_s, &c, 1, 0);
            m_ResponseHeader[nIndex++] = c;
            if (nIndex >= 4)
            {
                if (m_ResponseHeader[nIndex - 4] == '\r'
                        && m_ResponseHeader[nIndex - 3] == '\n'
                        && m_ResponseHeader[nIndex - 2] == '\r'
                        && m_ResponseHeader[nIndex - 1] == '\n')
                    bEndResponse = true;
            }
        }
        m_nResponseHeaderSize = nIndex;
        m_bResponsed = true;
    }
    nLength = m_nResponseHeaderSize;
    return m_ResponseHeader;
}
int CHttpSocket::GetResponseLine(char *pLine, int nMaxLength)
{
    if (m_nCurIndex >= m_nResponseHeaderSize)
    {
        m_nCurIndex = 0;
        return -1;
    }
    int nIndex = 0;
    char c = 0;
    do
    {
        c = m_ResponseHeader[m_nCurIndex++];
        pLine[nIndex++] = c;
    } while (c != '\n' && m_nCurIndex < m_nResponseHeaderSize && nIndex
            < nMaxLength);
    return nIndex;
}
int CHttpSocket::GetField(const char *szSession, char *szValue, int nMaxLength)
{
    if (!m_bResponsed)
        return -1;
    std::string strRespons;
    strRespons = m_ResponseHeader;
    int nPos = -1;
    nPos = strRespons.find(szSession, 0);
    if (nPos != -1)
    {
        nPos += strlen(szSession);
        nPos += 2;
        int nCr = strRespons.find("\r\n", nPos);
        std::string strValue = strRespons.substr(nPos, nCr - nPos);
        strcpy(szValue, strValue.c_str());
        return (nCr - nPos);
    }
    else
    {
        return -1;
    }
}
int CHttpSocket::GetServerState()
{
    if (!m_bResponsed)
        return -1;
    char szState[3];
    szState[0] = m_ResponseHeader[9];
    szState[1] = m_ResponseHeader[10];
    szState[2] = m_ResponseHeader[11];
    return atoi(szState);
}
