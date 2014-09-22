#ifndef HTTP_H_H
#define HTTP_H_H

#include <stdlib.h>
#include <stdio.h>

/*
从HTTP服务器Get数据，
如果成功返回相应的HTTP返回码，否则返回-1,
当HTTP返回码是200时，将会解析HTTP包的内容并存到pRevBuf中，
这时使用完pRcvBuf后需要手动释放内存
*/
int GetDataFromHttp(const char* pszUrl, const char* p_addr,
                    int iPort, char** pRevBuf);

/*
从HTTP服务器Post数据，
如果成功返回相应的HTTP返回码，否则返回-1,
当HTTP返回码是200时，将会解析HTTP包的内容并存到pRevBuf中，
这时使用完pRcvBuf后需要手动释放内存
*/
int PostDataToHttp(const char* pszUrl, const char* pszContent,
                   const char* p_addr, int iPort, char** pRevBuf);

#endif

