#ifndef _CELL_BUFFER_HPP_
#define	_CELL_BUFFER_HPP_

#include "Cell.hpp"

class CELLBuffer
{
public:
	CELLBuffer(int nSize = 8192)
	{
		_nSize = nSize;
		_pBuff = new char[_nSize];
	}
	~CELLBuffer()
	{
		if (_pBuff)
		{
			delete[] _pBuff;
			_pBuff = _pBuff;
		}
	}

	char* data()
	{
		return _pBuff;
	}

	bool push(const char* pData, int nLen)
	{
		if (_nLast + nLen <= _nSize)
		{
			//将要发送的数据拷贝到发送缓冲区尾部
			memcpy(_pBuff + _nLast, pData, nLen);
			//数据尾部位置移动
			_nLast += nLen;

			if (_nLast == SEND_BUFF_SIZE)
			{
				++_fullCount;
			}
			return true;
		}
		else {
			++_fullCount;
		}
		return false;
	}

	//
	void pop(int nLen)
	{
		int n = _nLast - nLen;
		if (n > 0)
		{
			memcpy(_pBuff, _pBuff + nLen, n);
		}
		_nLast = n;
		if (_fullCount > 0)
			--_fullCount;
	}
	//立即将发送缓冲区的数据发送给客户端
	int write2socket(SOCKET sockfd)
	{
		int ret = 0;
		//缓冲区有数据
		if (_nLast > 0 && INVALID_SOCKET != sockfd)
		{
			//发送数据
			ret = send(sockfd, _pBuff, _nLast, 0);
			//数据尾部位置 置0
			_nLast = 0;
			//
			_fullCount = 0;

		}
		return ret;
	}
	//
	int read4socket(SOCKET sockfd)
	{
		if (_nSize - _nLast > 0)
		{
			char* szRecv = _pBuff + _nLast;
			// 接收客户端数据
			int nLen = (int)recv(sockfd, szRecv, _nSize - _nLast, 0);
			//CELLLog::Info("Len=%d\n", nLen);
			if (nLen <= 0)
			{
				return nLen;
			}
			//消息缓冲区的数据尾部位置后移
			_nLast += nLen;
			return nLen;
		}
		return 0;
	}
	//
	bool hasMsg()
	{
		//判断消息缓冲区的数据长度大于消息头netmsg_DataHeader长度
		if (_nLast >= sizeof(DataHeader))
		{
			//这时就可以知道当前消息的长度
			DataHeader* header = (DataHeader*)_pBuff;
			//判断消息缓冲区的数据长度是否大于消息长度
			return _nLast >= header->dataLength;
		}
		return false;
	}
private:
	//第二缓冲区 发送缓冲区
	char* _pBuff = nullptr;
	//可以用链表或队列来管理缓冲数据块
	//list<char*> _pBuffList;
	//缓冲区的数据尾部位置，已有数据长度
	int _nLast = 0;
	//缓冲区总的空间大小，字节长度
	int _nSize = 0;
	//缓冲区写满次数计数
	int _fullCount = 0;
};
#endif // !_CELL_BUFFER_HPP_
