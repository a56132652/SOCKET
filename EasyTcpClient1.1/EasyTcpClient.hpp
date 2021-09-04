#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS	
	#define _CRT_SECURE_NO_WARNINGS	
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
#include<stdio.h>
#include "MessageHeader.hpp"

class EasyTcpClient
{
	SOCKET _sock;
	bool _isConnect;
public:
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
		_isConnect = false;
	}
	virtual ~EasyTcpClient()
	{
		Close();
	}

	//��ʼ��Socket
	void initSocket()
	{
		//����WinSock2.x����
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);				//����WINDOWS�汾��
		WSADATA dat;
		WSAStartup(ver, &dat);					//�������绷��,�˺���������һ��WINDOWS�ľ�̬���ӿ⣬�����Ҫ���뾲̬���ӿ��ļ�
#endif	
		 //1 ����SOCKET
		if (_sock != INVALID_SOCKET)
		{
			printf("<socket=%d>�رվ�����...\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("���󣬽���Socketʧ��...\n");
		}
		else {
			//printf("����<Socket=%d>�ɹ�...\n",_sock);
		}
	}

	//���ӷ�����
	int Connect(const char* ip, unsigned short port)
	{
		// 2 ���ӷ����� connect
		if (_sock == INVALID_SOCKET)
		{
			initSocket();
		}
		sockaddr_in _sin = { };
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (ret == SOCKET_ERROR)
		{
			printf("<Socket=%d>�������ӷ�����<%s:%d>ʧ��...\n",_sock,ip,port);
		}
		else {
			_isConnect = true;
			//printf("<Socket=%d>���ӷ�����<%s:%d>�ɹ�...\n",_sock, ip, port);
		}
		return ret;
	}

	//�ر�Socket
	void Close()
	{

		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			closesocket(_sock);
			WSACleanup();
#else
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
		_isConnect = false;
	}

	//��ѯ������Ϣ
	bool OnRun()
	{
		if (isRun())
		{
			fd_set fdRead;
			fd_set fdWrite;
			fd_set fdExc;
			//���
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExc);
			//����������������
			FD_SET(_sock, &fdRead);
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExc);

			timeval t = { 0,0 };
			int ret = select(_sock, &fdRead, 0, 0, &t);
			//printf("select ret = %d , count = %d\n", ret, _nCount++);
			if (ret < 0)
			{
				printf("<socket=%d>select�������1\n", _sock);
				Close();
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);

				if (-1 == RecvData(_sock))
				{
					printf("select�������2\n");
					Close();
					return false;
				}
			}
			return true;
		}
	}

	//�Ƿ�����
	bool isRun()
	{
		return _sock != INVALID_SOCKET && _isConnect;
	}

#ifndef RECV_BUFF_SIZE
	//��������С��Ԫ��С
#define RECV_BUFF_SIZE 10240 
#endif // !RECV_BUFF_SIZE

	//���ջ�����
	//char _szRecv[RECV_BUFF_SIZE] = { };
	//�ڶ������� ��Ϣ������
	char _szMsgBuf[RECV_BUFF_SIZE ] = { };
	//��Ϣ������β��ָ��
	int _lastPos = 0;

	//����������Ϣ ����ճ�� ��ְ�
	int RecvData(SOCKET cSock)
	{
		char* szRecv = _szMsgBuf + _lastPos;
		int nLen = recv(cSock, szRecv, (RECV_BUFF_SIZE ) - _lastPos, 0);
		//printf("Len=%d\n", nLen);
		if (nLen <= 0)
		{
			printf("<socket=%d>��������Ͽ����ӣ����������\n", cSock);
			return -1;
		}
		//���յ������ݿ�������Ϣ������
		//memcpy(_szMsgBuf + _lastPos, _szRecv,nLen); 
		//��Ϣ������������β��λ�ú���
		_lastPos += nLen;
		//�ж���Ϣ�����������ݳ����Ƿ������ϢͷDataHeader����
		while (_lastPos >= sizeof(DataHeader))
		{
			//��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
			DataHeader* header = (DataHeader*)_szMsgBuf;
			//�ж���Ϣ�����������ݳ����Ƿ������Ϣ����
			if (_lastPos > header->dataLength)
			{
				//ʣ��δ������Ϣ���������ݵĳ���
				int nSize = _lastPos - header->dataLength;
				//����������Ϣ
				OnNetMsg(header);
				//����Ϣ������ʣ��δ��������ǰ��
				memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, _lastPos - header->dataLength);
				//��Ϣ������������β��ǰ��
				_lastPos = nSize;
			}
			else
			{
				//��Ϣ������ʣ�����ݲ���һ����������
				break;
			}
		}
		return 0;
	}

	//��Ӧ������Ϣ(��ͬ�Ŀͻ��˻���Ӧ��ͬ����Ϣ���������Ϊ�麯��)
	virtual void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			LoginResult* loginRet = (LoginResult*)header;
			//printf("<socket=%d>�յ��������Ϣ��CMD_LOGIN_Result�����ݳ��� : %d\n", _sock, loginRet->dataLength);
		}
		break;
		case CMD_LOGINOUT_RESULT:
		{
			LoginoutResult* loginout = (LoginoutResult*)header;
			//printf("<socket=%d>�յ��������Ϣ��CMD_LOGINOUT_Result�����ݳ��� : %d\n", _sock, loginout->dataLength);
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			NewUserJoin* userJoin = (NewUserJoin*)header;
			//printf("<socket=%d>�յ��������Ϣ��CMD_NEW_USER_JOIN�����ݳ��� : %d\n", _sock, userJoin->dataLength);
		}
		break;
		case CMD_ERROR:
		{
			printf("<socket=%d>�յ��������Ϣ��CMD_ERROR�����ݳ��� : %d\n", _sock, header->dataLength);
		}
		break;
		default:
		{
			printf("<socket=%d>�յ�δ������Ϣ�����ݳ��� : %d\n", _sock, header->dataLength);
		}
		break;
		}
	}

	//��������
	int SendData(DataHeader* header,int nLen)
	{
		int ret = SOCKET_ERROR;
		if (isRun() && header)
		{
			ret = send(_sock, (const char*)header, nLen, 0);
			if (SOCKET_ERROR == ret)
			{
				Close();
			}
		}
		return ret;
	}
};

#endif