#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include "runner.h"
#include "Helper.h"
#include "CSessionMgr.h"

using namespace network;
using namespace std;

static const size_t C_READ_BUFFER_SIZE = 4096;

CReqAcceptor::CReqAcceptor(CEPoller& epoller, CachedConn& collection,MQ& Q)
:_epoller(epoller), _conns(collection),_flow(0), _Q(Q){}
CReqAcceptor::~CReqAcceptor(){}

void CReqAcceptor::Init(const std::string& sHost, unsigned short port)
{
	try
	{
		_listen.create();
		_listen.set_reuseaddr();
		_listen.bind(port, sHost);
		_listen.listen();
	}
	catch (socket_error& err)
	{
			Err("[Runner]:maybe ip or port is invalid.CReqAcceptor::Init,%s",err.what());
            		assert(false);
	}
}

void CReqAcceptor::OnConnect(CSockAttacher &sock,int flow)
{
	
}

void CReqAcceptor::Run()
{
	for(;;)
	{
		try
		{
			CSockAttacher sock(-1);	
			_listen.accept(sock);
			sock.set_nonblock();
			sock.set_reuseaddr();
			unsigned tempflow=_flow++;
			_conns.AddConn(sock.fd(),tempflow);
			_epoller.add(sock.fd());
			OnConnect(sock,tempflow);		

			string strIP;
			port_t ulPort;
			sock.get_peer_name(strIP, ulPort);

			MapFlow2IP(strIP, tempflow );
		}
		catch (socket_error& err)
		{
			if (err._err_no == EINTR || err._err_no == EAGAIN)
				continue;
		}
	}
}

CReceiver::CReceiver(CEPoller& epoller, CachedConn& collection, MQ& Q)
:_epoller(epoller), _conns(collection), _Q(Q){}
CReceiver::~CReceiver(){}

void CReceiver::Run()
{
	unsigned long long count = 0;
	unsigned long long fail = 0;
    
	for(;;)
	{
		CEPollResult res = _epoller.wait(C_EPOLL_WAIT);
		for(CEPollResult::iterator it = res.begin()
			; it != res.end()
			; it++ )
		{
			int fd = it->data.fd;
			unsigned flow = 0;
			
			//没有EPOLLIN和EPOLLOUT事件
			if (!(it->events & (EPOLLIN | EPOLLOUT))) 
			{
				Warn("%s","[Runner]:NOT EPOLLIN | EPOLLOUT\n");
				_conns.CloseConn(fd);
				continue;
			}

			//EPOLLOUT事件
			if (it->events & EPOLLOUT)
			{
	
				unsigned flow = 0;
				int sent = _conns.Send(fd, flow);
				if (sent == 0)	
				{					
					try
					{
						_epoller.modify(fd,EPOLLIN | EPOLLERR | EPOLLHUP);
					}
					catch (exception& ex)
					{
						_conns.CloseConn(fd);
					}
					continue;
				}
				else if(sent<0)
				{
					_conns.CloseConn(fd);

				}
			}
			
			//EPOLLIN事件
			if (!(it->events & EPOLLIN))
			{
				continue;
			}

			int rcv =  _conns.Recv(fd, flow);
			if (rcv <= 0)
			{
				_conns.CloseConn(fd);
				continue;
			}	
			
			for(int i = 0; i < 100; i++)
			{
				Message msg(flow);
				int msg_len = _conns.GetMessage(fd, msg);
				
				if (msg_len == 0)
				{
					break;
				}
				
				else if (msg_len < 0)	
				{
					_conns.CloseConn(fd);
					break;
				}
				else
				{
					if (!_Q.Enqueue(msg))
					{
						fail++;
						break;
					}
					else
					{
						count++;
						continue;
					}
				}
			}
		}
	}
}

CRspSender::CRspSender(CachedConn& collection, MQ& Q, CEPoller& epoller)
: _conns(collection), _Q(Q), _epoller(epoller){}
CRspSender::~CRspSender(){}

void CRspSender::Run()
{
	unsigned long long count = 0;
	unsigned long long fail = 0;

	for(;;)
	{
		Message msg(ULONG_MAX);
		if (!_Q.Dequeue(msg))
		{
			continue;
		}
	

		int length;
		char* sData = msg.GetMessage(length);
		if (sData == NULL)
		{
			_conns.CloseConnect(msg.Flow());
			continue;
		}
		int fd = -1;
		int ret = _conns.Send(msg.Flow(), sData, length, fd);
		if (ret > 0)	
		{
			try
			{
				_epoller.modify(fd);
			}
			catch (exception& ex)
			{
				_conns.CloseConn(fd);
			}
			count++;
		}
		else
		{
			_conns.CloseConn(fd);
			fail++;
		}
	}
}



