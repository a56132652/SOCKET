#ifndef _EasyTcpServer_hpp_
#define	_EasyTcpServer_hpp_


#include"CELL.hpp"
#include"CELLClient.hpp"
#include"CELLServer.hpp"
#include"INetEvent.hpp"
#include"CELLNetWork.hpp"

#include<thread>
#include<mutex>
#include<atomic>

class EasyTcpServer : public INetEvent
{
private:
	//
	CELLThread _thread;
	std::vector<CELLServer*> _CELLServers;
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
		CELLNetWork::Init();
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			CELLLog_Info("���󣬽���socketʧ��...\n");
		}
		else {
			CELLLog_Info("����<socket=%d>�ɹ�...\n", (int)_sock);
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
			CELLLog_Info("���󣬰�����˿�<%d>ʧ��...\n",port);
		}
		else {
			CELLLog_Info("�󶨶˿�<%d>�ɹ�...\n", port);
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
			CELLLog_Info("socket=<%d>���󣬼�������˿�ʧ��...\n", (int)_sock);
		}
		else {
			CELLLog_Info("socket=<%d>��������˿ڳɹ�...\n", (int)_sock);
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
			CELLLog_Info("socket=<%d>���󣬽��յ���Ч�ͻ���SOCKET...\n", (int)_sock);
		}
		else {
			//���¿ͻ��˷�����ͻ��������ٵ�CELLServer
			addClientToCELLServer(new CELLClient(cSock));
			//��ȡIP��ַ	inet_ntoa(_clientAddr.sin_addr)
		}
		return cSock;
	}

	void addClientToCELLServer(CELLClient* pClient)
	{
		//���ҿͻ��������ٵ�CELLServer��Ϣ�������
		auto pMinServer = _CELLServers[0];
		for (auto pCELLServer : _CELLServers)
		{
			if (pMinServer->getClientCount() > pCELLServer->getClientCount())
			{
				pMinServer = pCELLServer;
			}	
		}
		pMinServer->addClient(pClient); 
	}

	void Start(int nCELLServer)
	{
		for (int i = 0; i < nCELLServer; i++)
		{
			auto ser = new CELLServer(i+1);
			_CELLServers.push_back(ser);
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
		CELLLog_Info("EasyTcpServer.Close begin\n");
		_thread.Close();
		if (_sock != INVALID_SOCKET)
		{
			for (auto s : _CELLServers)
			{
				delete s;
			}
			_CELLServers.clear();
#ifdef _WIN32
			//�ر��׽���socket
			closesocket(_sock);
#else
			close(_sock);
#endif
			_sock == INVALID_SOCKET;
		} 
		CELLLog_Info("EasyTcpServer.Close end\n");
	}

	//���㲢���ÿ���յ���������Ϣ
	void time4msg()
	{
		
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			CELLLog_Info("thread<%d>,time<%lf>,socket<%d>,clients<%d>,recv<%d>,msg<%d>\n",(int)_CELLServers.size(),t1, _sock, (int)_clientCount,(int)(_recvCount/ t1), (int)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}
	virtual void OnNetJoin(CELLClient* pClient)
	{
		_clientCount++;
		//CELLLog_Info("client<%d> join\n", pClient->sockfd());
	}
	virtual void OnNetLeave(CELLClient* pClient)
	{
		_clientCount--;
		//CELLLog_Info("client<%d> leave\n", pClient->sockfd());
	}
	virtual void OnNetMsg(CELLServer* pCELLServer, CELLClient* pClient,DataHeader* header)
	{
		_msgCount++;
	}
	virtual void OnNetRecv(CELLClient* pClient)
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
				CELLLog_Info("EasyTcpServer.OnRun Accept Select Exit.\n");
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
