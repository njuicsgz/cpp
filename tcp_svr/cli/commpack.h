#ifndef __COMMPACK_H__
#define __COMMPACK_H__

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <string>

using namespace std;

/*
 ******   协议格式    *********
 * unsigned char  SOH;     包头
 * unsigned int   len;     包长
 * unsigned char  HVer;    高版本号
 * unsigned char  LVer;    低版本号
 * unsigned int   cmd;     协议号
 * unsigned short seq;     序列号
 * unsigned int   uin;     序列号
 * unsigned char  body[n]; 变长的包体
 * unsigned char  EOT;     包尾

 1      4      1    1       4       2      4     |         n           1
 +---+---------+----+----+---------+-----+---------+-------------------+---+
 |   |         |    |    |         |     |         |                   |   |
 |SOH|   len   |Hver|Lver|   cmd   | seq |   uin   |       body        |EOT|
 |   |         |    |    |         |     |         |                   |   |
 +---+---------+----+----+---------+-----+---------+-------------------+---+

 */

// 包头和包尾
#define SOH     0x02
#define EOT     0x03

// 版本号
#define COMMPACK_VERSION_HIGH   0x01
#define COMMPACK_VERSION_LOW    0x00

// 包中各个字段的位置
#define COMMPACK_SOH        0
#define COMMPACK_LEN        1
#define COMMPACK_HVER       5
#define COMMPACK_LVER       6
#define COMMPACK_CMD        7
#define COMMPACK_SEQ        11
#define COMMPACK_UIN        13
#define COMMPACK_BODY       17

//包的默认size
#define DEFAULT_SIZE        (4 * 1024)

//包的最小长度
#define PACK_MIN_LEN        18

//Body的最大位置
#define BODY_MAX_POS        (m_totalLen - 1)

//Body的最大长度
#define BODY_MAX_LEN        (m_totalLen - PACK_MIN_LEN)

//Body剩余可用长度
#define BODY_FREE_LEN       (BODY_MAX_POS - m_tail)

class Commpack
{
public:
    Commpack(int length = DEFAULT_SIZE)
    {
        m_totalLen = (length <= PACK_MIN_LEN) ? DEFAULT_SIZE : length;
        m_buff = (unsigned char *) malloc(m_totalLen);
        assert(NULL != m_buff);

        InitPack();
    }

    ~Commpack()
    {
        if (m_buff)
        {
            free(m_buff);
            m_buff = NULL;
        }
    }

public:
    inline void SetVer(unsigned char HVer, unsigned char LVer)
    {
        m_buff[COMMPACK_HVER] = HVer;
        m_buff[COMMPACK_LVER] = LVer;
    }

    inline void GetVer(unsigned char &HVer, unsigned char &LVer)
    {
        HVer = m_buff[COMMPACK_HVER];
        LVer = m_buff[COMMPACK_LVER];
    }

    inline void SetCmd(unsigned int cmd)
    {
        cmd = htonl(cmd);
        memcpy(&m_buff[COMMPACK_CMD], &cmd, sizeof(unsigned int));
    }

    inline unsigned int GetCmd()
    {
        unsigned int cmd;

        memcpy(&cmd, &m_buff[COMMPACK_CMD], sizeof(unsigned int));
        cmd = ntohl(cmd);

        return cmd;
    }

    inline void SetSeq(unsigned short seq)
    {
        seq = htons(seq);
        memcpy(&m_buff[COMMPACK_SEQ], &seq, sizeof(unsigned short));
    }

    inline unsigned short GetSeq()
    {
        unsigned short seq;

        memcpy(&seq, &m_buff[COMMPACK_SEQ], sizeof(unsigned short));
        seq = ntohs(seq);

        return seq;
    }

    inline void SetUin(unsigned int uin)
    {
        uin = htonl(uin);
        memcpy(&m_buff[COMMPACK_UIN], &uin, sizeof(unsigned int));
    }

    inline unsigned int GetUin()
    {
        unsigned int uin;

        memcpy(&uin, &m_buff[COMMPACK_UIN], sizeof(unsigned int));
        uin = ntohl(uin);

        return uin;
    }

    inline bool AddData(const void *data, int len)
    {
        bool bRet = false;

        if (data == NULL || len < 0)
            return false;

        if (m_tail + len > BODY_MAX_POS)
        {
            bRet = Resize(len);
            if (false == bRet)
                return false;
        }

        memcpy(&m_buff[m_tail], data, len);
        m_tail += len;
        m_buff[m_tail] = EOT;

        return true;
    }

    inline bool GetData(void *&data, const int len)
    {
        if (len < 0)
            return false;

        if (m_currPos + len > m_tail)
            return false;

        data = &m_buff[m_currPos];
        m_currPos += len;

        return true;
    }

    inline bool Input(void *buff, const int len)
    {
        if (buff == NULL || len < PACK_MIN_LEN || len > m_totalLen)
            return false;

        memcpy(m_buff, buff, len);

        m_currPos = COMMPACK_BODY;
        m_tail = len - 1;

        if (false == CheckPackFormat())
            return false;

        return true;
    }

    inline void Output(void *&buff, int &size)
    {
        int len = m_tail + 1;

        len = htonl(len);
        memcpy(&m_buff[COMMPACK_LEN], &len, sizeof(int));

        buff = m_buff;
        size = m_tail + 1;
    }

    inline bool AddByte(unsigned char param)
    {
        return AddData(&param, sizeof(unsigned char));
    }

    inline bool GetByte(unsigned char &param)
    {
        void *data;

        if (false == GetData(data, sizeof(unsigned char)))
            return false;

        memcpy(&param, data, sizeof(unsigned char));

        return true;
    }

    inline bool AddUShort(unsigned short param)
    {
        param = htons(param);
        return AddData(&param, sizeof(unsigned short));
    }

    inline bool GetUShort(unsigned short &param)
    {
        void *data;

        if (false == GetData(data, sizeof(unsigned short)))
            return false;

        memcpy(&param, data, sizeof(unsigned short));
        param = ntohs(param);

        return true;
    }

    inline bool AddUInt(unsigned int param)
    {
        param = htonl(param);
        return AddData(&param, sizeof(unsigned int));
    }

    inline bool GetUInt(unsigned int &param)
    {
        void *data;

        if (false == GetData(data, sizeof(unsigned int)))
            return false;

        memcpy(&param, data, sizeof(unsigned int));
        param = ntohl(param);

        return true;
    }

    inline bool AddString(const string &param)
    {
        if (false == AddUInt(param.length() + 1))
            return false;

        return AddData(param.c_str(), param.length() + 1);
    }

    inline bool GetString(string &param)
    {
        void *data;
        unsigned int len;

        if (false == GetUInt(len))
            return false;

        if (false == GetData(data, len))
            return false;

        param.assign((char *) data, len > 0 ? len - 1 : 0);

        return true;
    }

    inline void SeekToBegin()
    {
        m_currPos = COMMPACK_BODY;
    }

private:
    inline void InitPack()
    {
        unsigned int len;

        memset(m_buff, 0, m_totalLen);

        m_currPos = COMMPACK_BODY;
        m_tail = COMMPACK_BODY;

        m_buff[COMMPACK_SOH] = SOH;
        m_buff[m_tail] = EOT;

        len = htonl(PACK_MIN_LEN);
        memcpy(&m_buff[COMMPACK_LEN], &len, sizeof(unsigned int));

        SetVer(COMMPACK_VERSION_HIGH, COMMPACK_VERSION_LOW);
    }

    inline bool CheckVer()
    {
        return (m_buff[COMMPACK_HVER] == COMMPACK_VERSION_HIGH)
                && (m_buff[COMMPACK_LVER] == COMMPACK_VERSION_LOW);
    }

    inline bool CheckPackFormat()
    {
        if (m_buff[COMMPACK_SOH] != SOH || m_buff[m_tail] != EOT)
            return false;

        if (false == CheckVer())
            return false;

        return true;
    }

    //len为新添加数据的长度
    inline bool Resize(size_t len)
    {
        unsigned char *new_buff;

        /*
         *  将新数据长度减去剩余长度, 则为需要增长长度
         *  将该长度加上包总长, 则为需要申请的内存长
         *  加(4096 + 1)对4096取整, 得到4K的整数倍
         */
        size_t newLen = 4096
                * ((len - BODY_FREE_LEN + m_totalLen + 4097) / 4096);
        new_buff = (unsigned char *) malloc(newLen);
        if (NULL == new_buff)
            return false;

        memset(new_buff, 0, newLen);
        memcpy(new_buff, m_buff, m_totalLen);
        free(m_buff);
        m_buff = new_buff;
        m_totalLen = newLen;

        return true;
    }

public:
    unsigned char *m_buff; //整个包的BUFFER
    int m_currPos; //当前位置, 只指向Body部分
    int m_tail; //包尾的位置
    int m_totalLen; //整个包的包长
};

#endif
