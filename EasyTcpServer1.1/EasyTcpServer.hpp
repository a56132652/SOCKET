#ifndef _EasyTcpServer_hpp_
#define	_EasyTcpServer_hpp_

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

#include<stdio.h>
#include<thread>
#include<vector>
#include<map>
#include<mutex>
#include<functional>
#include<atomic>
#include<memory>
#include"MessageHeader.hpp"
#include"CELLTimestamp.hpp"
#include"CELLTask.hpp"

#ifndef RECV_BUFF_SIZE
//��������С��Ԫ��С
#define RECV_BUFF_SIZE 10240 * 5
#define SEND_BUFF_SIZE 10240 * 5

#endif // !RECV_BUFF_SIZE
//Ԥ����
class CellServer;
class ClientSocket;
class CellSendMsgToClientTask;

typedef std::shared_ptr<LoginResult> LoginResultPtr;
typedef std::shared_ptr<DataHeader> DataHeaderPtr;
typedef	std::shared_ptr<CellSendMsgToClientTask> CellSendMsgToClientTaskPtr;
typedef std::shared_ptr<ClientSocket> ClientSocketPtr;
typedef std::shared_ptr<CellServer> CellServerPtr;

/********************************************************************************************************************************/
/**********************-------------------------ClientSocket(�ͻ�����������)-----------------------******************************/
/********************************************************************************************************************************/

class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, RECV_BUFF_SIZE);
		_lastPos = 0;
		memset(_szSendBuf, 0, SEND_BUFF_SIZE);
		_lastSendPos = 0;
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
	int SendData(DataHeaderPtr& header)
	{
		int ret = SOCKET_ERROR;
		//Ҫ���͵����ݳ���
		int nSendLen = header->dataLength;
		//Ҫ���͵�����
		const char* pSendData = (const char*)header.get();

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
};

/********************************************************************************************************************************/
/************************-------------------------INetEvent(�����¼��ӿ�)-------------------------*******************************/
/********************************************************************************************************************************/
class INetEvent
{
public:
	//���麯��
	//�ͻ��˼����¼�
	virtual void OnNetJoin(ClientSocketPtr& pClient) = 0;
	//�ͻ����뿪�¼�
	virtual void OnNetLeave(ClientSocketPtr& pClient) = 0;
	//�ͻ�����Ϣ�¼�
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocketPtr& pClient,DataHeader* header) = 0;
	//�ͻ���recv�¼�
	virtual void OnNetRecv(ClientSocketPtr& pClient) = 0;
};


/********************************************************************************************************************************/
/*********************---------------------CellSendMsgToClientTask(������Ϣ��������)--------------------*************************/
/********************************************************************************************************************************/
class CellSendMsgToClientTask : public CellTask
{
public:
	CellSendMsgToClientTask(ClientSocketPtr& pclient,DataHeaderPtr& header)
	{
		_pClient = pclient;
		_pHeader = header;
	}

	~CellSendMsgToClientTask()
	{

	}

	virtual void doTask()
	{
		_pClient->SendData(_pHeader);
	}
private:
	ClientSocketPtr _pClient;
	DataHeaderPtr _pHeader;
};

/********************************************************************************************************************************/
/*****************************---------------------CellServer(��Ϣ������)--------------------************************************/
/********************************************************************************************************************************/

class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pNetEvent = nullptr;
	}
	~CellServer()
	{
		Close();
		_sock = INVALID_SOCKET;
	}

	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;  
	}
	//�ر�socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (auto iter : _clients)
			{
				closesocket(iter.second->sockfd());
			}
			// 8 �ر��׽���closesocket
			closesocket(_sock);

			//-----------------
			//���Windows socket����
			WSACleanup();							//�ر�Socket���绷��
#else
			for (auto iter : _clients)
			{
				closesocket(iter.second->sockfd());
				delete iter.second;
			}
			close(_sock);
#endif
			_clients.clear();
		}
	}

	//�Ƿ�����
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	//����������Ϣ
	//int _nCount = 0;
	//���ݿͻ�socket fd_set
	fd_set _fdRead_back;
	//�ͻ��б��Ƿ�仯
	bool _clients_change;
	SOCKET _maxSock;
	bool onRun()
	{
		_clients_change = true;
		while(isRun())
		{
			if (_clientsBuff.size() > 0)
			{
				//�ӻ��������ȡ���ͻ�����
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
				}
				_clientsBuff.clear();
				_clients_change = true;
			}
			//���û����Ҫ����Ŀͻ��ˣ�������
			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			fd_set fdRead;
			//���
			FD_ZERO(&fdRead);
			if (_clients_change)
			{
				_clients_change = false;
				//�����������뼯��
				_maxSock = _clients.begin()->second->sockfd();
				//���¼���Ŀͻ��˼���fdRead����
				for (auto iter : _clients)
				{
					FD_SET(iter.second->sockfd(), &fdRead);
					if (_maxSock < iter.second->sockfd()) {
						_maxSock = iter.second->sockfd();
					}
				}
				memcpy(&_fdRead_back, &fdRead, sizeof(fd_set));
			}
			else {
				memcpy(&fdRead, &_fdRead_back, sizeof(fd_set));
			}
			int ret = select(_maxSock + 1, &fdRead,0, 0, nullptr);
			if (ret < 0)
			{
				printf("select���������\n");
				Close();
				return false;
			}

#ifdef _WIN32
			for (int n = 0; n < fdRead.fd_count; n++)
			{
				auto iter = _clients.find(fdRead.fd_array[n]);
				if (iter != _clients.end())
				{
					if (RecvData(iter->second) == -1)
					{
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(iter->second);
						_clients_change = true;
						_clients.erase(iter->first);
					}
				}
				else {
					printf("error.iter != _clients.end()\n");
				}
			}
#else
			std::vector<ClientSocketPtr> temp;
			for (auto iter : _clients)
			{
				if (FD_ISSET(iter.second->sockfd(), &fdRead))
				{
					if (RecvData(iter.second) == -1)
					{
						_clients_change = false;
						temp.push_back(iter.second);
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(iter.second);
					}
				}
			}
			for (auto pClient : temp)
			{
				_clients.erase(pClient->sockfd());
			}
#endif	

		}
	}

	//�������� ����ճ�� ��ְ�
	int RecvData(ClientSocketPtr& pClient)
	{
		char* szRecv = pClient->msgBuf() + pClient->getLastPos();
		// 5 ���տͻ�������
		int nLen = (int)recv(pClient->sockfd(), szRecv, (RECV_BUFF_SIZE) - pClient->getLastPos() , 0);
		_pNetEvent->OnNetRecv(pClient);
		//printf("Len=%d\n", nLen);
		if (nLen <= 0)
		{
			//printf("�ͻ���<Socket:%d>���˳������������\n", pClient->sockfd());
			return -1;
		}
		//���յ������ݿ�������Ϣ������
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		//��Ϣ������������β��λ�ú���
		pClient->setLastPos(pClient->getLastPos() + nLen);
		//�ж���Ϣ�����������ݳ����Ƿ������ϢͷDataHeader����
		while (pClient->getLastPos() >= sizeof(DataHeader))
		{
			//��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			//�ж���Ϣ�����������ݳ����Ƿ������Ϣ����
			if (pClient->getLastPos() > header->dataLength)
			{
				//ʣ��δ������Ϣ���������ݵĳ���
				int nSize = pClient->getLastPos() - header->dataLength;
				//����������Ϣ
				OnNetMsg(pClient, header);
				//����Ϣ������ʣ��δ��������ǰ��
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, pClient->getLastPos() - header->dataLength);
				//��Ϣ������������β��ǰ��
				pClient->setLastPos(nSize);
			}
			else
			{
				//��Ϣ������ʣ�����ݲ���һ����������
				break;
			}
		}
		return 0;
	}

	//��Ӧ������Ϣ
	virtual void OnNetMsg(ClientSocketPtr& pClient, DataHeader* header)
	{
		_pNetEvent->OnNetMsg(this,pClient, header);
	}

	void addClient(ClientSocketPtr& pClient)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	void Start()
	{
		_thread = std::thread(std::mem_fn(&CellServer::onRun), this);
		_taskServer.Start();
	}

	size_t getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}

	void addSendTask(ClientSocketPtr& pClient, DataHeaderPtr& header)
	{
		CellSendMsgToClientTaskPtr task = std::make_shared<CellSendMsgToClientTask>(pClient, header);
		_taskServer.addTask((CellTaskPtr&)task);
	}
private:
	SOCKET _sock;
	//��ʽ�ͻ�����
	std::map<SOCKET, ClientSocketPtr> _clients;
	//����ͻ�����
	std::vector<ClientSocketPtr> _clientsBuff;
	//������е���
	std::mutex _mutex;
	std::thread _thread;
	//�����¼�����
	INetEvent* _pNetEvent;
	//
	CellTaskServer _taskServer;
};

/********************************************************************************************************************************/
/*****************************-------------------------EasyTcpServer-----------------------**************************************/
/********************************************************************************************************************************/


class EasyTcpServer : public INetEvent
{
private:
	SOCKET _sock;
	std::vector<CellServerPtr> _cellServers;
	//ÿ����Ϣ��ʱ
	CELLTimestamp _tTime;
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
			ClientSocketPtr pClient = std::make_shared<ClientSocket>(cSock);
			addClientToCellServer(pClient);
			//��ȡIP��ַ	inet_ntoa(_clientAddr.sin_addr)
		}
		return cSock;
	}

	void addClientToCellServer(ClientSocketPtr& pClient)
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
		OnNetJoin(pClient);
	}

	void Start(int nCellServer)
	{
		for (int i = 0; i < nCellServer; i++)
		{
			CellServerPtr ser = std::make_shared<CellServer>(_sock);
			_cellServers.push_back(ser);
			//ע�������¼����ն���
			ser->setEventObj(this);
			//������Ϣ�����߳�
			ser->Start();
		}
	}

	//�ر�socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			//�ر��׽���closesocket
			closesocket(_sock);
#else
			close(_sock);
#endif
		}
	}
	bool onRun() 
	{
		if (isRun())
		{
			time4msg();
			fd_set fdRead;
			//���
			FD_ZERO(&fdRead);
			//����������������
			FD_SET(_sock, &fdRead);

			timeval t = { 0,10 };
			int ret = select(_sock + 1, &fdRead, 0, 0, &t);
			if (ret < 0)
			{
				printf("Accept Select���������\n");
				Close();
				return false;
			}
			//�ж�������(socket)�Ƿ��ڼ�����
			if (FD_ISSET(_sock, &fdRead))
			{
				//�������ж�Ӧ���������ļ���ֵ��0����δ�������������
				FD_CLR(_sock, &fdRead);
				Accept();
				return true;
			}
		}
	}
	//�Ƿ�����
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	//���㲢���ÿ���յ���������Ϣ
	void time4msg()
	{
		
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			printf("thread<%d>,time<%lf>,socket<%d>,clients<%d>,recv<%d>,msg<%d>\n",_cellServers.size(),t1, _sock, (int)_clientCount,(int)(_recvCount/ t1), (int)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}
	virtual void OnNetJoin(ClientSocketPtr& pClient)
	{
		_clientCount++;
		//printf("client<%d> join\n", pClient->sockfd());
	}
	virtual void OnNetLeave(ClientSocketPtr& pClient)
	{
		_clientCount--;
		//printf("client<%d> leave\n", pClient->sockfd());
	}
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocketPtr& pClient,DataHeader* header)
	{
		_msgCount++;
	}
	virtual void OnNetRecv(ClientSocketPtr& pClient)
	{
		_recvCount++; 
	}
};

#endif // _EasyTcpServer_hpp_
