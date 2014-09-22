#ifndef HTTP_H_H
#define HTTP_H_H

#include <stdlib.h>
#include <stdio.h>

/*
��HTTP������Get���ݣ�
����ɹ�������Ӧ��HTTP�����룬���򷵻�-1,
��HTTP��������200ʱ���������HTTP�������ݲ��浽pRevBuf�У�
��ʱʹ����pRcvBuf����Ҫ�ֶ��ͷ��ڴ�
*/
int GetDataFromHttp(const char* pszUrl, const char* p_addr,
                    int iPort, char** pRevBuf);

/*
��HTTP������Post���ݣ�
����ɹ�������Ӧ��HTTP�����룬���򷵻�-1,
��HTTP��������200ʱ���������HTTP�������ݲ��浽pRevBuf�У�
��ʱʹ����pRcvBuf����Ҫ�ֶ��ͷ��ڴ�
*/
int PostDataToHttp(const char* pszUrl, const char* pszContent,
                   const char* p_addr, int iPort, char** pRevBuf);

#endif

