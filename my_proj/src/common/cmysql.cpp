#include <stdio.h>
#include <string>
#include <vector>
#include "cmysql.h"

using namespace std;

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
CMysql:: CMysql()
{
    m_bConnected = false;
    m_iField = 0;
    m_result = NULL;
    bzero (m_szHost, sizeof(m_szHost));
    bzero (m_szUser, sizeof(m_szUser));
    bzero (m_szPass, sizeof(m_szPass));
    m_bFieldIndexInitialized = false;
    m_nLastErrNo = 0;
}

CMysql:: CMysql(const char* szHost, const char* szUser, const char* szPass, unsigned short uPort )
{
    m_bConnected = false;
    m_iField = 0;
    m_result = NULL;
    strncpy (m_szHost, szHost, sizeof(m_szHost));
    strncpy (m_szUser, szUser, sizeof(m_szUser));
    strncpy (m_szPass, szPass, sizeof(m_szPass));
    m_uPort = uPort;
    m_strLastDB = "";
    m_bFieldIndexInitialized = false;
}

CMysql:: ~CMysql()
{
    Close();
}

int CMysql:: Close()
{
    if (m_bConnected)
    {
        FreeResult();
        mysql_close(&m_connection);
        m_bConnected= false;
    }
    return 0;
}

int CMysql:: EscapeString (string &str)
{
    if (str.size()==0)
    {
        return 0;
    }

    char *buff= new char[str.size()*2+1];
    mysql_escape_string (buff, str.c_str(), str.size());
    str= buff;
    delete[] buff;

    return 0;
}

int CMysql:: EscapeString(const char* dat, const int len, string &str)
{
    if (dat == NULL)
    {
        return 0;
    }

    char *buff = new char[len*2+1];
    mysql_escape_string (buff, dat, len);
    str = buff;
    delete[] buff;

    return 0;
}

int CMysql:: Connect(const char* szHost, const char* szUser, const char* szPass)
{
    if (!strcmp(szHost, m_szHost))
    {
        return Connect();
    }
    strncpy (m_szHost, szHost, sizeof(m_szHost));
    strncpy (m_szUser, szUser, sizeof(m_szUser));
    strncpy (m_szPass, szPass, sizeof(m_szPass));
    Close();
    return Connect();
}

//获取到DB名称
inline string GetDB(const char *pszSqlString,  int iStartPos, int iEndPos)
{
	string strReturn("");
	char szTmp[2048];
	if( iEndPos > (int)(sizeof(szTmp) - 1 ))
		return strReturn;	
	
	for( int i = iStartPos; i < iEndPos ; i++ )
	{
		if( pszSqlString[ i ] == '.' )
		{
			szTmp[i - iStartPos ]  = 0;
			strReturn = szTmp;
			return strReturn;
		}
		else
		{
			szTmp[i - iStartPos ] =  pszSqlString[ i ] ;
		}
	}
	
	return strReturn;
}

/*
update db.table set
insert into db.table values
insert ignore into db.table
replace into db.table
delete from db.table
只处理如上这几种情况
*/
inline string GetDBFrom(const char *szSqlString)
{
	string strReturn("");
    if( szSqlString == NULL )
        return strReturn;
    
	unsigned int iBlankCount = 0;
	int iStart[5];

	unsigned int iLen = strlen( szSqlString );
	unsigned int iLoop = 0 ;
	for( ; iLoop < iLen-1; iLoop++ )
	{
		//得到首字母
		if( iBlankCount == 0 )
		{
			if( isblank( szSqlString[iLoop] ) == 0 )
			{
				iStart[ iBlankCount++ ]  = iLoop;
				continue;
			}
		}
		//得到各个空格后的首字母
		else
		{
			if( szSqlString[iLoop]  == ' ' && szSqlString[iLoop+1]  != ' ')
			{
				iStart[ iBlankCount ] = ++iLoop;
				if( ++iBlankCount >= 5 )
					break;
			}
		}
	}

	//连两个空格都没有无法组成语句
	if( iBlankCount <3   )
		return strReturn;

	//只处理u, i , r, d四种类型
	switch( szSqlString[ iStart[0] ] )
	{
		//update类型: update db.table set
		case 'u':
		{
			if( iBlankCount < 3 )
			{
				return strReturn;
			}
			else
			{
				return GetDB( szSqlString,   iStart[1],  iStart[2]   );
			}
        }
		//insert类型: insert into db.table values ; insert ignore into db.table
		case 'i':
        {
			if( iBlankCount < 5 )
			{
				return strReturn;
			}
			else
			{
				//insert into db.table values 
				if( szSqlString[ iStart[1] + 1 ] == 'n' )
				{
					return GetDB( szSqlString,  iStart[2],  iStart[3] );
				}
				//insert ignore into db.table
				else if( szSqlString[ iStart[1] + 1 ] == 'g' )
				{
					return GetDB( szSqlString,  iStart[3],  iStart[4] );
				}
				else
				{
					return strReturn;
				}
			}
        }
		//replace类型 : replace into db.table
		//delete类型: delete from db.table
		case 'r':
		case 'd':
        {
			if( iBlankCount < 4 )
			{
				return strReturn;
			}
			else
			{
				return GetDB( szSqlString,   iStart[2],  iStart[3]   );
			}
        }
        default:
			return strReturn;
	}
}

int CMysql:: Connect(const char *szSqlString)
{
    if (!m_bConnected)
    {
        mysql_init (&m_connection);
        if (mysql_real_connect(&m_connection, m_szHost, m_szUser, m_szPass, NULL, m_uPort, NULL, 0) == NULL)
        {
            m_nLastErrNo = mysql_errno(&m_connection);
            snprintf(m_ErrMsg, sizeof(m_ErrMsg)-1, "connect[-h%s -u%s -p%s] fail.\nError %u (%s)\n",
                     m_szHost, m_szUser, m_szPass,
                     mysql_errno(&m_connection), mysql_error(&m_connection));
            throw CMysqlException(m_ErrMsg);
        }
        m_bConnected = true;
    }

    string strCurrDB = GetDBFrom( szSqlString );
    if( strCurrDB.length() == 0 || m_strLastDB == strCurrDB )
    {
        return 0 ;
    }
    else
    {
        mysql_select_db(&m_connection, strCurrDB.c_str() );
	 m_strLastDB = strCurrDB;
    }

    return 0;
}

bool CMysql::IfConnected(const char* szHost)
{
    if (m_bConnected)
        if (!strcmp(szHost, m_szHost))
        {
            return true;
        }
    return false;
}

void CMysql::Safe_EscapeMysqlString(const char* pszSqlString , string &strSql )
{
    //获取mysql 句柄，在调用mysql_real_escape_string的时候需要获取对应的字符集
    MYSQL* connHandle = this->GetConnectHandle();

    char szBuff[4096];
    char cSep = 0;

    string strSrc( pszSqlString );
    string strEscapeStr("");

    unsigned int iBeginPos = 0;
    unsigned int iEndPos = 0;

    unsigned int iPos = 0;
    unsigned int i = 0;

    //循环mysql语句
    for( i = 0 ; i < strlen( pszSqlString ); i++ )
    {
        //找到分隔符
        if( pszSqlString[i] == 39 || pszSqlString[i] == 34 )
        {
            //分隔符内字符的开始位置
            iBeginPos = i + 1;
            cSep = pszSqlString[i];

            //继续往下找出对应的结束位置
            while( pszSqlString[i]  )
            {
                i++;

                //找到了分隔符，但是不一定是结束位置
                if( iEndPos == 0 && pszSqlString[i] == cSep )
                {
                    iEndPos = i-1;
                    continue;
                }

                if( iEndPos == 0 )
                {
                    continue;
                }

                //只有在分隔符后面出现连续空格带,或者)才是结束位置
                if( pszSqlString[i] == ' ' )
                {
                    continue;
                }
                else if( pszSqlString[i] == cSep )
                {
                    i--;
                    iEndPos = 0;
                    continue;
                }
                else if( pszSqlString[i] == ',' || pszSqlString[i] == ')' || pszSqlString[i] == 'a' || pszSqlString[i] == 'o' )
                {
                    break;
                }
                else
                {
                    iEndPos = 0;
                }
            }
        }

        //已经找到一个完整的内容
        if( iEndPos != 0 )
        {
            //得到分隔符的前部
            strSql += strSrc.substr(0, iBeginPos-iPos);

            //得到分隔符里面的内容
            strEscapeStr = strSrc.substr( iBeginPos-iPos , iEndPos-iBeginPos + 1 );
            mysql_real_escape_string(connHandle, szBuff, strEscapeStr.c_str(), strEscapeStr.size() );

            strSql += szBuff;

            //得到分隔符的后部
            strSrc = strSrc.substr( iEndPos - iPos + 1 );

            iPos = iEndPos+1;
            iBeginPos = 0;
            iEndPos = 0;
        }
    }

    //最后还需要把语句的最后部分加上
    strSql += strSrc;

    return;
}

int CMysql:: Query(char* szSqlString)
{
    if( !szSqlString )
    {
        m_nLastErrNo = -1;
        snprintf(m_ErrMsg, sizeof(m_ErrMsg)-1, "SQL is null");
        throw CMysqlException(m_ErrMsg);
    }

    //对mysql语句进行处理，去除mysql语句中的特殊字符
    string strSql=szSqlString;

    static int iFailCount = 0;

    Connect( szSqlString );

    //Safe_EscapeMysqlString(szSqlString,strSql);

    int iRet = mysql_real_query(&m_connection, strSql.c_str(), strSql.length() );

    //执行成功
    if( 0 == iRet )
    {
        iFailCount = 0;
        return 0;
    }
    //MySQL服务器关闭或者对服务器的连接在查询期间失去
    else if( iFailCount < 3 )
    {
        iFailCount++;
        sleep(1);
        Close();
        return Query(szSqlString);
    }
    //执行失败3次以上或者返回其它错误
    else
    {
        iFailCount = 0;
        m_nLastErrNo = mysql_errno(&m_connection);
        snprintf(m_ErrMsg, sizeof(m_ErrMsg)-1, "query fail [%s].\nError=[%s]\nSQL=%s",
                 m_szHost, mysql_error(&m_connection), szSqlString);
        throw CMysqlException(m_ErrMsg);
    }

    return 0;
}


int CMysql:: FreeResult()
{
    if (m_result != NULL)
    {
        mysql_free_result (m_result);
    }
    m_iField = 0;
    m_result = NULL;
    if (m_bFieldIndexInitialized)
    {
        m_FieldIndex.erase(m_FieldIndex.begin(), m_FieldIndex.end());
        m_bFieldIndexInitialized = false;
    }
    return 0;
}

int CMysql:: StoreResult()
{
    FreeResult();
    m_result = mysql_store_result (&m_connection);
    if (m_result == NULL)
    {
        m_nLastErrNo = mysql_errno(&m_connection);
        snprintf(m_ErrMsg, sizeof(m_ErrMsg)-1, "store_result fail:%s!", mysql_error(&m_connection));
        throw CMysqlException(m_ErrMsg);
    }
    m_iField = mysql_num_fields (m_result);
    m_iRows = mysql_num_rows (m_result);
    return 0;
}

char** CMysql:: FetchRow()
{
    if (m_result == NULL)
    {
        StoreResult();
    }
    m_row = mysql_fetch_row (m_result);

    m_FieldLengths= NULL;
    return m_row;
}

int CMysql:: InitFieldName()
{
    if ((!m_bFieldIndexInitialized) && (m_result!=NULL))
    {
        unsigned int i;
        MYSQL_FIELD *fields;

        fields = mysql_fetch_fields(m_result);
        for(i = 0; i < m_iField; i++)
        {
            m_FieldIndex[fields[i].name] = i;
        }
        m_bFieldIndexInitialized = true;
    }
    return 0;
}

const char* CMysql:: GetFieldName(int iField)
{
    if (m_result==NULL)
    {
        return NULL;
    }
    MYSQL_FIELD *fields;
    fields = mysql_fetch_fields(m_result);
    if ((unsigned int)iField> m_iField)
    {
        return NULL;
    }
    return fields[iField].name;
}

/*
	返回受影响的行数
*/
unsigned int CMysql:: GetAffectedRows()
{
    my_ulonglong iNumRows;

    if (!m_bConnected)
    {
        return 0;
    }
    iNumRows = mysql_affected_rows(&m_connection);

    return (unsigned int)iNumRows;
}
/*
	返回查询结果集中的行数
*/
unsigned int CMysql:: GetRowsNum()
{
    my_ulonglong iNumRows;

    if (!m_bConnected || m_result == NULL)
    {
        return 0;
    }
    iNumRows = mysql_num_rows(m_result);

    return (unsigned int)iNumRows;
}

/* 返回最近一次insert的ID
*/
unsigned int CMysql:: GetLastInsertId ()
{
    return (unsigned int)mysql_insert_id(&m_connection);
}

unsigned long CMysql:: GetFieldLength (const char* szFieldName)
{
    InitFieldName();

    return GetFieldLength(m_FieldIndex[szFieldName]);
}

unsigned long CMysql:: GetFieldLength (int rowID)
{
    InitFieldLength();

    return m_FieldLengths[rowID];
}

void CMysql:: InitFieldLength()
{
    if (m_FieldLengths != NULL)
    {
        return;
    }

    m_FieldLengths= mysql_fetch_lengths(m_result);
}

/*
	按照字段名取回当前行的结果
*/
char* CMysql:: GetField(const char* szFieldName)
{
    InitFieldName();
    if(m_FieldIndex.find(szFieldName) == m_FieldIndex.end())
    {
        return NULL;
    }
    else
    {
        return GetField(m_FieldIndex[szFieldName]);
    }
}

/*
	按照字段索引取回当前行的结果
*/
char* CMysql:: GetField(unsigned int iField)
{
    if (iField > m_iField)
    {
        return NULL;
    }
    return m_row[iField];
}

/*
	按照字段索引取回当前行的结果
*/
char* CMysql:: GetField(int iField)
{
    if ((unsigned int)iField > m_iField)
    {
        return NULL;
    }
    return m_row[iField];
}

bool CMysql::GetULong(const char * szFieldName, unsigned long & ulFieldValue)
{
    char * pField = GetField(szFieldName);
    if(pField == NULL)
    {
        return false;
    }
    int iLen = strlen(pField);
    for(int i=0; i<iLen; i++)
    {
        if( !isspace(pField[i]) && !isdigit(pField[i]) )
        {
            return false;
        }
    }
    ulFieldValue = (unsigned long)atoll(pField);
    return true;
}

bool CMysql::GetInt(const char * szFieldName, int & iFieldValue)
{
    char * pField = GetField(szFieldName);
    if(pField == NULL)
    {
        return false;
    }
    int iLen = strlen(pField);
    for(int i=0; i<iLen; i++)
    {
        if( !isspace(pField[i]) && !isdigit(pField[i]) )
        {
            return false;
        }
    }
    iFieldValue = atoi(pField);
    return true;
}

bool CMysql::GetString(const char * szFieldName, string & strFieldValue)
{
    char * pField = GetField(szFieldName);
    if(pField == NULL)
    {
        return false;
    }
    strFieldValue = pField;
    return true;
}

