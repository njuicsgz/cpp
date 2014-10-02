#include <string.h>
#include <assert.h>

#include "raw_cache.h"

CRawCache::CRawCache() :
        _data(NULL), _len(0)
{
}
CRawCache::~CRawCache()
{
    if (_data)
        delete[] _data;
}

char* CRawCache::head()
{
    return _data;
}
char* CRawCache::tail()
{
    return NULL;
}
unsigned CRawCache::cached_len()
{
    return _len;
}
unsigned CRawCache::free_len()
{
    return 0;
}

void CRawCache::append(const char* data, size_t data_len)
{
    assert(data_len < 0x00010000);
    if (_data)
    {
        char* p = new char[_len + data_len + 1];
        memmove(p, _data, _len);
        memmove(p + _len, data, data_len);
        delete[] _data;
        _data = p;
        _len = _len + data_len;
    } else
    {
        _data = new char[data_len + 1];
        memmove(_data, data, data_len);
        _len = data_len;
    }
}

void CRawCache::skip(unsigned length)
{
    if (_data == NULL)
        return;

    else if (length >= _len)
    {
        delete[] _data;
        _data = NULL;
        _len = 0;
    } else
    {
        memmove(_data, _data + length, _len - length);
        _len = _len - length;
    }
}

