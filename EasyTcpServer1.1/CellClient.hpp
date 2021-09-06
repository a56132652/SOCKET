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
#define CLIENT_HEART_DEAD_TIME 60000
//���ָ��ʱ���ѷ��ͻ������ڻ������Ϣ���ݷ��͸��ͻ���
#define CLIENT_SEND_BUFF_TIME 1000 
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

	//������������
	int SendDataReal(DataHeader* header)
	{
		SendData(header);
		SendDataReal();
	}

	//���������ͻ����������ݷ��͸��ͻ���
	int SendDataReal()
	{
		int ret = SOCKET_ERROR;
		//������������
		if (_lastSendPos > 0 && SOCKET_ERROR != _sockfd)
		{
			//��������
			ret = send(_sockfd, _szSendBuf, _lastSendPos, 0);
			//����β��λ�� ��0
			_lastSendPos = 0;

			resetDtSend();
		}
		return ret;
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
				//���ͼ�ʱ����
				resetDtSend();
				//���ʹ���
				if (SOCKET_ERROR == ret)
				{
					return ret;
				}
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

	void resetDtSend()
	{
		_dtSend = 0;
	}

	//�������
	bool checkHeart(time_t dt)
	{
		_dtHeart += dt;
		if (_dtHeart >= CLIENT_HEART_DEAD_TIME) {
			printf("checkHeart dead:s=%d,time=%d\n", _sockfd, _dtHeart);
			return true;
		}
		return false;
	}

	//��ʱ������Ϣ���
	bool checkSend(time_t dt)
	{
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME) {
			//printf("checkSend:s=%d,time=%d\n", _sockfd, _dtSend);
			//�������ͻ���������
			SendDataReal();
			//���÷��ͼ�ʱ
			resetDtSend();
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
	//�ϴη�����Ϣ���ݵ�ʱ��
	time_t _dtSend;
};

#endif // CellClient


