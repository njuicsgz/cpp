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

//��ȡ��DB����
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
ֻ���������⼸�����
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
		//�õ�����ĸ
		if( iBlankCount == 0 )
		{
			if( isblank( szSqlString[iLoop] ) == 0 )
			{
				iStart[ iBlankCount++ ]  = iLoop;
				continue;
			}
		}
		//�õ������ո�������ĸ
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

	//�������ո�û���޷�������
	if( iBlankCount <3   )
		return strReturn;

	//ֻ����u, i , r, d��������
	switch( szSqlString[ iStart[0] ] )
	{
		//update����: update db.table set
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
		//insert����: insert into db.table values ; insert ignore into db.table
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
		//replace���� : replace into db.table
		//delete����: delete from db.table
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
    //��ȡmysql ������ڵ���mysql_real_escape_string��ʱ����Ҫ��ȡ��Ӧ���ַ���
    MYSQL* connHandle = this->GetConnectHandle();

    char szBuff[4096];
    char cSep = 0;

    string strSrc( pszSqlString );
    string strEscapeStr("");

    unsigned int iBeginPos = 0;
    unsigned int iEndPos = 0;

    unsigned int iPos = 0;
    unsigned int i = 0;

    //ѭ��mysql���
    for( i = 0 ; i < strlen( pszSqlString ); i++ )
    {
        //�ҵ��ָ���
        if( pszSqlString[i] == 39 || pszSqlString[i] == 34 )
        {
            //�ָ������ַ��Ŀ�ʼλ��
            iBeginPos = i + 1;
            cSep = pszSqlString[i];

            //���������ҳ���Ӧ�Ľ���λ��
            while( pszSqlString[i]  )
            {
                i++;

                //�ҵ��˷ָ��������ǲ�һ���ǽ���λ��
                if( iEndPos == 0 && pszSqlString[i] == cSep )
                {
                    iEndPos = i-1;
                    continue;
                }

                if( iEndPos == 0 )
                {
                    continue;
                }

                //ֻ���ڷָ���������������ո��,����)���ǽ���λ��
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

        //�Ѿ��ҵ�һ������������
        if( iEndPos != 0 )
        {
            //�õ��ָ�����ǰ��
            strSql += strSrc.substr(0, iBeginPos-iPos);

            //�õ��ָ������������
            strEscapeStr = strSrc.substr( iBeginPos-iPos , iEndPos-iBeginPos + 1 );
            mysql_real_escape_string(connHandle, szBuff, strEscapeStr.c_str(), strEscapeStr.size() );

            strSql += szBuff;

            //�õ��ָ����ĺ�
            strSrc = strSrc.substr( iEndPos - iPos + 1 );

            iPos = iEndPos+1;
            iBeginPos = 0;
            iEndPos = 0;
        }
    }

    //�����Ҫ��������󲿷ּ���
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

    //��mysql�����д���ȥ��mysql����е������ַ�
    string strSql=szSqlString;

    static int iFailCount = 0;

    Connect( szSqlString );

    //Safe_EscapeMysqlString(szSqlString,strSql);

    int iRet = mysql_real_query(&m_connection, strSql.c_str(), strSql.length() );

    //ִ�гɹ�
    if( 0 == iRet )
    {
        iFailCount = 0;
        return 0;
    }
    //MySQL�������رջ��߶Է������������ڲ�ѯ�ڼ�ʧȥ
    else if( iFailCount < 3 )
    {
        iFailCount++;
        sleep(1);
        Close();
        return Query(szSqlString);
    }
    //ִ��ʧ��3�����ϻ��߷�����������
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
	������Ӱ�������
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
	���ز�ѯ������е�����
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

/* �������һ��insert��ID
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
	�����ֶ���ȡ�ص�ǰ�еĽ��
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
	�����ֶ�����ȡ�ص�ǰ�еĽ��
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
	�����ֶ�����ȡ�ص�ǰ�еĽ��
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

