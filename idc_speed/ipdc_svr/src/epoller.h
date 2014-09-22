
#ifndef _EPOLLER_H_
#define _EPOLLER_H_

#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <assert.h>
#include <string>
#include <stdexcept>
#include <iostream>

//////////////////////////////////////////////////////////////////////////

class CEPollResult;

class CEPoller
{
public:
	CEPoller() : _fd(-1), _events(NULL){}
	~CEPoller(){if (_events) delete[] _events;}
	
	void create(size_t iMaxFD) throw(std::runtime_error);
	void add(int fd, int flag = EPOLLIN | EPOLLERR | EPOLLHUP) throw(std::runtime_error);
	void modify(int fd, int flag = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLOUT)  throw(std::runtime_error);
	CEPollResult wait(int iTimeout) throw(std::runtime_error);
	
protected:
	void ctl(int fd, int epollAction, int flag) throw(std::runtime_error);
	
	int _fd;
	epoll_event* _events;
	size_t _maxFD;
};

//////////////////////////////////////////////////////////////////////////

class CEPollResult
{
public:
	~CEPollResult(){}
	CEPollResult(const CEPollResult& right):_events(right._events), _size(right._size){}
	CEPollResult& operator=(const CEPollResult& right)
	{_events = right._events; _size = right._size; return *this;}

	class iterator;
	iterator begin(){return CEPollResult::iterator(0, *this);}
	iterator end(){return CEPollResult::iterator(_size, *this);}
	
	friend class CEPoller;
	friend class CEPollResult::iterator;

protected:
	CEPollResult(epoll_event* events, size_t size):_events(events), _size(size){}
	bool operator==(const CEPollResult& right){return (_events == right._events && _size == right._size);}

	epoll_event* _events;
	size_t _size;

public:
	class iterator
	{
	public:
		iterator(const iterator& right):_index(right._index), _res(right._res){}
		iterator& operator ++(){_index++; return *this;}
		iterator& operator ++(int){_index++; return *this;}
		bool operator ==(const iterator& right){return (_index == right._index && _res == right._res);}
		bool operator !=(const iterator& right){return !(_index == right._index && _res == right._res);}
		epoll_event* operator->(){return &_res._events[_index];}
		
		friend class CEPollResult;
	protected:
		iterator(size_t index, CEPollResult& res): _index(index), _res(res){}
		size_t _index;
		CEPollResult& _res;
	};
};

//////////////////////////////////////////////////////////////////////////
//	implementation
//////////////////////////////////////////////////////////////////////////

inline void CEPoller::create(size_t iMaxFD) throw(std::runtime_error)
{
	_maxFD = iMaxFD;
	_fd = epoll_create(1024);
	if(_fd == -1)
		throw std::runtime_error("epoll_create fail [" + std::string(strerror(errno)) + "]");
	_events = new epoll_event[iMaxFD];
}

inline void CEPoller::add(int fd, int flag) throw(std::runtime_error)
{
	ctl(fd, EPOLL_CTL_ADD, flag);
}

inline void CEPoller::modify(int fd, int flag) throw(std::runtime_error)
{
	ctl(fd, EPOLL_CTL_MOD, flag);
}

inline CEPollResult CEPoller::wait(int iTimeout) throw(std::runtime_error)
{
	int nfds = epoll_wait(_fd, _events, _maxFD, iTimeout);
	if (nfds < 0)
	{
		if (errno != EINTR)
			throw std::runtime_error("epoll_wait fail [" + std::string(strerror(errno)) + "]");
		else
                {
                	//std::cerr << strerror(errno) << std::endl;
			nfds = 0;
                }
	}
	return CEPollResult(_events, nfds);;
}

inline void CEPoller::ctl(int fd, int epollAction, int flag) throw(std::runtime_error)
{
	assert(_fd != -1);
	epoll_event ev;
	ev.data.fd = fd;
	ev.events = flag;
	int ret = epoll_ctl(_fd, epollAction, fd, &ev);
	if (ret < 0)
		throw std::runtime_error("epoll_ctl fail [" + std::string(strerror(errno)) + "]");
}

//////////////////////////////////////////////////////////////////////////
#endif//_EPOLLER_H_
///:~

