
#ifndef _MESSAGE_QUEUE_H_
#define _MESSAGE_QUEUE_H_

#include <semaphore.h>
#include <pthread.h>
#include <deque>
#include <string>
#include "guardian.h"

//////////////////////////////////////////////////////////////////////////

struct Message
{
	Message():_data(NULL), _length(0), _flow(0){}
	Message(unsigned f):_data(NULL), _length(0), _flow(f){}
	~Message(){if (_data) delete[] _data;}

	Message(const Message& right)
		://_url(right._url),
		_ip(right._ip),
		_port(right._port),
		_data(right._data),
		_length(right._length),
		_flow(right._flow)
	{
		Message* p = (Message*)(&right);
		p->_data = NULL;
	}	//	owener move

	Message& operator = (const Message& right)
	{
		Clear();
		_ip= right._ip;
		_port = right._port;
		_data = right._data;
		_length = right._length;
		_flow = right._flow;
		Message* p = (Message*)(&right);
		p->_data = NULL;
		return *this;
	}	//	owener move

	void AttachMessage(const char* msg, int length)
	{
		if(_data)
		{
			char *sBuf = new char[length+_length];
			memmove(sBuf, _data, _length);
			delete[] _data;
			_data = sBuf;

			memmove(sBuf+_length, msg, length);
			_length = _length + length;
		}
		else
		{
			_data = new char[length];
			memmove(_data, msg, length);
			_length = length;
		}
	}

	char* GetMessage(int& length){length = _length;return _data;}
	unsigned Flow(){return _flow;}
	void Clear()
	{
		if (_data)
		{
			delete[] _data;
			_data = NULL;
			_length = 0;
		}
	}

	//std::string _url;
	std::string _ip;
	std::string _port;
	
protected:
	char* _data;
	int _length;

public:
	unsigned _flow;
};

//////////////////////////////////////////////////////////////////////////

template<typename T>
class MessageQueue
{
public:
	virtual ~MessageQueue(){}
	virtual bool Enqueue(const T& msg) = 0;	//	non-block enqueue
	virtual bool Dequeue(T& msg) = 0;		//	block dequeue
};

template<typename T>
class MQDeque : public MessageQueue<T>
{
public:
	MQDeque(){}
	virtual ~MQDeque(){}
	virtual bool Enqueue(const T& msg){_Q.push_back(msg);return true;}
	virtual bool Dequeue(T& msg){if (_Q.size() == 0)	return false;
		msg = _Q.front();_Q.pop_front();return true;}

protected:
	std::deque<T> _Q;
};

template<typename T>
class MQMutexDecorator : public MessageQueue<T>
{
public:
	MQMutexDecorator(MessageQueue<T>& Q):_Q(Q){}
	virtual ~MQMutexDecorator(){}
	virtual bool Enqueue(const T& msg){ml lock(_mutex);return _Q.Enqueue(msg);}
	virtual bool Dequeue(T& msg){ml lock(_mutex);return _Q.Dequeue(msg);}

protected:
	CMutex _mutex;
	MessageQueue<T>& _Q;
};

template<typename T>
class MQMultiRace : public MessageQueue<T>
{
public:
	MQMultiRace(MessageQueue<T>& Q):_Q(Q){}
	virtual ~MQMultiRace(){}
	virtual bool Enqueue(const T& msg){return _Q.Enqueue(msg);}
	virtual bool Dequeue(T& msg){ml lock(_mutex);return _Q.Dequeue(msg);}

protected:
	CMutex _mutex;
	MessageQueue<T>& _Q;
};

template<typename T>
class MQSemMaxDecorator : public MessageQueue<T>
{
public:
	MQSemMaxDecorator(MessageQueue<T>& Q, size_t max_length):_Q(Q), _max(max_length){}
	virtual ~MQSemMaxDecorator(){}
	virtual bool Enqueue(const T& msg)
	{if(_max.try_lock() == -1)return false;return _Q.Enqueue(msg);}
	virtual bool Dequeue(T& msg)
	{bool ret = _Q.Dequeue(msg);if (ret) _max.unlock();return ret;}
	
protected:
	MessageQueue<T>& _Q;
	CSem _max;
};

template<typename T>
class MQSemSyncDecorator : public MessageQueue<T>
{
public:
	MQSemSyncDecorator(MessageQueue<T>& Q):_Q(Q), _sync(0){}
	virtual ~MQSemSyncDecorator(){}
	virtual bool Enqueue(const T& msg)
	{bool ret = _Q.Enqueue(msg);if (ret) _sync.unlock();return ret;}
	virtual bool Dequeue(T& msg)
	{int ret = _sync.lock();if (ret) return false;return _Q.Dequeue(msg);}
	
protected:
	MessageQueue<T>& _Q;
	CSem _sync;
};

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

typedef MessageQueue<Message> MQ;

inline MQ* GetMQ(size_t q_size)
{
	MQ* q = new MQDeque<Message>();
	q = new MQMutexDecorator<Message>(*q);
	q = new MQSemMaxDecorator<Message>(*q, q_size);
	q = new MQSemSyncDecorator<Message>(*q);
	return q;
}

inline MQ* GetMultiRaceQ(size_t q_size)
{
	MQ* q = new MQDeque<Message>();
	q = new MQMutexDecorator<Message>(*q);
	q = new MQSemMaxDecorator<Message>(*q, q_size);
	q = new MQSemSyncDecorator<Message>(*q);
	q = new MQMultiRace<Message>(*q);
	return q;
}

//////////////////////////////////////////////////////////////////////////
#endif
///:~
