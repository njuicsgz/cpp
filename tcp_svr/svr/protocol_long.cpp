#include "protocol_long.h"
#include "commpack.h"
#include "sock.h"

int CProtocolLong::GetMsg(Message& req)
{
    int nLen = cached_len();
    char * pData = head();
    if (pData == NULL)
    {
        return 0;
    }
    if (pData != NULL && pData[0] != SOH)
    {
        return -1;
    }
    unsigned long len = *((unsigned long*) (pData + 1));
    len = ntohl(len);
    if (len <= cached_len())
    {
        req.Clear();
        req.AttachMessage(pData, len);
        skip(len);
        return nLen;
    }
    return 0;
}
