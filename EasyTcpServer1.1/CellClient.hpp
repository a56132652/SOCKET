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
//�ͻ����������������ʱʱ��
#define CLIENT_HEART_DEAD_TIME 5000
/********************************************************************************************************************************/
/**********************-------------------------CellClient(�ͻ�����������)-----------------------******************************/
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
	//��������
	int SendData(DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		//Ҫ���͵����ݳ���
		int nSendLen = header->dataLength;
		//Ҫ���͵�����
		const char* pSendData = (const char*)header;

		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
			{
				//����ɿ��������ݳ���
				int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
				memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
				//ʣ������λ��
				pSendData += nCopyLen;
				//ʣ�����ݳ���
				nSendLen -= nCopyLen;
				//��������
				ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
				//����β��λ�� ��0
				_lastSendPos = 0;

			}
			else {
				//��Ҫ���͵����ݿ��������ͻ�����β��
				memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
				//����β��λ���ƶ�
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

	//�������
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
	//�ڶ������� ��Ϣ������
	char _szMsgBuf[RECV_BUFF_SIZE] = { };
	//��Ϣ������β��ָ��
	int _lastPos = 0;

	//�ڶ������� ���ͻ�����
	char _szSendBuf[SEND_BUFF_SIZE] = { };
	//��Ϣ������β��ָ��
	int _lastSendPos = 0;
	//����������ʱ
	time_t _dtHeart;
};

#endif // CellClient


