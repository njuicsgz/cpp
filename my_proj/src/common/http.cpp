#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include "http.h"

static const char szGetHttpHeader[]=
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Length: 0\r\n"
    "\r\n";

static const char szPostHttpHeader[]=
    "POST %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Length: %d\r\n"
    "\r\n"
    "%s";

#define HTTP_MAX_LENGTH 8092

int SetNoBlock(int fd)
{
    int val = fcntl(fd, F_GETFL, 0);

    if ( val == -1)
    {
        return -1;
    }

    if (fcntl(fd, F_SETFL, val | O_NONBLOCK | O_NDELAY) == -1)
    {
        return -1;
    }

    return 0;
}

int Open(const char* p_addr, int port)
{
    int fd;
    int iret;
    struct sockaddr_in svr_addr;
    fd_set readfd, writefd;
    int error;
    socklen_t optlen = sizeof(error);

    if( NULL == p_addr || port <= 0 )
    {
        return -1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if( -1 == fd )
    {
        fprintf(stderr, "%s\n", strerror(errno) );

        return -1;
    }

    iret = SetNoBlock(fd);

    if( -1 == iret )
    {
        fprintf(stderr, "%s\n", strerror(errno) );

        close(fd);

        return -1;
    }

    bzero(&svr_addr, sizeof(struct sockaddr_in));
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = htons(port);
    svr_addr.sin_addr.s_addr = inet_addr(p_addr);

    iret = connect(fd, (struct sockaddr*)&svr_addr, sizeof(struct sockaddr_in) );

    if( 0 == iret )
    {
        return fd;
    }
    else if ( iret < 0 && errno != EINPROGRESS )
    {
        fprintf(stderr, "%s\n", strerror(errno) );

        close(fd);

        return -1;
    }

    FD_ZERO(&readfd);
    FD_ZERO(&writefd);
    FD_SET(fd, &readfd);
    FD_SET(fd, &writefd);
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    iret = select( fd + 1, &readfd, &writefd, NULL, &tv);
    if( iret > 0 )
    {
        if( FD_ISSET(fd, &readfd) || FD_ISSET(fd, &writefd) )
        {
            iret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &optlen);

            if ( iret < 0 || error )
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

//从fd接收remain_len长度的数据，并存入prcv_buf中
int Receive(int fd, char* prcv_buf, int remain_len )
{
    int iret;
    unsigned long data_len = 0;
    int recv_len = 0;
    struct timeval tv;
    fd_set r_set;

    if( fd >= 1024 || fd < 0 )
    {
        printf("fd is invalid\n");

        return -1;
    }

    FD_ZERO(&r_set);
    FD_SET(fd, &r_set);

    tv.tv_sec = (int) 10;
    tv.tv_usec = 0;

    while( remain_len > 0 )
    {
        iret = select ( fd + 1, &r_set, NULL, NULL, &tv);

        if( iret <= 0 )
        {
            return -1;
        }

        recv_len = recv(fd, prcv_buf + data_len, remain_len, 0);

        if( recv_len <= 0)
        {
            if( errno == EAGAIN )
            {
                continue;
            }
            else
            {
                return -1;
            }
        }

        data_len += recv_len;
        remain_len -= recv_len;
    }

    return 0;
}

int Send(int fd, const char* psnd_buf, int remain_len)
{
    int iret = 0;
    int ilen = 0;
    int send_len = 0;

    struct timeval tv;
    fd_set w_set;

    if(fd >= 1024 || fd < 0 )
    {
        return -1;
    }

    FD_ZERO(&w_set);
    FD_SET(fd, &w_set);

    tv.tv_sec = 10;
    tv.tv_usec = 0;

    //在10秒内发送remain_len长度的数据
    while( remain_len > 0 )
    {
        iret = select( fd+1, NULL , &w_set, NULL, &tv);

        //select异常或者发送已经超时
        if( iret <= 0 )
        {
            return -1;
        }

        send_len = send(fd, psnd_buf + ilen, remain_len, 0);

        if( send_len <= 0 )
        {
            if( errno == EAGAIN )
            {
                continue;
            }
            else
            {
                return -1;
            }
        }

        ilen += send_len;
        remain_len -= send_len;
    }

    return 0;
}

char* ReceiveHttpData(int fd, float fTimeOut, int* pRet)
{
    int iTotalRcvLen = 0;     //总共接收数据包的长度
    int iContentLen  = 0;     //Http包内容长度
    int iHeadLen     = 0;     //Http包头长度
    int iBytes       = 0;

    char* pszHttp = NULL;
    char* pszPos = NULL;
    char* pChunkLenStart = NULL;
    char* pRcvBuf = NULL;

    struct timeval tv;
    fd_set r_set;

    FD_ZERO(&r_set);
    FD_SET(fd, &r_set);

    tv.tv_sec  = (int)fTimeOut;
    tv.tv_usec = (int)(( fTimeOut - (int)fTimeOut) * 1000000 );

    *pRet = -1;

    pszHttp = (char*) malloc( HTTP_MAX_LENGTH * sizeof(char) );

    if( NULL == pszHttp )
    {
        return NULL;
    }

    //接收http头数据,直到接收到http头的结束符或者已经超时
    while( 1 )
    {
        if( select( fd + 1, &r_set, NULL, NULL, &tv) <= 0 )
        {
            goto err;
        }

        if( HTTP_MAX_LENGTH <= iTotalRcvLen )
        {
            goto err;
        }

        iBytes = recv(fd, pszHttp + iTotalRcvLen, HTTP_MAX_LENGTH - iTotalRcvLen, 0);

        if( iBytes <= 0)
        {
            if( errno == EAGAIN )
            {
                continue;
            }
            else
            {
                goto err;
            }
        }

        iTotalRcvLen += iBytes;

        //判断是否已经接收到http头的线束符
        pszPos = strstr(pszHttp, "\r\n\r\n");

        if( NULL != pszPos )
        {
            iHeadLen = (pszPos - pszHttp) + strlen("\r\n\r\n");

            break;
        }
    }

    //没有接收到HTTP数据包头的结束符
    if( NULL == pszPos )
    {
        goto err;
    }

    /*
    HTTP/1.1 200 OK
    Server: Microsoft-IIS/4.0
    Date: Mon, 3 Jan 2005 13:13:33 GMT
    Content-Type: text/html
    Last-Modified: Mon, 11 Jan 2004 13:23:42 GMT
    Content-Length: 90
    */

    //解析HTTP包头的返回码
    pszPos = strstr(pszHttp, " ");

    if( NULL == pszPos )
    {
        goto err;
    }

    *pRet = atoi(pszPos);

    if( *pRet != 200 )
    {
        goto err;
    }

    pszPos = strstr(pszHttp, "Content-Length: ");

    if( NULL == pszPos )
    {
        pszPos = strstr(pszHttp, "Content-length: ");
    }

    //解析Content-Length:类型的数据包
    if( NULL != pszPos )
    {
        int iLeftLen = 0;
        //HTTP数据包内容总长度
        int iTotalConLen = atoi( pszPos + strlen("Content-Length: ") );

        if( 0 == iTotalConLen )
        {
            pRcvBuf = (char*) malloc( (iTotalConLen + 1 ) * sizeof(char) );

            iContentLen = 0;

            goto out;
        }

        pRcvBuf = (char*) malloc( (iTotalConLen + 1 ) * sizeof(char) );

        iContentLen = iTotalRcvLen - iHeadLen;

        memcpy(pRcvBuf, pszHttp + iHeadLen, iContentLen);

        iLeftLen = iTotalConLen - iContentLen;

        while( iLeftLen > 0 )
        {
            if( select( fd + 1, &r_set, NULL, NULL, &tv) <= 0 )
            {
                goto err;
            }

            iBytes = recv(fd, pRcvBuf + iContentLen, iLeftLen, 0);

            if( iBytes <= 0)
            {
                if( errno == EAGAIN )
                {
                    continue;
                }
                else
                {
                    goto err;
                }
            }

            iContentLen += iBytes;
            iLeftLen    -= iBytes;
        }

        goto out;
    }

    //解析Transfer-Encoding: chunked类型的数据包
    pszPos = strstr(pszHttp, "Transfer-Encoding: chunked");

    if( pszPos == NULL )
    {
        goto err;
    }

    pRcvBuf = (char*)malloc( (HTTP_MAX_LENGTH - iHeadLen )* sizeof(char) );

    pChunkLenStart = pszHttp + iHeadLen;

    while( 1 )
    {
        //查找Chunk块结束标志,从而得到块大小
        char* pszChunkEnd = strstr(pChunkLenStart, "\r\n");

        while( NULL != pszChunkEnd )
        {
            int ChunkSize = strtol(pChunkLenStart, NULL, 16);//得到块大小

            //已经接收完所有数据
            if( 0 == ChunkSize )
            {
                goto out;
            }

            //如果还没有接收到一块完整的数据，则继续接收
            if( pszHttp + iTotalRcvLen - pszChunkEnd - (int)strlen("\r\n") < ChunkSize )
            {
                break;
            }

            memcpy(pRcvBuf + iContentLen, pszChunkEnd + (int)strlen("\r\n"), ChunkSize);
            iContentLen += ChunkSize;

            pChunkLenStart = pszChunkEnd + strlen("\r\n") + ChunkSize;
            pszChunkEnd = strstr(pChunkLenStart, "\r\n");
        }

        //继续接收其它Chunk块的数据
        if( select( fd + 1, &r_set, NULL, NULL, &tv) <= 0 )
        {
            goto err;
        }

        if( HTTP_MAX_LENGTH <= iTotalRcvLen )
        {
            goto err;
        }

        iBytes = recv(fd, pszHttp + iTotalRcvLen, HTTP_MAX_LENGTH - iTotalRcvLen, 0);

        if( iBytes <= 0 )
        {
            if( errno == EAGAIN )
            {
                continue;
            }
            else
            {
                goto err;
            }
        }

        iTotalRcvLen += iBytes;
    }

err:
    if( NULL != pRcvBuf )
    {
        free(pRcvBuf);

        pRcvBuf = NULL;
    }

out:
    if( NULL != pszHttp )
    {
        free(pszHttp);
    }

    if( NULL != pRcvBuf )
    {
        *(pRcvBuf + iContentLen) = '\0';
    }

    return pRcvBuf;
}

/*
发送数据包给HTTT服务器，
如果成功返回相应的HTTP返回码，否则返回-1,
当HTTP返回码是200时，将会解析HTTP包的内容并存到pRevBuf中，
这时使用完pRcvBuf后需要手动释放内存
*/
int GetDataFromHttp(const char* pszUrl, const char* p_addr,
                    int iPort, char** pRevBuf)
{
    int fd = -1;
    int iret = -1;
    int ilen = sizeof(char) * (strlen(pszUrl) + 512);
    char* pszGet = NULL;

    pszGet = (char*) malloc( ilen );

    if( NULL == pszGet )
    {
        return -1;
    }

    if( ( fd = Open(p_addr, iPort) ) < 0 )
    {
        goto err;
    }

    if( fd >= 1024 )
    {
        goto err;
    }

    snprintf(pszGet, ilen - 1, szGetHttpHeader, pszUrl, p_addr);

    //发送Get请求给服务端
    if( Send(fd, pszGet, strlen(pszGet) ) < 0 )
    {
        goto err;
    }

    //接收Http回包
    *pRevBuf = ReceiveHttpData(fd, 10.0, &iret);

err:
    if( fd >= 0 )
    {
        close(fd);
    }

    if( NULL != pszGet )
    {
        free(pszGet);
    }

    return iret;
}

/*
从HTTP服务器Post数据，
如果成功返回相应的HTTP返回码，否则返回-1,
当HTTP返回码是200时，将会解析HTTP包的内容并存到pRevBuf中，
这时使用完pRcvBuf后需要手动释放内存
*/
int PostDataToHttp(const char* pszUrl, const char* pszContent,
                   const char* p_addr, int iPort, char** pRevBuf)
{
    int fd = -1;
    char* pszPost = NULL;
    int iret = -1;
    int ilen = sizeof(char) * ( strlen(pszContent) + strlen(pszUrl) + 512 );

    pszPost = (char*) malloc( ilen );

    if( NULL == pszPost )
    {
        return -1;
    }

    if( ( fd = Open(p_addr, iPort) ) < 0 )
    {
        goto err;
    }

    if( fd >= 1024 )
    {
        goto err;
    }

    snprintf(pszPost, ilen - 1, szPostHttpHeader, pszUrl, p_addr,
             (int)strlen(pszContent), pszContent);

    //发送Get请求给服务端
    if( Send(fd, pszPost, strlen(pszPost) ) < 0 )
    {
        goto err;
    }

    //接收Http回包
    *pRevBuf = ReceiveHttpData(fd, 10.0, &iret);

err:
    if( fd >= 0 )
    {
        close(fd);
    }

    if( NULL != pszPost )
    {
        free(pszPost);
    }

    return iret;
}

