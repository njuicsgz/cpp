#ifndef _CONF_H__
#define _CONF_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern struct CONF *G_pConf;

/*�ṹCONF ��װ�����õĲ���*/

typedef struct CONF 
{
	char	*name;		/* Name of option */
	char	*value;		/* Value for this option */
	char	*desc;		/* Description of this option */
	char	*altname;	/* Alternate name for this option */
	struct  CONF *next;
}CONF;

/**
* ���������ļ�
* @param fileName   �����ļ���
*/
int LoadConf(const char *fileName);

/**
* ��ȡ������
* @param name       ��Ҫ��ȡ��������
* Return ptr        
*/
char *GetConf(const char *name);

/**
* ���������
* @param name       ��Ҫ��ӵ������� 
* @param value      ��Ҫ��ӵ������������ 
*/
void  SetConf(const char *name ,const char *value);

/**
* ��ӡ���������
*/
void  DumpConf(void);

#define	Free(P)	if ((P)) free((P)), (P) = NULL

#ifdef __cplusplus
}
#endif

#endif 
