#ifndef _IDCSPEED_V3_DATA_STRUCT_2012_12_25
#define _IDCSPEED_V3_DATA_STRUCT_2012_12_25

#include <string>

using namespace std;

struct SIPAttrShort
{
    unsigned short uCountry;
    unsigned char uProvince;
    unsigned char uIsp;

    SIPAttrShort()
    {
        memset(this, 0x00, sizeof(SIPAttrShort));
    }

    unsigned serial() const
    {
        return uCountry * 10000 + uProvince * 100 + uIsp;;
    }

    bool operator <(const SIPAttrShort &ias) const
    {
        return serial() < ias.serial();
    }
};

struct SIPAttr
{
    char szIP[16];
    unsigned int uCountry;
    unsigned int uProvince;
    unsigned int uIsp;
    unsigned int uOptFlag;
    unsigned int uModTS;

    SIPAttr()
    {
        memset(this, 0x00, sizeof(SIPAttr));
    }
};

struct SReptTask
{
    string sIP;
    unsigned uTimestamp;
    unsigned uCountry;
    unsigned uProv;
    unsigned uISP;
    unsigned uDelay;
    unsigned uIPNum;

    unsigned uDayPoint;
    string sDay;
};

#endif

