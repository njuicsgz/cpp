#include "sock.h"
#include "cached_conn.h"
#include "CSessionMgr.h"

using namespace std;
using namespace network;

static const size_t C_READ_BUFFER_SIZE = 4096;

//新进一个连接
void CachedConn::AddConn(int fd, unsigned flow)
{
	ml lock(_mutex);
	CProtocolInterface* pi = _pf.CreateProtocolCache();
	pi->_access = time(0);
	pi->_flow = flow;
	pi->_fd = fd;	
	_fd_attr[fd] = pi;
	_flow_2_fd[flow] = fd;
}

//关闭套接字
unsigned CachedConn::CloseConn(int fd)
{
	ml lock(_mutex);
	return Close(fd);
}

//断开连接
int CachedConn::CloseConnect(unsigned int flow)
{
	int fd=0;
	bool bClose=true;
	{
		ml lock(_mutex);
		map<unsigned, int>::iterator it = _flow_2_fd.find(flow);
		if (it == _flow_2_fd.end())
			return -1;
		
		int fd = it->second;
		iterator itr = _fd_attr.find(fd);

		CProtocolInterface* pi = itr->second;
		int nMsgLen;
		pi->_send_msg.GetMessage(nMsgLen);

		//还有数据没有发送
		if (pi->_send_offset>0&&pi->_send_offset <nMsgLen) 
		{
			pi->_delete_flag=1;
			bClose=false;
		}
	}
	if(bClose)
	{		
		CloseFlow(flow);
	}
	return fd;
}

//删除一个flow
int CachedConn::CloseFlow(unsigned flow)
{
	ml lock(_mutex);	
	map<unsigned, int>::iterator it = _flow_2_fd.find(flow);
	if (it == _flow_2_fd.end())
		return -1;
	
	int fd = it->second;
	_flow_2_fd.erase(it);
	
	iterator itr = _fd_attr.find(fd);
	assert(itr != _fd_attr.end());
	_pf.DestroyProtocolCache(itr->second);
	_fd_attr.erase(itr);

	CloseSession( flow );
		
	shutdown(fd, SHUT_RDWR);
	return close(fd);
}

//接收数据
int CachedConn::Recv(int fd,  unsigned& flow)
{
	ml lock(_mutex);
	try
	{
		if(fd < 0)
		{
			return 0;
		}
		map<int, CProtocolInterface*>::iterator it = _fd_attr.find(fd);
		if(it == _fd_attr.end())
		{
			return 0;
		}
		
		flow = it->second->_flow;
		it->second->_access = time(0);		

		//从socket读取数据
		char sBuffer[C_READ_BUFFER_SIZE];
		CSockAttacher sock(fd);
		int ret = sock.receive(sBuffer, C_READ_BUFFER_SIZE);
		if (ret <= 0)
		{
			return 0;	
		}
		it->second->_access = time(0);
		it->second->append(sBuffer, ret);		
		return ret;
	}
	catch (socket_error& ex)
	{
		if (ex._err_no != EAGAIN)
		{
			return 0;	
		}
		else
		{
			return 1;
		}
	}
}

//发送数据
int CachedConn::Send(unsigned flow, const char* sData, size_t iDataLen, int& fd)
{
	ml lock(_mutex);
	map<unsigned, int>::iterator it = _flow_2_fd.find(flow);
	if (it == _flow_2_fd.end())
	{
		return 0;	
	}
	fd = it->second;
	iterator itr = _fd_attr.find(fd);
	
	try
	{
		CSockAttacher sock(fd);
		int ret = sock.send(sData, iDataLen);
		
		if ((unsigned )ret < iDataLen)
		{
			itr->second->_send_msg.AttachMessage(sData + ret, iDataLen - ret);
			if (itr->second->_send_offset == -1)
				itr->second->_send_offset = 0;
		}

		itr->second->_access = time(0);
		return ret;
	}
	catch (socket_error& ex)
	{
		return 0;
	}
}

static bool BlockSend(CSocket& sock, big_fd_set& fdSet,const char* data, size_t len)
{
	int fd = sock.fd();
	unsigned sent = 0;

	for(int i = 0; i < 30; i++)
	{
		try
		{
			fd_set* fs = fdSet.get_fd_set();
			FD_SET(fd, fs);
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 10000;
			int ret = select(fd+1, NULL, fs, NULL, &tv);
			
			if (ret < 0)
			{
				return false;
			}
			else if (ret == 0)
			{
				continue;
			}
			else if (FD_ISSET(fd, fs) == 0)
			{
				return false;
			}
			else
			{
				fdSet.FD_UNSET(fd);
			}

			ret = sock.send(data + sent, len - sent);
			
			sent += ret;
			if (sent == len)
				return true;
		}

		catch (socket_error& ex)
		{
			if (ex._err_no != EAGAIN && ex._err_no != EWOULDBLOCK)
			{
				return false;
			}
		}
	}
	return false;
}

//发送数据
int CachedConn::Send(unsigned flow, big_fd_set& fd_set, const char* sData, size_t iDataLen)
{
	ml lock(_mutex);
	map<unsigned, int>::iterator it = _flow_2_fd.find(flow);
	if (it == _flow_2_fd.end())
	{
		return 0;
    }

	int fd = it->second;
	try
	{
		CSockAttacher sock(fd);
		int isent = sock.send(sData, iDataLen);
        if ((unsigned)isent < iDataLen)
		{
			if (!BlockSend(sock,fd_set,sData+isent, iDataLen-isent))
			{
				isent = -1;
			}
		}

		iterator itr = _fd_attr.find(fd);
		assert(itr != _fd_attr.end());
		itr->second->_access = time(0);
		return isent;
	}
	catch (socket_error& ex)
	{
		return 0;
	}
}

//发送数据
int CachedConn::Send(int fd, unsigned& flow)
{
	ml lock(_mutex);
	iterator it = _fd_attr.find(fd);
	if (it == _fd_attr.end())
	{
		return -1;
    }
	
	CProtocolInterface* pi = it->second;
	flow = pi->_flow;
	if (pi->_send_offset == -1)
	{
		return 0;
	}
	
	int total_len = 0;
	char* p = pi->_send_msg.GetMessage(total_len);
	int nNeedSendLen;
	if (pi->_send_offset >= total_len)
	{
		if(pi->_delete_flag)
		{
			//关闭FLOW
			return -1;			
		}
		else
		{
			return 0;
		}	
	}
	else
	{
		p += pi->_send_offset;
		nNeedSendLen=total_len-pi->_send_offset;
	}
	
	CSockAttacher sock(fd);
	try
	{
		int ret = sock.send(p, nNeedSendLen);
		if (ret >= nNeedSendLen)
		{
			pi->_send_msg.Clear();
			pi->_send_offset = -1;      		
			if(pi->_delete_flag)
			{
				//为了关闭FLOW
				return -1;				
			}
			else
			{
				return 0;
			}
		}
		else
		{
			pi->_send_offset += ret;
			pi->_access = time(0);
			return nNeedSendLen - ret;
		}
	}
	catch (socket_error& ex)
	{
		if (ex._err_no == EAGAIN)
		{
			pi->_access = time(0);
			return nNeedSendLen;
		}
		else
		{
			return -1;
		}
	}
}

//获取msg中数据
int CachedConn::GetMessage(int fd, Message& msg)
{
	ml lock(_mutex);
	
	iterator it = _fd_attr.find(fd);
	if (it == _fd_attr.end())
		return 0;
	
	int ret = it->second->GetMsg(msg);
	return ret;
}

int CachedConn::FD(unsigned flow)
{
	ml lock(_mutex);
	map<unsigned, int>::iterator it = _flow_2_fd.find(flow);
	if (it == _flow_2_fd.end())
		return -1;
	else
		return it->second;
}

//检查超时
void CachedConn::CheckTimeout(time_t access_deadline, deque<unsigned>& timeout_flow)
{
	timeout_flow.clear();
	ml lock(_mutex);
	
	for(iterator it = _fd_attr.begin(); it != _fd_attr.end(); it++)
	{
		if (it->second->_access < access_deadline)
		{
			timeout_flow.push_back(it->second->_flow);
		}
	}
}

//关闭fd
unsigned CachedConn::Close(int fd)
{
	unsigned flow;
	iterator it = _fd_attr.find(fd);
	if (it == _fd_attr.end())
		return 0;
	
	 flow= it->second->_flow;
	_pf.DestroyProtocolCache(it->second);
	_fd_attr.erase(it);
	
	map<unsigned, int>::iterator itr = _flow_2_fd.find(flow);
	assert(itr != _flow_2_fd.end());
	_flow_2_fd.erase(itr);

	CloseSession( flow );
	
	shutdown(fd, SHUT_RDWR);
	close(fd);
	return flow;
}



