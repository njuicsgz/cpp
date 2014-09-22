/**
 * @author      allengao
 * @date        2012-01-01
 * @Discription
 *      this test cpp show the usage of following items:
 *      1)how to get configuration from g_szConfFile
 *      2)how to use log by different level, Err/Warn/Info/Debug
 *      3)how to write & read mysql db
 *      4)how to get information from itil
 */
#include "Log.h"
#include "cmysql.h"
#include "TestMySql.hpp"
#include "TestItil.hpp"
#include "TestConf.hpp"

static const char g_szConfFile[] = "../conf/test.conf";

int main(int argc, char **argv)
{
    /* init by configuration file */
    InitConf(g_szConfFile);

    /* init log */
    G_pLog = CreateLog(g_TestConf.szLogFileName,
            g_TestConf.uiLogFileCount, g_TestConf.uiLogFileSize,
            g_TestConf.uiLogLevel);

    /* mysql read/write test */
    CMysql hMySql(g_TestConf.szDBHost, g_TestConf.szDBUsr,
            g_TestConf.szDBPwd);
    MysqlSelectTest(hMySql);
    MysqlUpdateTest(hMySql);

    /* itil request test */
    string sTestIP = "10.168.128.139";
    GetIdcByIPFromItil(g_TestConf.szItilIP, g_TestConf.uiItilPort,
            sTestIP);

    return 0;
}
