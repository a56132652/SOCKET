#ifndef _CellClienthpp_
#define _CellClienthpp_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define FD_SETSIZE      2506
#include<windows.h>
#include<WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>

#define SOCKET int
#define INVALID_SOCKET (int)(~0)
#define SOCKET_ERROR (-1)
#endif

#include"Cell.hpp"
//客户端心跳检测死亡计时时间
#define CLIENT_HEART_DEAD_TIME 60000
//间隔指定时间后把发送缓冲区内缓冲的消息数据发送给客户端
#define CLIENT_SEND_BUFF_TIME 1000 
/********************************************************************************************************************************/
/**********************-------------------------CellClient(客户端数据类型)-----------------------******************************/
/********************************************************************************************************************************/
class CellClient
{
public:
	CellClient(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, RECV_BUFF_SIZE);
		_lastPos = 0;
		memset(_szSendBuf, 0, SEND_BUFF_SIZE);
		_lastSendPos = 0;

		resetDtHeart();
		resetDtSend();
	}

	SOCKET sockfd()
	{
		return _sockfd;
	}

	char* msgBuf()
	{
		return _szMsgBuf;
	}

	int getLastPos()
	{
		return _lastPos;
	}
	void setLastPos(int pos)
	{
		_lastPos = pos;
	}

	//立即发送数据
	int SendDataReal(DataHeader* header)
	{
		SendData(header);
		SendDataReal();
	}

	//立即将发送缓冲区的数据发送给客户端
	int SendDataReal()
	{
		int ret = SOCKET_ERROR;
		//缓冲区有数据
		if (_lastSendPos > 0 && SOCKET_ERROR != _sockfd)
		{
			//发送数据
			ret = send(_sockfd, _szSendBuf, _lastSendPos, 0);
			//数据尾部位置 置0
			_lastSendPos = 0;

			resetDtSend();
		}
		return ret;
	}

	//发送数据
	int SendData(DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		//要发送的数据长度
		int nSendLen = header->dataLength;
		//要发送的数据
		const char* pSendData = (const char*)header;

		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
			{
				//计算可拷贝的数据长度
				int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
				memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
				//剩余数据位置
				pSendData += nCopyLen;
				//剩余数据长度
				nSendLen -= nCopyLen;
				//发送数据
				ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
				//数据尾部位置 置0
				_lastSendPos = 0;
				//发送计时清零
				resetDtSend();
				//发送错误
				if (SOCKET_ERROR == ret)
				{
					return ret;
				}
			}
			else {
				//将要发送的数据拷贝到发送缓冲区尾部
				memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
				//数据尾部位置移动
				_lastSendPos += nSendLen;
				break;
			}
		}
		return ret;
	}

	void resetDtHeart()
	{
		_dtHeart = 0;
	}

	void resetDtSend()
	{
		_dtSend = 0;
	}

	//心跳检测
	bool checkHeart(time_t dt)
	{
		_dtHeart += dt;
		if (_dtHeart >= CLIENT_HEART_DEAD_TIME) {
			printf("checkHeart dead:s=%d,time=%d\n", _sockfd, _dtHeart);
			return true;
		}
		return false;
	}

	//定时发送消息检测
	bool checkSend(time_t dt)
	{
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME) {
			//printf("checkSend:s=%d,time=%d\n", _sockfd, _dtSend);
			//立即发送缓冲区数据
			SendDataReal();
			//重置发送计时
			resetDtSend();
			return true;
		}
		return false;
	}

private:
	SOCKET _sockfd;
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE] = { };
	//消息缓冲区尾部指针
	int _lastPos = 0;

	//第二缓冲区 发送缓冲区
	char _szSendBuf[SEND_BUFF_SIZE] = { };
	//消息缓冲区尾部指针
	int _lastSendPos = 0;
	//心跳死亡计时
	time_t _dtHeart;
	//上次发送消息数据的时间
	time_t _dtSend;
};

#endif // CellClient


