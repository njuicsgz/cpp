#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include "Conf.h"
#include "Log.h"

struct CONF *G_pConf;

static char conf_file[PATH_LEN];
/**
* strtrimlead   
* @param str
*/
static char *strtrimlead(char *str)
{
	char *obuf;
	if (str)
	{
		for (obuf = str; *obuf && isspace((int)(*obuf)); ++obuf)
			;
		if (str != obuf)
			memmove(str, obuf, strlen(obuf) + 1);
	}
	return (str);
}

/**
* strtrimlead   
* @param str
*/
static char *strtrimtrail(char *str)
{
	int i;

	if (str && 0 != (i = strlen(str)))
	{
		while (--i >= 0)
		{
			if (!isspace((int)(str[i])))
				break;
		}
		str[++i] = '\0';
	}
	return (str);
}

/**
* strtrimlead   
* @param str
*/
static char *strtrim(char *str)
{
	strtrimlead(str);
	strtrimtrail(str);
	return (str);
}

/**
* strtoupper   
* @param str
*/
/*
static char *strtoupper(char *str)
{
	register char *c;

	if (!str || !*str)
		return (NULL);
	for (c = str; *c; c++)
		*c = toupper(*c);
	return (str);
}  
*/

/**
* strtolower   
* @param str
*/
static char *strtolower(char *str)
{
	register char *c;

	if (!str || !*str)
		return (NULL);
	for (c = str; *c; c++)
		*c = tolower(*c);
	return (str);
}  

/**
* conf_trim_comments   
* @param str
*/
static void conf_trim_comments(char *s)
{
	register char *c;

	for (c = s; *c; c++)
	{
		if (*c == '\\')
			c++;
		else if (c[0] == '#' || (c[0] == '/' && (c[1] == '*' || c[1] == '/')))
		{
			*c = '\0';
			return;
		}
	}
}

/**
* conf_trim_comments   
* @param str
* @param namep
* @param valuep
*/
static void conf_get_option(char *str, char **namep, char **valuep)
{
	register char	*c;
	register int	got_equal = 0;								
	register size_t nlen, vlen;
	char	*name, *value;

	if (!(name = (char *)malloc(1 * sizeof(char))))
	{
		Err("%s","malloc");
	}
	if (!(value = (char *)malloc(1 * sizeof(char))))
		Err("%s","malloc");
	nlen = vlen = (size_t)0;
	name[0] = '\0';
	value[0] = '\0';

	*namep = *valuep = (char *)NULL;

	for (c = str; *c; c++)
	{
		if (*c == '=' && !got_equal)
		{
			got_equal = 1;
			continue;
		}

		if (*c == '\\')
			c++;

		if (!got_equal)
		{
			if (!name)
			{
				if (!(name = (char *)malloc(nlen + 2)))
					Err("%s","malloc");
			}
			else
			{
				if (!(name = (char *)realloc(name, (nlen + 2))))
					Err("%s","realloc");
			}
			name[nlen++] = *c;
			name[nlen] = '\0';
		}
		else
		{
			if (!value)
			{
				if (!(value = (char *)malloc(vlen + 2)))
					Err("%s","malloc");
			}
			else
			{
				if (!(value = (char *)realloc(value, (vlen + 2))))
					Err("%s","realloc");
			}
			value[vlen++] = *c;
			value[vlen] = '\0';
		}
	}
	strtrim(name);
	strtolower(name);
	strtrim(value);

	if (strlen(name))
		*namep = name;
	else
	{
		Free(name);
	}

	if (strlen(value))
		*valuep = value;
	else
	{
		Free(value);
	}
}

/**
* ReplaceConf   
* @param name
* @param value
*/
static void ReplaceConf(const char *name,const char *value)
{
	struct CONF *c;

	if (!name || !value)
		return;

	for (c = G_pConf; c; c = c->next)
		if (c->name && !strncasecmp(c->name, name, strlen(c->name)))
		{
			Free(c->value);
			c->value = strdup(value);
		}
}

/**
* SetConf   
* @param name
* @param value
*/
void SetConf(const char *name,const char *value)
{
	struct	CONF *newconf;

	if (!name)
	{
		ReplaceConf(name, value);
		return;
	}

	if (!(newconf = (struct CONF *)calloc(1, sizeof(struct CONF))))
		Err("%s","calloc");
	if (!(newconf->name = strdup(name)))
		Err("%s","strdup");
	if (!(newconf->value = value ? (char *)strdup(value) : (char *)NULL))
		Err("%s","strdup");

	newconf->next = G_pConf;
	G_pConf = newconf;
}

/**
* SetConf   
* @param name
* return ptr value
*/
char * GetConf(const char *name)
{
	struct  CONF *c;
	for (c = G_pConf; c; c = c->next)
		if (!strncasecmp(c->name, name, strlen(c->name))
			 || (c->altname && !strncasecmp(c->altname, name, strlen(c->name))))
		{
			return (c->value);
		}
	return (NULL);
}

/**
* 导入配置文件
* @param fileName   配置文件名
*/
int LoadConf(const char *filename)
{
	FILE	*fp;														
	char	linebuf[BUFSIZ];										
	char	*name, *value;											
	int	lineno = 0;											
	struct stat st;									

	if (stat(filename, &st))
	{
		printf("conf file %s not exist!\n",filename);
		return -1;
	}

	strncpy(conf_file,filename,PATH_LEN-1);	

	if ((fp = fopen(filename, "r")))
	{
		while (fgets(linebuf, sizeof(linebuf), fp))
		{
			lineno++;
			conf_trim_comments(linebuf);
			conf_get_option(linebuf, &name, &value);
			if (name)
			{
				if (value)
					SetConf(name, value);
				Free(name);
			}
			if (value)
			{
				Free(value);
			}
		}
		fclose(fp);
	}
	else
	{
		printf("%s: %s", filename, strerror(errno));
		return -1;
	}
	return 0;
}

/**
* 销毁指针
*/
void DestroyConf(void)
{
	struct CONF	*c;
	for(c = G_pConf; c; c = c->next)
	{
		Free(c->name);
		Free(c->value);
		Free(c->desc);
		Free(c->altname);
	}
	Free(G_pConf);
}

/**
* 打印输出配置项
*/
void DumpConf(void)
{
	time_t	time_now = time(NULL);
	struct CONF	*c;

	if(G_pConf == NULL)
	{
		printf("No Config File\n");
		return;
	}

	puts("##");
	puts(conf_file);
	printf("%.24s\n", ctime(&time_now));
	puts("##");

	for(c = G_pConf; c; c = c->next)
	{
		if (c->name)
		{
			printf("%s : %s\n",c->name , c->value);
		}
	}
}


