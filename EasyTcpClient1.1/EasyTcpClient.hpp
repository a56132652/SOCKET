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

	//初始化Socket
	void initSocket()
	{
		//启动WinSock2.x环境
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);				//创建WINDOWS版本号
		WSADATA dat;
		WSAStartup(ver, &dat);					//启动网络环境,此函数调用了一个WINDOWS的静态链接库，因此需要加入静态链接库文件
#endif	
		 //1 创建SOCKET
		if (_sock != INVALID_SOCKET)
		{
			printf("<socket=%d>关闭旧连接...\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("错误，建立Socket失败...\n");
		}
		else {
			//printf("建立<Socket=%d>成功...\n",_sock);
		}
	}

	//连接服务器
	int Connect(const char* ip, unsigned short port)
	{
		// 2 连接服务器 connect
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
			printf("<Socket=%d>错误，连接服务器<%s:%d>失败...\n",_sock,ip,port);
		}
		else {
			_isConnect = true;
			//printf("<Socket=%d>连接服务器<%s:%d>成功...\n",_sock, ip, port);
		}
		return ret;
	}

	//关闭Socket
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

	//查询网络信息
	bool OnRun()
	{
		if (isRun())
		{
			fd_set fdRead;
			fd_set fdWrite;
			fd_set fdExc;
			//清空
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExc);
			//将描述符存入数组
			FD_SET(_sock, &fdRead);
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExc);

			timeval t = { 0,0 };
			int ret = select(_sock, &fdRead, 0, 0, &t);
			//printf("select ret = %d , count = %d\n", ret, _nCount++);
			if (ret < 0)
			{
				printf("<socket=%d>select任务结束1\n", _sock);
				Close();
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);

				if (-1 == RecvData(_sock))
				{
					printf("select任务结束2\n");
					Close();
					return false;
				}
			}
			return true;
		}
	}

	//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET && _isConnect;
	}

#ifndef RECV_BUFF_SIZE
	//缓冲区最小单元大小
#define RECV_BUFF_SIZE 10240 
#endif // !RECV_BUFF_SIZE

	//接收缓冲区
	//char _szRecv[RECV_BUFF_SIZE] = { };
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE ] = { };
	//消息缓冲区尾部指针
	int _lastPos = 0;

	//接收网络消息 处理粘包 拆分包
	int RecvData(SOCKET cSock)
	{
		char* szRecv = _szMsgBuf + _lastPos;
		int nLen = recv(cSock, szRecv, (RECV_BUFF_SIZE ) - _lastPos, 0);
		//printf("Len=%d\n", nLen);
		if (nLen <= 0)
		{
			printf("<socket=%d>与服务器断开连接，任务结束。\n", cSock);
			return -1;
		}
		//将收到的数据拷贝到消息缓冲区
		//memcpy(_szMsgBuf + _lastPos, _szRecv,nLen); 
		//消息缓冲区的数据尾部位置后移
		_lastPos += nLen;
		//判断消息缓冲区的数据长度是否大于消息头DataHeader长度
		while (_lastPos >= sizeof(DataHeader))
		{
			//这时就可以知道当前消息的长度
			DataHeader* header = (DataHeader*)_szMsgBuf;
			//判断消息缓冲区的数据长度是否大于消息长度
			if (_lastPos > header->dataLength)
			{
				//剩余未处理消息缓冲区数据的长度
				int nSize = _lastPos - header->dataLength;
				//处理网络消息
				OnNetMsg(header);
				//将消息缓冲区剩余未处理数据前移
				memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, _lastPos - header->dataLength);
				//消息缓冲区的数据尾部前移
				_lastPos = nSize;
			}
			else
			{
				//消息缓冲区剩余数据不够一条完整数据
				break;
			}
		}
		return 0;
	}

	//响应网络消息(不同的客户端会响应不同的消息，因此声明为虚函数)
	virtual void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			LoginResult* loginRet = (LoginResult*)header;
			//printf("<socket=%d>收到服务端消息：CMD_LOGIN_Result，数据长度 : %d\n", _sock, loginRet->dataLength);
		}
		break;
		case CMD_LOGINOUT_RESULT:
		{
			LoginoutResult* loginout = (LoginoutResult*)header;
			//printf("<socket=%d>收到服务端消息：CMD_LOGINOUT_Result，数据长度 : %d\n", _sock, loginout->dataLength);
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			NewUserJoin* userJoin = (NewUserJoin*)header;
			//printf("<socket=%d>收到服务端消息：CMD_NEW_USER_JOIN，数据长度 : %d\n", _sock, userJoin->dataLength);
		}
		break;
		case CMD_ERROR:
		{
			printf("<socket=%d>收到服务端消息：CMD_ERROR，数据长度 : %d\n", _sock, header->dataLength);
		}
		break;
		default:
		{
			printf("<socket=%d>收到未定义消息，数据长度 : %d\n", _sock, header->dataLength);
		}
		break;
		}
	}

	//发送数据
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