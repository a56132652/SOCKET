# 一、首次封装EasyTcpServer

- 初始化SOCKET——InitSocket()
- 绑定端口号——Bind()
- 监听端口号——Listen()
- 接收客户端连接——Accept() 
- 关闭Socket——Close()
- 处理网络消息——OnRun()
- 是否工作中——isRun()
- 接收数据 处理粘包 拆分包——RecvData()
- 向指定Socket发送数据——SendData()
- 群发——SendDataToAll()
- 响应网络消息——virtual void OnNetMsg()

## 1. 初始化SOCKET

```c++
//初始化socket
	SOCKET InitSocket()
	{
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);				//创建WINDOWS版本号
		WSADATA dat;
		WSAStartup(ver, &dat);					//启动网络环境,此函数调用了一个WINDOWS的静态链接库，因此需要加入静态链接库文件
#endif

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("错误，建立socket失败...\n");
		}
		else {
			printf("建立<socket=%d>成功...\n", (int)_sock);
		}
		return _sock;
	}

```

## 2. 绑定IP和端口号

```c++
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
			printf("错误，绑定网络端口<%d>失败...\n",port);
		}
		else {
			printf("绑定端口<%d>成功...\n", port);
		}
		return ret;
	}


```

## 3. 监听端口号

```c++
//监听端口号
	int Listen(int n)
	{
		// 3 listen 监听网络端口
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("socket=<%d>错误，监听网络端口失败...\n", (int)_sock);
		}
		else {
			printf("socket=<%d>监听网络端口成功...\n", (int)_sock);
		}
		return ret;
	}

```

## 4. 接收客户端连接

```c++
//接受客户端连接
	SOCKET Accept()
	{
		sockaddr_in _clientAddr = { };
		int nAddrLen = sizeof(_clientAddr);
		SOCKET _cSock = INVALID_SOCKET;

		// accept 等待接收客户端连接
#ifdef _WIN32
		_cSock = accept(_sock, (sockaddr*)&_clientAddr, &nAddrLen);
#else
		_cSock = accept(_sock, (sockaddr*)&_clientAddr, (socklen_t*)&nAddrLen);
#endif
		if (INVALID_SOCKET == _cSock)
		{
			printf("socket=<%d>错误，接收到无效客户端SOCKET...\n", (int)_sock);
		}
		else {
			NewUserJoin userJoin;
			SendDataToAll(&userJoin);
			_clients.push_back(_cSock);
			printf("socket=<%d>新客户加入：IP = %s ,socket = %d \n", (int)_sock, inet_ntoa(_clientAddr.sin_addr), (int)_cSock);
		}
		return _cSock;
	}

```

## 5. 关闭SOCKET

```c++
//关闭socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (int n = _clients.size() - 1; n >= 0; n--)
			{
				closesocket(_clients[n]);
			}
			// 8 关闭套接字closesocket
			closesocket(_sock);

			//-----------------
			//清除Windows socket环境
			WSACleanup();							//关闭Socket网络环境
#else
			for (int n = _clients.size() - 1; n >= 0; n--)
			{
				close(_clients[n]);
			}
			close(_sock);
#endif
		}
	}
```

## 6. 处理网络消息

```c++
//处理网络消息
	bool onRun()
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

			SOCKET maxSock = _sock;
			//将新加入的客户端加入fdRead数组
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				FD_SET(_clients[n], &fdRead);
				if (_clients[n] > maxSock) {
					maxSock = _clients[n];
				}
			}
			timeval t = { 1,0 };
			//select最后一个参数为NULL时，select函数阻塞直到有数据可以操作,否则继续向下执行，当有消息时返回
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExc, &t);
			{
				if (ret < 0) {
					printf("select任务结束。\n");
					Close();
					return false;
				}
			}

			//测试该集合中一个给定位是否发生变化
			if (FD_ISSET(_sock, &fdRead))
			{
				//将数组中对应的描述符的计数值清0，并未将该描述符清除
				FD_CLR(_sock, &fdRead);
				Accept();
			}
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				if (FD_ISSET(_clients[n], &fdRead))
				{
                    //接受数据，如果数据有误，则清除该客户端socket
					if (RecvData(_clients[n]) == -1)
					{
						auto iter = _clients.begin() + n;
						if (iter != _clients.end())
						{
							_clients.erase(iter);
						}
					}
				}
			}
			return true;
		}
		return false;
	}

```

## 7. 是否工作中

```c++
//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	//接收数据 处理粘包 拆分包
	int RecvData(SOCKET _cSock)
	{
		char szRecv[1024] = { };
		int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
		// 5 接收客户端数据
		//将数据放在缓冲区中，缓冲区大小为1024
		//首先判断消息头
		DataHeader* header = (DataHeader*)szRecv;
		if (nLen <= 0)
		{
			printf("客户端<Socket:%d>已退出，任务结束。\n", _cSock);
			return -1;
		}
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNewMsg(_cSock,header);
		return 0;
	}

```

## 8. 响应网络消息

```c++
//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
```

## 9. 接收数据 处理粘包、拆包

```c++
//接收数据 处理粘包 拆分包
	int RecvData(SOCKET _cSock)
	{
		char szRecv[1024] = { };
		int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
		// 5 接收客户端数据
		//将数据放在缓冲区中，缓冲区大小为1024
		//首先判断消息头
		DataHeader* header = (DataHeader*)szRecv;
		if (nLen <= 0)
		{
			printf("客户端<Socket:%d>已退出，任务结束。\n", _cSock);
			return -1;
		}
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNewMsg(_cSock,header);
		return 0;
	}
```



## 10. 给指定SOCKET发送数据

```C++
//发送给指定Socket数据
	int SendData(SOCKET _cSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(_cSock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
```

## 11. 群发消息

```c++
//群发消息给所有Socket
	void SendDataToAll(DataHeader* header)
	{
		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			SendData(_clients[n], header);
		}
	}

```



# 二、缓冲区溢出、无法发送、网络阻塞

## 1. 原因

- 调用send（）函数时，系统会首先将消息放入一个发送缓冲区，该缓冲区的大小随操作系统类型而变。消息进入发送缓冲区后，并不是立即发出，而是有一个定时定量的发送机制，即经过一定时间后发出或者当消息缓冲区的数据量达到一定值后一起发出。消息经发送缓冲区，然后通过网络传输层，发送至接收端。

- 接收端也有一个接收缓冲区，调用recv（）时，将数据从接收缓冲区中取出 ,本程序中，每次取出固定大小的数据

  ```c++
  int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
  ```

  然后根据DataHeader中记录的消息体长度，再从接收缓冲区中取出整个消息

  ```c++
  recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
  ```

- 当send的速度大于recv的速度时，接收缓冲区将会发生溢出，发送端也将无法发送数据，造成网络阻塞

- 为什么前辈们在设计网络通信时没有为此问题设计解决方案？

  - 因为在当时，数据传输速率是很低的，数字拨号上网的带宽仅为56Kbps，所以当时的设计方案是符合当时的要求的



## 2. 解决方法

1. 尽可能的将数据从系统的接收缓冲区中提取出来，设计双缓冲区，即自定义一个接收缓冲区和一个消息缓冲区

   ```c++
       #ifndef RECV_BUFF_SIZE
       #define RECV_BUFF_SIZE 102400
       #endif // !RECV_BUFF_SIZE
   
       //缓冲区
       char _szRecv[RECV_BUFF_SIZE] = {};
       //第二缓冲区 消息缓冲区
       char _szMsgBuf[RECV_BUFF_SIZE * 10];
       //消息缓冲区的数据尾部位置
       int _lastPos = 0;
   ```

   

2. 每次将消息从系统的缓冲区中提取到接收缓冲区，然后将接收缓冲区的数据拷贝到消息缓冲区

   ```c++
    //接收数据 处理粘包 拆分包
   	int RecvData(SOCKET _cSock)
   	{
   		int nLen = recv(_cSock, _szRecv, RECV_BUFF_SIZE, 0);
   		if (nLen <= 0)
   		{
   			printf("客户端<Socket:%d>已退出，任务结束。\n", _cSock);
   			return -1;
   		}
           //将收取到的数据拷贝到消息缓冲区
   		memcpy(_szMsgBuf + _lastPos, _szRecv, nLen);
           //消息缓冲区的数据尾部位置后移
   		_lastPos += nLen;
           //判断消息缓冲区的数据长度大于消息头DataHeader长度
   		while (_lastPos >= sizeof(DataHeader))
   		{
   			//这时就可以知道当前消息的长度
   			DataHeader* header = (DataHeader*)_szMsgBuf;
   			//判断消息缓冲区的数据长度大于消息长度
   			if (_lastPos >= header->dataLength)
   			{
   				//消息缓冲区剩余未处理数据的长度
   				int nSize = _lastPos - header->dataLength;
   				//处理网络消息
   				OnNetMsg(_cSock,header);
   				//将消息缓冲区剩余未处理数据前移
   				memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
   				//消息缓冲区的数据尾部位置前移
   				_lastPos = nSize;
   			}
   			else {
   				//消息缓冲区剩余数据不够一条完整消息
   				break;
   			}
   		}
   		return 0;
   	}
   ```






# 三、定义客户类型ClientSocket

## ClientSocket

为方便管理，在服务端中定义一个客户数据类型ClientSocket

```c++

class ClientSocket 
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
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
private:
	// socket fd_set  file desc set
	SOCKET _sockfd;
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SZIE * 10];
	//消息缓冲区的数据尾部位置
	int _lastPos;
};
```



## EasyTcpServer

EasyTcpServer中也要进行相应更改

```c++
class EasyTcpServer
{
private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
	}
	virtual ~EasyTcpServer()
	{
		Close();
	}
	//初始化Socket
	SOCKET InitSocket()
	{
#ifdef _WIN32
		//启动Windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (INVALID_SOCKET != _sock)
		{
			printf("<socket=%d>关闭旧连接...\n", (int)_sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("错误，建立socket失败...\n");
		}
		else {
			printf("建立socket=<%d>成功...\n", (int)_sock);
		}
		return _sock;
	}

	//绑定IP和端口号
	int Bind(const char* ip, unsigned short port)
	{
		//if (INVALID_SOCKET == _sock)
		//{
		//	InitSocket();
		//}
		// 2 bind 绑定用于接受客户端连接的网络端口
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);//host to net unsigned short

#ifdef _WIN32
		if (ip){
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret)
		{
			printf("错误,绑定网络端口<%d>失败...\n", port);
		}
		else {
			printf("绑定网络端口<%d>成功...\n", port);
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
			printf("socket=<%d>错误,监听网络端口失败...\n",_sock);
		}
		else {
			printf("socket=<%d>监听网络端口成功...\n", _sock);
		}
		return ret;
	}

	//接受客户端连接
	SOCKET Accept()
	{
		// 4 accept 等待接受客户端连接
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSock)
		{
			printf("socket=<%d>错误,接受到无效客户端SOCKET...\n", (int)_sock);
		}
		else
		{
			NewUserJoin userJoin;
			SendDataToAll(&userJoin);
			_clients.push_back(new ClientSocket(cSock));
			printf("socket=<%d>新客户端加入：socket = %d,IP = %s \n", (int)_sock, (int)cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return cSock;
	}

	//关闭Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			// 8 关闭套节字closesocket
			closesocket(_sock);
			//------------
			//清除Windows socket环境
			WSACleanup();
#else
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				close(_clients[n]->sockfd());
				delete _clients[n];
			}
			// 8 关闭套节字closesocket
			close(_sock);
#endif
			_clients.clear();
		}
	}
	//处理网络消息
	int _nCount = 0;
	bool OnRun()
	{
		if (isRun())
		{
			//伯克利套接字 BSD socket
			fd_set fdRead;//描述符（socket） 集合
			fd_set fdWrite;
			fd_set fdExp;
			//清理集合
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);
			//将描述符（socket）加入集合
			FD_SET(_sock, &fdRead);
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExp);
			SOCKET maxSock = _sock;
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				FD_SET(_clients[n]->sockfd(), &fdRead);
				if (maxSock < _clients[n]->sockfd())
				{
					maxSock = _clients[n]->sockfd();
				}
			}
			///nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
			///既是所有文件描述符最大值+1 在Windows中这个参数可以写0
			timeval t = { 1,0 };
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t); //
			//printf("select ret=%d count=%d\n", ret, _nCount++);
			if (ret < 0)
			{
				printf("select任务结束。\n");
				Close();
				return false;
			}
			//判断描述符（socket）是否在集合中
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				Accept();
			}
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				if (FD_ISSET(_clients[n]->sockfd(), &fdRead))
				{
					if (-1 == RecvData(_clients[n]))
					{
						auto iter = _clients.begin() + n;//std::vector<SOCKET>::iterator
						if (iter != _clients.end())
						{
							delete _clients[n];
							_clients.erase(iter);
						}
					}
				}
			}
			return true;
		}
		return false;

	}
	//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	//缓冲区
	char _szRecv[RECV_BUFF_SZIE] = {};

	//接收数据 处理粘包 拆分包
	int RecvData(ClientSocket* pClient)
	{
		// 5 接收客户端数据
		int nLen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SZIE, 0);
		//printf("nLen=%d\n", nLen);
		if (nLen <= 0)
		{
			printf("客户端<Socket=%d>已退出，任务结束。\n", pClient->sockfd());
			return -1;
		}
		//将收取到的数据拷贝到消息缓冲区
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		//消息缓冲区的数据尾部位置后移
		pClient->setLastPos(pClient->getLastPos() + nLen);

		//判断消息缓冲区的数据长度大于消息头DataHeader长度
		while (pClient->getLastPos() >= sizeof(DataHeader))
		{
			//这时就可以知道当前消息的长度
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			//判断消息缓冲区的数据长度大于消息长度
			if (pClient->getLastPos() >= header->dataLength)
			{
				//消息缓冲区剩余未处理数据的长度
				int nSize = pClient->getLastPos() - header->dataLength;
				//处理网络消息
				OnNetMsg(pClient->sockfd(),header);
				//将消息缓冲区剩余未处理数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				//消息缓冲区的数据尾部位置前移
				pClient->setLastPos(nSize);
			}
			else {
				//消息缓冲区剩余数据不够一条完整消息
				break;
			}
		}
		return 0;
	}
	//响应网络消息
	virtual void OnNetMsg(SOCKET cSock, DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			
			Login* login = (Login*)header;
			//printf("收到客户端<Socket=%d>请求：CMD_LOGIN,数据长度：%d,userName=%s PassWord=%s\n", cSock, login->dataLength, login->userName, login->PassWord);
			//忽略判断用户密码是否正确的过程
			LoginResult ret;
			SendData(cSock, &ret);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			//printf("收到客户端<Socket=%d>请求：CMD_LOGOUT,数据长度：%d,userName=%s \n", cSock, logout->dataLength, logout->userName);
			//忽略判断用户密码是否正确的过程
			LogoutResult ret;
			SendData(cSock, &ret);
		}
		break;
		default:
		{
			printf("<socket=%d>收到未定义消息,数据长度：%d\n", cSock, header->dataLength);
			//DataHeader ret;
			//SendData(cSock, &ret);
		}
		break;
		}
	}

	//发送指定Socket数据
	int SendData(SOCKET cSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(cSock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	void SendDataToAll(DataHeader* header)
	{
		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			SendData(_clients[n]->sockfd(), header);
		}
	}

};
```





# 四、添加高精度计时器CELLTimestamp.hpp

使用c++11标准下的 std::chrono库，具体解析可见网址

[[C++11 std::chrono库详解](https://www.cnblogs.com/jwk000/p/3560086.html)](https://www.cnblogs.com/jwk000/p/3560086.html)



# 五、升级多线程服务端

## 1. 线程创建函数

使用C++提供的线程库

```c++
#include<thread>

void workFun(int n){}

//main()函数作为主线程的入口函数
int main()
{
	//传入入口函数workFun(),及入口函数的参数
	thread t(workFun,10);
	//启动线程
	t.detach();
	//另外一种启动方式
	t.join();
	
	return 0;
}
```

- join()函数是一个等待线程完成函数，主线程需要等待子线程运行结束了才可以结束
- detach()称为分离线程函数，使用detach()函数会让线程在后台运行，即说明主线程不会等待子线程运行结束才结束
  - 抢占式

**join()函数是一个等待线程函数，主线程需等待子线程运行结束后才可以结束（注意不是才可以运行，运行是并行的），如果打算等待对应线程，则需要细心挑选调用join()的位置**
**detach()函数是子线程的分离函数，当调用该函数后，线程就被分离到后台运行，主线程不需要等待该线程结束才结束**

## 2. 锁与临界区域

使用c++提供的库<mutex>

```c++
#include <mutex>
using namespace std;

mutex m;
m.lock();
//临界区域-开始
 临界资源
//临界区域-结束
m.unlock();
```

**自解锁**

```
lock_guard<mutex> lg(m);
```

**原子操作**

```c++
#include<atomic>
atomic_int sum = 0;
atomic<int> sum = 0;
```

- atomic对int、char、bool等数据结构进行了原子性封装，在多线程环境中，对std::atomic对象的访问不会造成竞争-冒险。利用std::atomic可实现数据结构的无锁设计。
- 所谓的原子操作，取的就是“原子是最小的、不可分割的最小个体”的意义，它表示在多个线程访问同一个全局资源的时候，能够确保所有其他的线程都不在同一时间内访问相同的资源。也就是他确保了在同一时刻只有唯一的线程对这个资源进行访问。这有点类似互斥对象对共享资源的访问的保护，但是原子操作更加接近底层，因而效率更高。



