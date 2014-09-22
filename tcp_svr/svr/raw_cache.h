
#ifndef _RAW_CACHE_H_
#define _RAW_CACHE_H_

//////////////////////////////////////////////////////////////////////////

class CRawCache
{
public:
	CRawCache();
	~CRawCache();
	
	char* head();
	char* tail();
	
	unsigned cached_len();
	unsigned free_len();	//	empty size in buffer tail
	
	void append(const char* data, size_t data_len);
	void skip(unsigned length);
	
protected:
	char* _data;
	size_t _len;
};

//////////////////////////////////////////////////////////////////////////
#endif//_RAW_CACHE_H_
///:~
