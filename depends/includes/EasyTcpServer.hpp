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
	//每秒消息计时
	CELLTimestamp _tTime;
	//
	SOCKET _sock;
protected:
	//收到消息计数
	std::atomic_int _msgCount;
	//客户端进入计数
	std::atomic_int _clientCount;
	//recv计数
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

	//初始化socket
	SOCKET InitSocket()
	{
		CELLNetWork::Init();
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			CELLLog_Info("错误，建立socket失败...\n");
		}
		else {
			CELLLog_Info("建立<socket=%d>成功...\n", (int)_sock);
		}
		return _sock;
	}

	//绑定Ip和端口号
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
			CELLLog_Info("错误，绑定网络端口<%d>失败...\n",port);
		}
		else {
			CELLLog_Info("绑定端口<%d>成功...\n", port);
		}
		return ret;
	}

	//监听端口号
	int Listen(int n)
	{
		// 3 listen 监听网络端口
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			CELLLog_Info("socket=<%d>错误，监听网络端口失败...\n", (int)_sock);
		}
		else {
			CELLLog_Info("socket=<%d>监听网络端口成功...\n", (int)_sock);
		}
		return ret;
	}
	//接受客户端连接
	SOCKET Accept()
	{
		sockaddr_in _clientAddr = { };
		int nAddrLen = sizeof(_clientAddr);
		SOCKET cSock = INVALID_SOCKET;

		// accept 等待接收客户端连接
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&_clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&_clientAddr, (socklen_t*)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSock)
		{
			CELLLog_Info("socket=<%d>错误，接收到无效客户端SOCKET...\n", (int)_sock);
		}
		else {
			//将新客户端分配给客户数量最少的CELLServer
			addClientToCELLServer(new CELLClient(cSock));
			//获取IP地址	inet_ntoa(_clientAddr.sin_addr)
		}
		return cSock;
	}

	void addClientToCELLServer(CELLClient* pClient)
	{
		//查找客户数量最少的CELLServer消息处理对象
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
			//注册网络事件接收对象
			ser->setEventObj(this);
			//启动消息处理线程
			ser->Start();
		}
		_thread.Start(nullptr,
			[this](CELLThread* pThread) {onRun(pThread); });
	}

	//关闭socket
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
			//关闭套接字socket
			closesocket(_sock);
#else
			close(_sock);
#endif
			_sock == INVALID_SOCKET;
		} 
		CELLLog_Info("EasyTcpServer.Close end\n");
	}

	//计算并输出每秒收到的网络消息
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
	//处理网络消息
	void onRun(CELLThread* pThread)
	{
		while(pThread->isRun())
		{
			time4msg();
			fd_set fdRead;
			//清空
			FD_ZERO(&fdRead);
			//将描述符存入数组
			FD_SET(_sock, &fdRead);

			timeval t = { 0,1 };
			int ret = select(_sock + 1, &fdRead, 0, 0, &t);
			if (ret < 0)
			{
				CELLLog_Info("EasyTcpServer.OnRun Accept Select Exit.\n");
				pThread->Exit();
				break;
			}
			//判断描述符(socket)是否在集合中
			if (FD_ISSET(_sock, &fdRead))
			{
				//将数组中对应的描述符的计数值清0，并未将该描述符清除
				FD_CLR(_sock, &fdRead);
				Accept();
			}
		}
	}
};

#endif // _EasyTcpServer_hpp_
