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
#define CLIENT_HEART_DEAD_TIME 5000
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

	//心跳检测
	bool checkHeart(time_t dt)
	{
		_dtHeart += dt;
		if (_dtHeart >= CLIENT_HEART_DEAD_TIME) {
			printf("socket<%d> dead,LiveTime:%d\n", _sockfd, dt);
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
};

#endif // CellClient


