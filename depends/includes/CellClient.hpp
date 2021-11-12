#ifndef _CELLClienthpp_
#define _CELLClienthpp_

#include"CELL.hpp"
#include"CELLBuffer.hpp"

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
	CELLClient(SOCKET sockfd = INVALID_SOCKET) : 
		_sendBuff(SEND_BUFF_SIZE),
		_recvBuff(RECV_BUFF_SIZE)
	{
		static int n = 1;
		id = n++;
		_sockfd = sockfd;

		resetDtHeart();
		resetDtSend();
	}

	~CELLClient()
	{
		CELLLog_Debug("s=%d CELLClient%d.Close 1", serverID, id);
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

	//������������
	void SendDataReal(DataHeader* header)
	{
		SendData(header);
		SendDataReal();
	}
	//��������
	int RecvData()
	{
		return _recvBuff.read4socket(_sockfd);
	}
	//
	bool hasMsg()
	{
		return _recvBuff.hasMsg();
	}
	//
	DataHeader* front_msg()
	{
		return (DataHeader*)_recvBuff.data();
	}

	bool needWrite()
	{
		return _sendBuff.needWrite();
	}

	void pop_front_msg()
	{
		if (hasMsg())
			_recvBuff.pop(front_msg()->dataLength);
		
	}
	//���������ͻ����������ݷ��͸��ͻ���
	int SendDataReal()
	{
		resetDtSend();
		return _sendBuff.write2socket(_sockfd);
	}

	//��������С����ҵ������Ĳ�����仯����
	//��������
	int SendData(DataHeader* header)
	{
		return SendData((const char*) header, header->dataLength);
	}
	int SendData(const char* pData, int Len)
	{
		if (_sendBuff.push(pData, Len))
		{
			return Len;
		}

		return SOCKET_ERROR;
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
			CELLLog_Info("checkHeart dead:s=%d,time=%d", _sockfd, _dtHeart);
			return true;
		}
		return false;
	}
	 
	//��ʱ������Ϣ���
	bool checkSend(time_t dt)
	{
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME) {
			//CELLLog_Info("checkSend:s=%d,time=%d", _sockfd, _dtSend);
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
	//�ڶ������� ������Ϣ������
	CELLBuffer _recvBuff;
	//��Ϣ������β��ָ��
	int _lastPos = 0;
	//�ڶ������� ���ͻ�����
	CELLBuffer _sendBuff;  
	//����������ʱ
	time_t _dtHeart;
	//�ϴη�����Ϣ���ݵ�ʱ��
	time_t _dtSend;
	//���ͻ�����д������
	int _sendBuffFullCount = 0;
};

#endif // CELLClient


