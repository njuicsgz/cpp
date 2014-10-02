#ifndef _CONF_H__
#define _CONF_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern struct CONF *G_pConf;

/*结构CONF 封装对配置的操作*/

typedef struct CONF
{
    char *name; /* Name of option */
    char *value; /* Value for this option */
    char *desc; /* Description of this option */
    char *altname; /* Alternate name for this option */
    struct CONF *next;
} CONF;

/**
 * 导入配置文件
 * @param fileName   配置文件名
 */
int LoadConf(const char *fileName);

/**
 * 获取配置项
 * @param name       需要获取的配置项
 * Return ptr
 */
char *GetConf(const char *name);

/**
 * 添加配置项
 * @param name       需要添加的配置项
 * @param value      需要添加的配置项的内容
 */
void SetConf(const char *name, const char *value);

/**
 * 打印输出配置项
 */
void DumpConf(void);

#define	Free(P)	if ((P)) free((P)), (P) = NULL

#ifdef __cplusplus
}
#endif

#endif 
