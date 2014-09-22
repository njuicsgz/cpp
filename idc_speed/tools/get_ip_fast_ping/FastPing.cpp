#include "FastPing.h"

const unsigned PING_SEND_INTERVAL = 500;

int my_usleep(unsigned long us)
{
    struct timespec req = {0};
    time_t sec = (int) (us / 1000000);
    us = us - sec * 1000 * 1000;

    req.tv_sec = sec;
    req.tv_nsec = us * 1000L;

    while (nanosleep(&req, &req) == -1 && errno == EINTR)
        continue;

    return 1;
}

void mysleep()
{
    unsigned long long ull = 0;
    for(size_t i = 1; i < 100000; i++)
        ull += i;
}

inline uint16_t in_chksum(uint16_t *addr, int len)
{
    int nleft = len;
    uint32_t sum = 0;
    uint16_t *w = addr;
    uint16_t answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }
    if (nleft == 1)
    {
        *(unsigned char *) (&answer) = *(unsigned char *) w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return (answer);
}

CFastPing::CFastPing(int iTimeOut, const char *pszFileName) :
    m_bDelay(false), m_iFD(0), m_iLastTime(0), m_iEpollWait((iTimeOut < 0
            || iTimeOut > 10000000) ? 10000000 : iTimeOut)
{
    m_stEpoller.create(20000);
    m_iFile = fopen(pszFileName, "w+");
}

CFastPing::~CFastPing()
{
    fclose(m_iFile);
}

int CFastPing::CreateNonBlockSocket(int family, int type, int protocal)
{
    //AF_INET, SOCK_RAW, IPPROTO_ICMP
    int sockfd = socket(family, type, protocal);
    if (sockfd < 0)
    {
        return -1;
    }
    int n;
    int opt;
    for(n = 1; n < 1024; n++)
    {
        opt = n * 1024 * 16;
        if ((setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt))) < 0)
            break;
    }

    for(n = 1; n < 1024; n++)
    {
        opt = n * 1024 * 16;
        if ((setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt))) < 0)
            break;
    }

    int flags = fcntl(sockfd, F_GETFL);
    if (flags < 0)
    {
        close(sockfd);
        return -1;
    }
    flags |= O_NONBLOCK;
    int ret = fcntl(sockfd, F_SETFL, flags);
    if (ret < 0)
    {
        close(sockfd);
        return -1;
    }

    return sockfd;
}
;

bool CFastPing::FastPing(const vector<unsigned long> &vecIP, bool bDelay)
{
    m_bDelay = bDelay;

    m_iFD = CreateNonBlockSocket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (m_iFD == -1)
        return false;

    try
    {
        m_stEpoller.add(m_iFD);
    } catch (exception& ex)
    {
        return false;
    }

    for(unsigned int i = 0; i < vecIP.size(); i++)
    {
        m_mapIP[vecIP[i]] = 0;
    }

    pthread_t tID;
    pthread_create(&tID, NULL, RecvReply, this);

    SendPing(vecIP);

    m_iLastTime = time(NULL);

    pthread_join(tID, NULL);
    close(m_iFD);
    return false;
}

void CFastPing::SendPing(const vector<unsigned long> &vecIP)
{
    struct timeval stTimeVal;

    struct sockaddr_in stDestAddr;
    memset(&stDestAddr, 0, sizeof(struct sockaddr_in));

    struct icmphdr stIcmpHdr;
    memset(&stIcmpHdr, 0, sizeof(struct icmphdr));

    stIcmpHdr.type = ICMP_ECHO;
    stIcmpHdr.code = 0;
    stIcmpHdr.un.echo.id = htons(rand());
    stIcmpHdr.checksum = 0;
    stIcmpHdr.un.echo.sequence = 0;

    char szBuff[1024];
    int iLen = sizeof(struct icmphdr);
    memcpy(szBuff, &stIcmpHdr, iLen);

    int DATALEN = 64;
    for(int i = 0; i < DATALEN - iLen; i++)
    {
        szBuff[iLen + i] = 'a';
    }

    struct icmphdr *pstTmp = (struct icmphdr *) szBuff;
    pstTmp->checksum = in_chksum((uint16_t*) (&szBuff), DATALEN);

    for(unsigned long i = 0; i < vecIP.size(); i++)
    {
        if (m_bDelay)
        {
            gettimeofday(&stTimeVal, NULL);
            memcpy(szBuff + iLen, &stTimeVal, sizeof(struct timeval));

            pstTmp = (struct icmphdr *) szBuff;
            pstTmp->checksum = 0;
            pstTmp->checksum = in_chksum((uint16_t*) (&szBuff), DATALEN);
        }

        stDestAddr.sin_addr.s_addr = vecIP[i];
        sendto(m_iFD, szBuff, DATALEN, 0, (struct sockaddr *) &stDestAddr,
                sizeof(stDestAddr));

        //my_usleep(PING_SEND_INTERVAL);
        //mysleep();
        usleep(0);
    }
}

void* CFastPing::RecvReply(void *p)
{
    CFastPing *pThis = (CFastPing *) p;

    struct sockaddr_in stRecvAddr;
    socklen_t iRecvAddrLen = sizeof(stRecvAddr);
    static char szRecvBuf[4096];
    int iRecvLen;

    while (true)
    {
        CEPollResult res = pThis->m_stEpoller.wait(pThis->m_iEpollWait);
        unsigned int iNowTime = time(NULL);

        if (res.begin() == res.end() || (pThis->m_iLastTime != 0 && iNowTime
                - pThis->m_iLastTime > 10))
        {
            break;
        }

        for(CEPollResult::iterator it = res.begin(); it != res.end(); it++)
        {
            int iFD = it->data.fd;
            if (it->events & EPOLLIN)
            {
                if ((iRecvLen = recvfrom(iFD, szRecvBuf, 4096, 0,
                        (struct sockaddr *) &stRecvAddr, &iRecvAddrLen)) <= 0)
                    continue;

                if (pThis->m_mapIP.find(stRecvAddr.sin_addr.s_addr)
                        == pThis->m_mapIP.end())
                    continue;

                if (!pThis->m_bDelay)
                {
                    pThis->m_mapIP[stRecvAddr.sin_addr.s_addr] = 1;
                    continue;
                }

                struct ip *iph = (struct ip*) szRecvBuf;
                int iIphlen = iph->ip_hl << 2;
                struct timeval stTimeValNow, stTimeValBegin;
                gettimeofday(&stTimeValNow, NULL);
                memcpy(&stTimeValBegin, szRecvBuf + iIphlen
                        + sizeof(struct icmphdr), sizeof(struct timeval));
                pThis->m_mapIP[stRecvAddr.sin_addr.s_addr]
                        = (stTimeValNow.tv_sec - stTimeValBegin.tv_sec)
                                * 1000000 + (stTimeValNow.tv_usec
                                - stTimeValBegin.tv_usec);
                if (pThis->m_iFile != NULL)
                {
                    fprintf(pThis->m_iFile, "%s\t%ld\n", inet_ntoa(
                            stRecvAddr.sin_addr),
                            pThis->m_mapIP[stRecvAddr.sin_addr.s_addr]);
                }
            }
        }
    }

    return NULL;
}
;
