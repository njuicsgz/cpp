#include "Log.h"
#include "cmysql.h"

int MysqlSelectTest(CMysql &hMySql)
{
    char szSql[] =
            "select * from gslbcc2.Tbl_Domain_Info where strDomain='www.qq.com.';";
    try
    {
        hMySql.Query(szSql);
        hMySql.StoreResult();
        while (NULL != hMySql.FetchRow())
        {
            char * pszDomain = hMySql.GetField("strDomain");
            char * pszAux = hMySql.GetField("ulAux");
            size_t nAux = (pszAux) ? atoi(pszAux) : 0;

            Info("[MysqlSelectTest] OK! strDomain[%s], ulAux[%u]",pszDomain, nAux);
        }
    } catch (exception &ex)
    {
        Err("[MysqlSelectTest] Fail!");
        return -1;
    }

    return 0;
}

int MysqlUpdateTest(CMysql &hMySql)
{
    char szSql[] =
            "update gslbcc2.Tbl_Domain_Info set ulAux='100' where strDomain='www.qq.com.';";
    try
    {
        hMySql.Query(szSql);
    } catch (CCommonException & ex)
    {
        Err("[MysqlUpdateTest] Fail!");
        return -1;
    }
    Info("[MysqlUpdateTest] OK!");
    return 0;
}
