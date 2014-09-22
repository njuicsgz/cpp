/*
 * simple_svr.cpp
 *
 *  Created on: 2012-3-27
 *      Author: allengao
 */

#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFLEN  128
#define QLEN    10

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

int init_svr(int type, const struct sockaddr *addr, socklen_t alen, int qlen)
{
    int fd;
    int err = 0;

    if ((fd = socket(addr->sa_family, type, 0)) < 0)
        return -1;
    if (bind(fd, addr, alen) < 0)
    {
        err = errno;
        goto errout;
    }

    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
    {
        if (listen(fd, qlen) < 0)
        {
            err = errno;
            goto errout;
        }
    }

    return fd;

    errout: close(fd);
    errno = err;
    return -1;
}

void serve(int sockfd)
{
    int clfd;
    FILE *fp;
    char buf[BUFLEN];

    while (1)
    {
        clfd = accept(sockfd, NULL, NULL);
        if (clfd < 0)
        {
            printf("ruptimed: accept error[%s]", strerror(errno));
            exit(1);
        }

        if ((fp = popen("/usr/bin/uptime", "r")) == NULL)
        {
            snprintf(buf, sizeof(buf) - 1, "error: %s\n", strerror(errno));
            send(clfd, buf, strlen(buf), 0);
        }
        else
        {
            while (fgets(buf, BUFLEN, fp) != NULL)
                send(clfd, buf, strlen(buf), 0);
            pclose(fp);
        }
        close(clfd);
    }
}

int main(int argc, char *argv[])
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int sockfd, err;

    hint.ai_flags = AI_CANONNAME;
    hint.ai_family = 0;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = 0;
    hint.ai_addrlen = 0;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo("10.168.128.139", "6666", &hint, &ailist)) != 0)
    {
        printf("ruptimed: getaddrinfo error[%s]\n", gai_strerror(err));
        exit(1);
    }

    for(aip = ailist; aip != NULL; aip = aip->ai_next)
    {
        if ((sockfd
                = init_svr(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen, QLEN))
                >= 0)
        {
            serve(sockfd);
            exit(0);
        }
    }

    exit(1);
}
