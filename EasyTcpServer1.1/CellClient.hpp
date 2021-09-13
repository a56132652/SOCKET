#ifndef _CELLClienthpp_
#define _CELLClienthpp_

#include"CELL.hpp"

//�ͻ����������������ʱʱ��
#define CLIENT_HEART_DEAD_TIME 60000
//�ڼ��ָ��ʱ���
//�ѷ��ͻ������ڻ������Ϣ���ݷ��͸��ͻ���
#define CLIENT_SEND_BUFF_TIME 200 
//�ͻ�����������
class CELLClient
{
public:
	int id = -1;
	//����serverID
	int serverID = -1;
public:
	CELLClient(SOCKET sockfd = INVALID_SOCKET)
	{
		static int n = 1;
		id = n++;
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, RECV_BUFF_SIZE);
		_lastPos = 0;
		memset(_szSendBuf, 0, SEND_BUFF_SIZE);
		_lastSendPos = 0;

		resetDtHeart();
		resetDtSend();
	}

	~CELLClient()
	{
		printf("s=%d CELLClient%d.Close 1\n", serverID, id);
		if (INVALID_SOCKET != _sockfd)
		{
#ifdef _WIN32
			closesocket(_sockfd);
#else
			close(_sockfd);
#endif
			_sockfd = INVALID_SOCKET;
		}
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
	void SendDataReal(DataHeader* header)
	{
		SendData(header);
		SendDataReal();
	}

	//���������ͻ����������ݷ��͸��ͻ���
	int SendDataReal()
	{
		int ret = 0;
		//������������
		if (_lastSendPos > 0 && INVALID_SOCKET != _sockfd)
		{
			//��������
			ret = send(_sockfd, _szSendBuf, _lastSendPos, 0);
			//����β��λ�� ��0
			_lastSendPos = 0;
			//
			_sendBuffFullCount = 0;
			//
			resetDtSend();
		}
		return ret;
	}

	//��������С����ҵ������Ĳ�����仯����
	//��������
	int SendData(DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		//Ҫ���͵����ݳ���
		int nSendLen = header->dataLength;
		//Ҫ���͵�����
		const char* pSendData = (const char*)header;

		if (_lastSendPos + nSendLen <= SEND_BUFF_SIZE)
		{
			//��Ҫ���͵����ݿ��������ͻ�����β��
			memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
			//����β��λ���ƶ�
			_lastSendPos += nSendLen;

			if (_lastSendPos == SEND_BUFF_SIZE)
			{
				_sendBuffFullCount++;
			}
			return nSendLen;
		}
		else {
			_sendBuffFullCount++;
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
	int _lastSendPos;
	//����������ʱ
	time_t _dtHeart;
	//�ϴη�����Ϣ���ݵ�ʱ��
	time_t _dtSend;
	//���ͻ�����д������
	int _sendBuffFullCount = 0;
};

#endif // CELLClient


