#ifndef _EasyTcpServer_hpp_
#define	_EasyTcpServer_hpp_


#include<stdio.h>
#include<thread>
#include<vector>
#include<map>
#include<mutex>
#include<functional>
#include<atomic>

#include"Cell.hpp"
#include"INetEvent.hpp"
#include "CellServer.hpp"
#include "CellClient.hpp"
/********************************************************************************************************************************/
/*****************************-------------------------EasyTcpServer-----------------------**************************************/
/********************************************************************************************************************************/

class EasyTcpServer : public INetEvent
{
private:
	//
	CELLThread _thread;
	std::vector<CellServer*> _cellServers;
	//ÿ����Ϣ��ʱ
	CELLTimestamp _tTime;
	//
	SOCKET _sock;
protected:
	//�յ���Ϣ����
	std::atomic_int _msgCount;
	//�ͻ��˽������
	std::atomic_int _clientCount;
	//recv����
	std::atomic_int _recvCount;
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
		_msgCount = 0;
		_clientCount = 0;
		_recvCount = 0;
		
		
	}

	virtual ~EasyTcpServer()
	{
		Close();
	}

	//��ʼ��socket
	SOCKET InitSocket()
	{
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);				//����WINDOWS�汾��
		WSADATA dat;
		WSAStartup(ver, &dat);					//�������绷��,�˺���������һ��WINDOWS�ľ�̬���ӿ⣬�����Ҫ���뾲̬���ӿ��ļ�
#endif

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("���󣬽���socketʧ��...\n");
		}
		else {
			printf("����<socket=%d>�ɹ�...\n", (int)_sock);
		}
		return _sock;
	}

	//��Ip�Ͷ˿ں�
	int Bind(const char* ip,unsigned short port)
	{
		//if (_sock == INVALID_SOCKET)
		//{
		//	InitSocket();
		//}
		sockaddr_in _sin = { };
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		if (ip == NULL)
		{
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;		
		}
		else {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}

#else
		if (ip == NULL)
		{
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
		else {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret)
		{
			printf("���󣬰�����˿�<%d>ʧ��...\n",port);
		}
		else {
			printf("�󶨶˿�<%d>�ɹ�...\n", port);
		}
		return ret;
	}

	//�����˿ں�
	int Listen(int n)
	{
		// 3 listen ��������˿�
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("socket=<%d>���󣬼�������˿�ʧ��...\n", (int)_sock);
		}
		else {
			printf("socket=<%d>��������˿ڳɹ�...\n", (int)_sock);
		}
		return ret;
	}
	//���ܿͻ�������
	SOCKET Accept()
	{
		sockaddr_in _clientAddr = { };
		int nAddrLen = sizeof(_clientAddr);
		SOCKET cSock = INVALID_SOCKET;

		// accept �ȴ����տͻ�������
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&_clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&_clientAddr, (socklen_t*)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSock)
		{
			printf("socket=<%d>���󣬽��յ���Ч�ͻ���SOCKET...\n", (int)_sock);
		}
		else {
			//���¿ͻ��˷�����ͻ��������ٵ�cellServer
			addClientToCellServer(new CellClient(cSock));
			//��ȡIP��ַ	inet_ntoa(_clientAddr.sin_addr)
		}
		return cSock;
	}

	void addClientToCellServer(CellClient* pClient)
	{
		//���ҿͻ��������ٵ�CellServer��Ϣ�������
		auto pMinServer = _cellServers[0];
		for (auto pCellServer : _cellServers)
		{
			if (pMinServer->getClientCount() > pCellServer->getClientCount())
			{
				pMinServer = pCellServer;
			}	
		}
		pMinServer->addClient(pClient); 
	}

	void Start(int nCellServer)
	{
		for (int i = 0; i < nCellServer; i++)
		{
			auto ser = new CellServer(i+1);
			_cellServers.push_back(ser);
			//ע�������¼����ն���
			ser->setEventObj(this);
			//������Ϣ�����߳�
			ser->Start();
		}
		_thread.Start(nullptr,
			[this](CELLThread* pThread) {onRun(pThread); });
	}

	//�ر�socket
	void Close()
	{
		printf("EasyTcpServer.Close begin\n");
		_thread.Close();
		if (_sock != INVALID_SOCKET)
		{
			for (auto s : _cellServers)
			{
				delete s;
			}
			_cellServers.clear();
#ifdef _WIN32
			//�ر��׽���socket
			closesocket(_sock);
			WSACleanup();
#else
			close(_sock);
#endif
			_sock == INVALID_SOCKET;
		}
		printf("EasyTcpServer.Close end\n");
	}

	//���㲢���ÿ���յ���������Ϣ
	void time4msg()
	{
		
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			printf("thread<%d>,time<%lf>,socket<%d>,clients<%d>,recv<%d>,msg<%d>\n",(int)_cellServers.size(),t1, _sock, (int)_clientCount,(int)(_recvCount/ t1), (int)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}
	virtual void OnNetJoin(CellClient* pClient)
	{
		_clientCount++;
		//printf("client<%d> join\n", pClient->sockfd());
	}
	virtual void OnNetLeave(CellClient* pClient)
	{
		_clientCount--;
		//printf("client<%d> leave\n", pClient->sockfd());
	}
	virtual void OnNetMsg(CellServer* pCellServer, CellClient* pClient,DataHeader* header)
	{
		_msgCount++;
	}
	virtual void OnNetRecv(CellClient* pClient)
	{
		_recvCount++; 
	}
private:
	//����������Ϣ
	void onRun(CELLThread* pThread)
	{
		while(pThread->isRun())
		{
			time4msg();
			fd_set fdRead;
			//���
			FD_ZERO(&fdRead);
			//����������������
			FD_SET(_sock, &fdRead);

			timeval t = { 0,1 };
			int ret = select(_sock + 1, &fdRead, 0, 0, &t);
			if (ret < 0)
			{
				printf("EasyTcpServer.OnRun Accept Select Exit.\n");
				pThread->Exit();
				break;
			}
			//�ж�������(socket)�Ƿ��ڼ�����
			if (FD_ISSET(_sock, &fdRead))
			{
				//�������ж�Ӧ���������ļ���ֵ��0����δ�������������
				FD_CLR(_sock, &fdRead);
				Accept();
			}
		}
	}
};

#endif // _EasyTcpServer_hpp_
