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



## 3. 升级多线程客户端，模拟高频并发数据

 客户端多线程分组发送消息，尽可能提升发送效率，模拟大规模客户端并发连接，并发数据消息

```c++
//发送线程
void sendThread(int id)
{
	printf("thread<%d> ,start\n", id);
	//4个线程，ID 1 - 4
	int c = cCount / tCount;
	int begin = (id - 1) * c;  
	int end = id * c;

	for (int i = begin; i < end; i++)
	{
		client[i] = new EasyTcpClient();
	}  
	for (int i = begin; i < end; i++)
	{

		client[i]->Connect("127.0.0.1", 4567);	//Linux:192.168.43.129	yql:192.168.1.202  benji:192.168.43.1
	}
	//心跳检测 死亡计时  
	printf("thread<%d> Connect<begin=%d, end=%d>\n", id, begin,end);

	readyCount++;
	while (readyCount < tCount) {
		//等待其他线程准备好发送数据
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}
	
	//启动接收线程
	std::thread t1(recvThread, begin, end);
	t1.detach();

	Login login[1];
	for (int i = 0; i < 1; i++)
	{
		strcpy(login[i].userName, "yql");
		strcpy(login[i].PassWord, "yqlyyds");
	}
	const int nLen = sizeof(login);
	while (g_bRun)
	{
		for (int i = begin; i < end; i++)
		{
			if (SOCKET_ERROR != client[i]->SendData(login, nLen)) {
					sendCount++;
			}
		}
		std::chrono::milliseconds t(100);
		std::this_thread::sleep_for(t);
	}

	for (int n = begin; n < end; n++)
	{
		client[n]->Close();	//Linux:192.168.43.129
		delete client[n];
	}
	printf("thread<%d> ,exit\n", id);
}
```

## 4. 服务端多线程模型

采用生产者-消费者设计模式

![image-20211012212048439](C:\Users\Sakura\AppData\Roaming\Typora\typora-user-images\image-20211012212048439.png)

生产者线程用于接收客户端连接，当接收了一个新连接，会产生一个新的socket，将此socket传递给另外一个线程，另外一个线程用来处理socket，即消费者线程。

缓冲区作用：

1. 解耦，生产者和消费者只依赖缓冲区，而不互相依赖
2. 支持并发和异步

###  1）**完善结构：**

​	目前缓冲区只用于传递socket，当消费者线程中recv()返回-1即有客户端断开连接时，消费者线程还需要通知生产者线程，还缺少这样一个反向的逻辑。 

### 2）**修改代码**

#### （1）从EasyTcpServer中分离处理消息逻辑

定义新类型CELLServer，用于消息处理

```c++
//网络消息接收处理服务类
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

	//关闭Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (auto iter : _clients)
			{
				closesocket(iter.second->sockfd());
				delete iter.second;
			}
			//关闭套节字closesocket
			closesocket(_sock);
#else
			for (auto iter : _clients)
			{
				close(iter.second->sockfd());
				delete iter.second;
			}
			//关闭套节字closesocket
			close(_sock);
#endif
			_clients.clear();
		}
	}

	//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//处理网络消息
	//备份客户socket fd_set
	fd_set _fdRead_bak;
	//客户列表是否有变化
	bool _clients_change;
	SOCKET _maxSock;
	void OnRun()
	{
		_clients_change = true;
		while (isRun())
		{
			if (!_clientsBuff.empty())
			{//从缓冲队列里取出客户数据
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
				}
				_clientsBuff.clear();
				_clients_change = true;
			}

			//如果没有需要处理的客户端，就跳过
			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			//伯克利套接字 BSD socket
			fd_set fdRead;//描述符（socket） 集合
			//清理集合
			FD_ZERO(&fdRead);
			if (_clients_change)
			{
				_clients_change = false;
				//将描述符（socket）加入集合
				_maxSock = _clients.begin()->second->sockfd();
				for (auto iter : _clients)
				{
					FD_SET(iter.second->sockfd(), &fdRead);
					if (_maxSock < iter.second->sockfd())
					{
						_maxSock = iter.second->sockfd();
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else {
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			///nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
			///既是所有文件描述符最大值+1 在Windows中这个参数可以写0
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0)
			{
				printf("select任务结束。\n");
				Close();
				return;
			}
			else if (ret == 0)
			{
				continue;
			}
			
#ifdef _WIN32
			for (int n = 0; n < fdRead.fd_count; n++)
			{
				auto iter  = _clients.find(fdRead.fd_array[n]);
				if (iter != _clients.end())
				{
					if (-1 == RecvData(iter->second))
					{
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(iter->second);
						_clients_change = true;
						_clients.erase(iter->first);
					}
				}else {
					printf("error. if (iter != _clients.end())\n");
				}

			}
#else
			std::vector<ClientSocket*> temp;
			for (auto iter : _clients)
			{
				if (FD_ISSET(iter.second->sockfd(), &fdRead))
				{
					if (-1 == RecvData(iter.second))
					{
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(iter.second);
						_clients_change = false;
						temp.push_back(iter.second);
					}
				}
			}
			for (auto pClient : temp)
			{
				_clients.erase(pClient->sockfd());
				delete pClient;
			}
#endif
		}
	}
	//接收数据 处理粘包 拆分包
	int RecvData(ClientSocket* pClient)
	{

		//接收客户端数据
		char* szRecv = pClient->msgBuf() + pClient->getLastPos();
		int nLen = (int)recv(pClient->sockfd(), szRecv, (RECV_BUFF_SZIE)- pClient->getLastPos(), 0);
		_pNetEvent->OnNetRecv(pClient);
		//printf("nLen=%d\n", nLen);
		if (nLen <= 0)
		{
			//printf("客户端<Socket=%d>已退出，任务结束。\n", pClient->sockfd());
			return -1;
		}
		//将收取到的数据拷贝到消息缓冲区
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
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
				OnNetMsg(pClient, header);
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
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_pNetEvent->OnNetMsg(this, pClient, header);
	}

	void addClient(ClientSocket* pClient)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	void Start()
	{
        //std::mem_fn(),把成员函数转为函数对象，使用对象指针或对象（引用）进行绑定
		_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
		_taskServer.Start();
	}

	size_t getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}

	void addSendTask(ClientSocket* pClient, DataHeader* header)
	{
		CellSendMsg2ClientTask* task = new CellSendMsg2ClientTask(pClient, header);
		_taskServer.addTask(task);
	}
private:
	SOCKET _sock;
	//正式客户队列
	std::map<SOCKET,ClientSocket*> _clients; 
	//缓冲客户队列
	std::vector<ClientSocket*> _clientsBuff;
	//缓冲队列的锁
	std::mutex _mutex;
    //为了方便管理，定义一个线程成员
	std::thread _thread;
	//网络事件对象
	INetEvent* _pNetEvent;
	//
	CellTaskServer _taskServer;
};
```

#### （2） 将消息处理类型用于EasyTcpServer

- 在EasyTcpServer成员变量中添加一个消息处理对象的数组

  ```c++
  vector<CellServer*> _cellServers
  ```

- 定义消息处理线程启动方法

  ```c++
  void Start(int nCellServer)
  	{
  		for (int n = 0; n < nCellServer; n++)
  		{
  			auto ser = new CellServer(_sock);
  			_cellServers.push_back(ser);
  			//注册网络事件接受对象
  			ser->setEventObj(this);
  			//启动消息处理线程
  			ser->Start();
  		}
  	}
  ```

- 将新客户端分配给客户数量最少的消息线程

  ```c++
  void addClientToCellServer(ClientSocket* pClient)
  	{
  		//查找客户数量最少的CellServer消息处理对象
  		auto pMinServer = _cellServers[0];
  		for(auto pCellServer : _cellServers)
  		{
  			if (pMinServer->getClientCount() > pCellServer->getClientCount())
  			{
  				pMinServer = pCellServer;
  			}
  		}
  		pMinServer->addClient(pClient);
  		OnNetJoin(pClient);
  	}
  ```

  

- 事件通知，告知主线程有客户端退出

  实现一个网络事件接口（纯虚函数），让EasyTcpServer继承此接口，然后在EasyTcpServer中进行具体实现的定义

  ```c++
  //网络事件接口
  class INetEvent
  {
  public:
  	//纯虚函数
  	//客户端加入事件
  	virtual void OnNetJoin(ClientSocket* pClient) = 0;
  	//客户端离开事件
  	virtual void OnNetLeave(ClientSocket* pClient) = 0;
  	//客户端消息事件
  	virtual void OnNetMsg(CellServer* pCellServer, ClientSocket* pClient, DataHeader* header) = 0;
  	//recv事件
  	virtual void OnNetRecv(ClientSocket* pClient) = 0;
  private:
  };
  ```

  

### 3）达成目标，迅速稳定连接10K客户端



# 六、select模型接收数据性能瓶颈与优化



## 1. 利用性能探查器分析性能消耗

![image-20211014173107333](C:\Users\Sakura\AppData\Roaming\Typora\typora-user-images\image-20211014173107333.png)



![image-20211014173117088](C:\Users\Sakura\AppData\Roaming\Typora\typora-user-images\image-20211014173117088.png)



![image-20211014173430135](C:\Users\Sakura\AppData\Roaming\Typora\typora-user-images\image-20211014173430135.png)



## 2. 优化FD_SET

#### 	优化前

```c++
for(int n = (int)_clients.size() - 1 ; n>= 0; n--)
{
    //FD_SET(fd_set *fdset);用于在文件描述符集合中增加一个新的文件描述符。
    FD_SET(_clients[n]->sockfd(), &fdRead);
    if(maxSock < _clients[n]->sockfd())
    {
        maxSock = _clients[n]->sockfd();
    }
}
```

当n = 10000时，在while死循环中，每次循环都要将这10000个socket逐个加入fdRead中，耗费了大量时间。

#### **解决方法：**

​	定义一个备份数组以及一个标志量，标志量用来标识是否有新客户加入或是否有客端离开，备份数组用来备份上次循环中的fdRead数组，若没有发生变化，则下次循环时直接将备份数组拷贝给fdRead，若发生变化，则采用旧方法，对现有的所有socket进行逐个加入

#### 优化后

```c++
if (_clients_change)
{
	_clients_change = false;
	//清空
	FD_ZERO(&fdRead);
	//将描述符存入集合
	_maxSock = _clients.begin()->second->sockfd();
	//将新加入的客户端加入fdRead数组
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
```

![image-20211014203347034](C:\Users\Sakura\AppData\Roaming\Typora\typora-user-images\image-20211014203347034.png)

![image-20211014203403680](C:\Users\Sakura\AppData\Roaming\Typora\typora-user-images\image-20211014203403680.png)

![image-20211014203442376](C:\Users\Sakura\AppData\Roaming\Typora\typora-user-images\image-20211014203442376.png)

此时性能消耗主要集中在select模型。

## 3. 优化FD_ISSET

将_clients从vector换成map,提升查询效率

```c++
#ifdef _WIN32
		for (int n = 0; n < fdRead.fd_count; n++)
		{
			auto iter = _clients.find(fdRead.fd_array[n]);
			if (iter != _clients.end())
			{
				if (-1 == RecvData(iter->second))
				{
					OnClientLeave(iter->second);
					_clients.erase(iter);
				}
			}
		}
#else
		for (auto iter = _clients.begin(); iter != _clients.end(); )
		{
			if (FD_ISSET(iter->second->sockfd(), &fdRead))
			{
				if (-1 == RecvData(iter->second))
				{
					OnClientLeave(iter->second);
					auto iterOld = iter;
					iter++;
					_clients.erase(iterOld);
					continue;
				}
			}
			iter++;
		}
#endif
```

#### 优化后

![image-20211014210314616](C:\Users\Sakura\AppData\Roaming\Typora\typora-user-images\image-20211014210314616.png)

性能消耗集中在select模型中

 

## 4. CELLServer数据收发的性能瓶颈

经过测试，recv能力远大于send,recv可达到 400万+/秒，send可达到170万+/秒

当前程序中，数据的收与发在同一线程中完成，线程中，只有当数据发送完后，才能继续去处理别的消息，这不符合程序设计中减少耦合性的要求，因此，需要将收发业务分离，接收与发送分别用两个线程来处理，依旧使用生产者-消费者设计模式



## 5. 添加定量发送数据缓冲区

在实际应用场景中，当Server接收（recv）一次某客户端数据时，Server可能还要通知其它N个客户端，即调用N次send()函数。在这种情况下，为了提高程序效率，需要减少send()的调用，因此我们可以把要发送的数据积攒起来，即用一个缓冲区缓存起来，在**经过一段时间或当消息量达到一定量后**进行一次性发送，即**定时定量发送**。

```c++
    #define RECV_BUFF_SIZE 10240*5
    #define SEND_BUFF_SIZE RECV_BUFF_SIZE

	//发送数据
	int SendData(DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		//要发送的数据长度
		int nSendLen = header->dataLength;
		//要发送的数据
		const char* pSendData = (const char*)header;

		//while循环用于发送数据量远远大于发送缓冲区时，一次发送不可能发完
		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
			{
				//计算可拷贝的数据长度
				int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
				//拷贝数据
				memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
				//计算剩余数据位置
				pSendData += nCopyLen;
				//计算剩余数据长度
				nSendLen -= nCopyLen;
				//发送数据
				ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
				//数据尾部位置清零
				_lastSendPos = 0;
				//发送错误
				if (SOCKET_ERROR == ret)
				{
					return ret;
				}
			}else {
				//将要发送的数据 拷贝到发送缓冲区尾部
				memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
				//计算数据尾部位置
				_lastSendPos += nSendLen;
				//退出循环
				break;
			}
		}
		return ret;
	}

```



## 6. Server消息接收与发送分离

当前程序中，数据的收与发在同一线程中完成，线程中，只有当数据发送完后，才能继续去处理别的消息，这不符合程序设计中减少耦合性的要求，因此，需要将收发业务分离，接收与发送分别用两个线程来处理，依旧使用生产者-消费者设计模式

### 1) 添加新类型 CELLTask(任务基类)

```c++
//任务类型-基类
class CellTask
{
public:
	CellTask()
	{

	}

	//虚析构
	virtual ~CellTask()
	{

	}
	//执行任务
	virtual void doTask()
	{

	}
private:

};
```

### 2) 添加任务的服务类型 CELLTaskServer

```c++
//执行任务的服务类型
class CellTaskServer 
{
private:
	//任务数据
	std::list<CellTask*> _tasks;
	//任务数据缓冲区
	std::list<CellTask*> _tasksBuf;
	//改变数据缓冲区时需要加锁
	std::mutex _mutex;
public:
	//添加任务
	void addTask(CellTask* task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}
	//启动工作线程
	void Start()
	{
		//线程
		std::thread t(std::mem_fn(&CellTaskServer::OnRun),this);
		t.detach();
	}
protected:
	//工作函数
	void OnRun()
	{
		while (true)
		{
			//从缓冲区取出数据
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _tasksBuf)
				{
					_tasks.push_back(pTask);
				}
				_tasksBuf.clear();
			}
			//如果没有任务
			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			//处理任务
			for (auto pTask : _tasks)
			{
				pTask->doTask();
				delete pTask;
			}
			//清空任务
			_tasks.clear();
		}
	}
};
```

### 3) 分离向客户端发送数据业务

#### （1）定义 网络消息发送任务

```c++
//网络消息发送任务
class CellSendMsg2ClientTask:public CellTask
{
	ClientSocket* _pClient;
	DataHeader* _pHeader;
public:
	CellSendMsg2ClientTask(ClientSocket* pClient, DataHeader* header)
	{
		_pClient = pClient;
		_pHeader = header;
	}

	//执行任务
	void doTask()
	{
		_pClient->SendData(_pHeader);
		delete _pHeader;
	}
};
```

#### （2）CELLServer中添加

	CeLLTaskServer _taskServer;

#### （3）调用

```c++
LoginResult* ret = new LoginResult();
pCellServer->addSendTask(pClient, ret);
```

### 4）隐患

频繁的申请与释放内存，降低了效率，还可能形成内存碎片

![image-20211016225851183](C:\Users\Sakura\AppData\Roaming\Typora\typora-user-images\image-20211016225851183.png)



## 7. 内存管理

-   做好内存管理：避免内存碎片的产生，使程序长期稳定、高效的运行
  - **内部碎片：**已经被分配出去（能明确指出属于哪个进程）却不能被利用的内存空间(需要50MB的进程却占用了60MB的内存，这10MB就是内部碎片)
  - **外部碎片**：还没有被分配出去（不属于任何进程），但由于太小了无法分配给申请内存空间的新进程的内存空闲区域。
- 内存池：从系统中申请足够大小的内存，由程序自己管理
- 对象池：创建足够多的对象，减少创建释放对象的消耗
- 智能指针：保证被创建的对象，正确的释放  

### 1）内存池设计

![image-20211017160432296](C:\Users\Sakura\AppData\Roaming\Typora\typora-user-images\image-20211017160432296.png)

#### （1）内存块结构设计

**每个小内存块设计为两部分，一部分为内存块头，其中个记录内存块的信息，另一部分则为实际可用内存**

| 小内存块的描述信息 |  实际可用内存  |
| :----------------: | :------------: |
|    MemoryBlock     |    最小单元    |
|        nID         |   内存块编号   |
|        nRef        |    引用次数    |
|       pAlloc       |   所属内存块   |
|       pNext        |   下一块位置   |
|       bPool        | 是否在内存池中 |

#### （2）内存池高效设计

内存池需要对每一个链表做字典映射，在需内存的时候找到对应的链表取出数据，如果链表较多，查找速率下降很多

因此采用映射机制，无需查找，可直接定位到该内存链表

### 2）内存池实现

#### （1）内存块MemoryBlock

```c++
//内存块 最小单元
class MemoryBlock
{
public:
	//所属大内存块（池）
	MemoryAlloc* pAlloc;
	//下一块位置
	MemoryBlock* pNext;
	//内存块编号
	int nID;
	//引用次数
	int nRef;
	//是否在内存池中
	bool bPool;
private:
	//预留
	char c1;
	char c2;
	char c3;
};

```

#### （2）内存池MemoryAlloc

```c++
//内存池
class MemoryAlloc
{
public:
	MemoryAlloc()
	{
		_pBuf = nullptr;
		_pHeader = nullptr;
		_nSzie = 0;
		_nBlockSzie = 0;
	}

	~MemoryAlloc()
	{
		if (_pBuf)
			free(_pBuf);
	}

	//申请内存
	void* allocMemory(size_t nSize)
	{
		if (!_pBuf)
		{
			initMemory();
		}

		MemoryBlock* pReturn = nullptr;
		if (nullptr == _pHeader)
		{
			pReturn = (MemoryBlock*)malloc(nSize+sizeof(MemoryBlock));
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
		}
		else {
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;
			assert(0 == pReturn->nRef);
			pReturn->nRef = 1;
		}
		xPrintf("allocMem: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		return ((char*)pReturn + sizeof(MemoryBlock));
	}

	//释放内存
	void freeMemory(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)( (char*)pMem  - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);
		if (--pBlock->nRef != 0)
		{
			return;
		}
		if (pBlock->bPool)
		{
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else {
			free(pBlock);
		}
	}

	//初始化
	void initMemory()
	{	//断言
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;
		//计算内存池的大小
		size_t realSzie = _nSzie + sizeof(MemoryBlock);
		size_t bufSize = realSzie*_nBlockSzie;
		//向系统申请池的内存
		_pBuf = (char*)malloc(bufSize);

		//初始化内存池
		_pHeader = (MemoryBlock*)_pBuf;
		_pHeader->bPool = true;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->pAlloc = this;
		_pHeader->pNext = nullptr;
		//遍历内存块进行初始化
		MemoryBlock* pTemp1 = _pHeader;
		
		for (size_t n = 1; n < _nBlockSzie; n++)
		{
			MemoryBlock* pTemp2 = (MemoryBlock*)(_pBuf + (n* realSzie));
			pTemp2->bPool = true;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->pAlloc = this;
			pTemp2->pNext = nullptr;
			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}
protected:
	//内存池地址
	char* _pBuf;
	//头部内存单元
	MemoryBlock* _pHeader;
	//内存单元的大小
	size_t _nSzie;
	//内存单元的数量
	size_t _nBlockSzie;
};
```

#### （3）内存管理工具MemoryMgr

```c++
//内存管理工具
class MemoryMgr
{
private:
	MemoryMgr()
	{
		init_szAlloc(0, 64, &_mem64);
		init_szAlloc(65, 128, &_mem128);
		init_szAlloc(129, 256, &_mem256);
		init_szAlloc(257, 512, &_mem512);
		init_szAlloc(513, 1024, &_mem1024);
	}

	~MemoryMgr()
	{

	}

public:
	static MemoryMgr& Instance()
	{//单例模式 静态
		static MemoryMgr mgr;
		return mgr;
	}
	//申请内存
	void* allocMem(size_t nSize)
	{
		if (nSize <= MAX_MEMORY_SZIE)
		{
			return _szAlloc[nSize]->allocMemory(nSize);
		}
		else 
		{
			MemoryBlock* pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			xPrintf("allocMem: %llx, id=%d, size=%d\n", pReturn , pReturn->nID, nSize);
			return ((char*)pReturn + sizeof(MemoryBlock));
		}
		
	}

	//释放内存
	void freeMem(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		xPrintf("freeMem: %llx, id=%d\n", pBlock, pBlock->nID);
		if (pBlock->bPool)
		{
			pBlock->pAlloc->freeMemory(pMem);
		}
		else 
		{
			if (--pBlock->nRef == 0)
				free(pBlock);
		}
	}

	//增加内存块的引用计数
	void addRef(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		++pBlock->nRef;
	}
private:
	//初始化内存池映射数组
	void init_szAlloc(int nBegin, int nEnd, MemoryAlloc* pMemA)
	{
		for (int n = nBegin; n <= nEnd; n++)
		{
			_szAlloc[n] = pMemA;
		}
	}
private:
	MemoryAlloctor<64, 100000> _mem64;
	MemoryAlloctor<128, 100000> _mem128;
	MemoryAlloctor<256, 100000> _mem256;
	MemoryAlloctor<512, 100000> _mem512;
	MemoryAlloctor<1024, 100000> _mem1024;
	MemoryAlloc* _szAlloc[MAX_MEMORY_SZIE + 1];
};
```

#### （4）MemoryAlloctor

```c++
//便于在声明类成员变量时初始化MemoryAlloc的成员数据
template<size_t nSzie,size_t nBlockSzie>
class MemoryAlloctor :public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		//8 4   61/8=7  61%8=5
		const size_t n = sizeof(void*);
		//(7*8)+8 
		_nSzie = (nSzie/n)*n +(nSzie % n ? n : 0);
		_nBlockSzie = nBlockSzie;
	}

};
```

### 3) 代码编写细节

- 由于内存块分为块头以及实际使用内存两部分，因此在进行内存申请与释放时要额外注意。当申请内存时，我们申请到的是一块完整的内存，即包含内存头和实际使用内存，此时指针指向内存头，而返回的时候，应该对该地址进行偏移，使其指向实际使用内存，然后返回给用户；当释放内存时，传入的是实际使用内存的地址，因此我们也要偏移，使指针指向内存头，然后进行释放操作。
- 使用了模板编程MemoryAlloctor，便于在声明类成员变量时初始化MemoryAlloc的成员数据
- 调试技巧

```c++
//在Visual Studio Debug模式下，输出调试信息，否则，不输出
#ifdef _DEBUG
#include<stdio.h>
	#define xPrintf(...) printf(__VA_ARGS__)
#else
	#define xPrintf(...)
#endif
```

### 

### 4）支持多线程

**申请内存加锁**

```c++

	//申请内存
	void* allocMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (!_pBuf)
		{
			initMemory();
		}
        ...
    }
		
```

**释放内存加锁**

```c++
//释放内存
	void freeMemory(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)( (char*)pMem  - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);
		if (pBlock->bPool)
		{
			std::lock_guard<std::mutex> lg(_mutex);
			if (--pBlock->nRef != 0)
			{
				return;
			}
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else {
			if (--pBlock->nRef != 0)
			{
				return;
			}
			free(pBlock);
		}
	}
```



### 5）使用智能指针替换new()以及delete()

```c++
typedef std::shared_ptr<CellTask> CellTaskPtr;

typedef std::shared_ptr<DataHeader> DataHeaderPtr;
typedef std::shared_ptr<LoginResult> LoginResultPtr;

typedef std::shared_ptr<CellS2CTask> CellS2CTaskPtr;

	//正式客户队列
std::map<SOCKET,ClientSocketPtr> _clients;
	//缓冲客户队列
std::vector<ClientSocketPtr> _clientsBuff;

```



### 6）对象池

在类中重载new()和delete()时，当使用new()创建对象时，优先使用类中的new()，因此对象池可以和全局的内存池共存，若想使用对象池，则在类中重载，若使用全局的内存池，则不在类中重载new()。

 

```c++
#ifndef _CELLObjectPool_hpp_
#define _CELLObjectPool_hpp_
#include<stdlib.h>
#include<assert.h>
#include<mutex>

#ifdef _DEBUG
	#ifndef xPrintf
		#include<stdio.h>
		#define xPrintf(...) printf(__VA_ARGS__)
	#endif
#else
	#ifndef xPrintf
		#define xPrintf(...)
	#endif
#endif // _DEBUG

template<class Type, size_t nPoolSzie>
class CELLObjectPool
{
public:
	CELLObjectPool()
	{
		initPool();
	}

	~CELLObjectPool()
	{
		if(_pBuf)
			delete[] _pBuf;
	}
private:
	class NodeHeader
	{
	public:
		//下一块位置
		NodeHeader* pNext;
		//内存块编号
		int nID;
		//引用次数
		char nRef;
		//是否在内存池中
		bool bPool;
	private:
		//预留
		char c1;
		char c2;
	};
public:
	//释放对象内存
	void freeObjMemory(void* pMem)
	{
		NodeHeader* pBlock = (NodeHeader*)((char*)pMem - sizeof(NodeHeader));
		xPrintf("freeObjMemory: %llx, id=%d\n", pBlock, pBlock->nID);
		assert(1 == pBlock->nRef);
		if (pBlock->bPool)
		{
			std::lock_guard<std::mutex> lg(_mutex);
			if (--pBlock->nRef != 0)
			{
				return;
			}
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else {
			if (--pBlock->nRef != 0)
			{
				return;
			}
			delete[] pBlock;
		}
	}
	//申请对象内存
	void* allocObjMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		NodeHeader* pReturn = nullptr;
		if (nullptr == _pHeader)
		{
			pReturn = (NodeHeader*)new char[sizeof(Type) + sizeof(NodeHeader)];
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pNext = nullptr;
		}
		else {
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;
			assert(0 == pReturn->nRef);           
			pReturn->nRef = 1;
		}
		xPrintf("allocObjMemory: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		return ((char*)pReturn + sizeof(NodeHeader));
	}
private:
	//初始化对象池
	void initPool()
	{
		//断言
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;
		//计算对象池的大小
		size_t realSzie = sizeof(Type) + sizeof(NodeHeader);
		size_t n = nPoolSzie*realSzie;
		//申请池的内存
		_pBuf = new char[n];
		//初始化内存池
		_pHeader = (NodeHeader*)_pBuf;
		_pHeader->bPool = true;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->pNext = nullptr;
		//遍历内存块进行初始化
		NodeHeader* pTemp1 = _pHeader;

		for (size_t n = 1; n < nPoolSzie; n++)
		{
			NodeHeader* pTemp2 = (NodeHeader*)(_pBuf + (n* realSzie));
			pTemp2->bPool = true;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->pNext = nullptr;
			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}
private:
	//
	NodeHeader* _pHeader;
	//对象池内存缓存区地址
	char* _pBuf;
	//
	std::mutex _mutex;
};

template<class Type, size_t nPoolSzie>
class ObjectPoolBase
{
public:
	void* operator new(size_t nSize)
	{
		return objectPool().allocObjMemory(nSize);
	}

	void operator delete(void* p)
	{
		objectPool().freeObjMemory(p);
	}

	template<typename ...Args>
	static Type* createObject(Args ... args)
	{	//不定参数  可变参数
		Type* obj = new Type(args...);
		//可以做点想做的事情
		return obj;
	}

	static void destroyObject(Type* obj)
	{
		delete obj;
	}
private:
	//
	typedef CELLObjectPool<Type, nPoolSzie> ClassTypePool;
	//
	static ClassTypePool& objectPool()
	{	//静态CELLObjectPool对象
		static ClassTypePool sPool;
		return sPool;
	}
};

#endif // !_CELLObjectPool_hpp_

```

#### 小技巧：

1. 可变参数

   ```c++
   template<typename ...Args>
   static Type* createObject(Args ... args)
   {	//不定参数  可变参数
   	Type* obj = new Type(args...);
   	//可以做点想做的事情
   	return obj;
   }
   ```



#### 将对象池应用到EasyTCPServer中

​	为ClientSocket创建对象池

```c++
class ClientSocket : public ObjectPoolBase<ClientSocket,10000>
{
  ...  
}
```



## 8. 内存管理设计结束，将代码版本回退到无内存管理状态，先完善其他功能





## 9. 优化CELLTaskServer

### 1）function

​		类模版`std::function`是一种通用、多态的函数封装。`std::function`的实例可以对任何可以调用的目标实体进行存储、复制、和调用操作，这些目标实体包括普通函数、Lambda表达式、函数指针、以及其它函数对象等。`std::function`对象是对C++中现有的可调用实体的一种类型安全的包裹。

​		通过`std::function`对C++中各种可调用实体的封装，形成一个可调用的`std::function`对象，让我们不再纠结那么多的可调用实体，一切变得简单粗暴。

```c++
#include <functionl>

void funA()
{
    printf("funA\n");
}

void funB(int n)
{
    printf("funB : %d\n",n);
}

int funC(int a , int b)
{
    printf("funC : %d,%d\n",a,b);
    return 0;
}

int main()
{
    std::function<void()> callA = funA;
    std::function<void(int)> callB = funB;
    std::function<itn(int,int)> callC = funC;
    
    callA();
    callB(10);
    int c = callC(10,10);
    return 0;
}

/*-----------------------输出------------------------*/
//  funA
//	funB : 10
//	funC : 10,10
//	c = 0;
```

### 2）lambda表达式

```c++
[capture list] (params list) mutable exception-> return type { function body }
```

**[函数对象参数] (操作符重载函数参数) mutable 或 exception 声明 -> 返回值类型 {函数体}**

#### (1)  capture list ：捕获外部变量列表

​	

|  捕获形式   |                             说明                             |
| :---------: | :----------------------------------------------------------: |
|     []      |                      不捕获任何外部变量                      |
| [变量名, …] | 默认以**值**的形式捕获指定的多个外部变量（用逗号分隔），如果引用捕获，需要显示声明（使用&说明符） |
|   [this]    |                    以值的形式捕获this指针                    |
|     [=]     |                  以值的形式捕获所有外部变量                  |
|     [&]     |                  以引用形式捕获所有外部变量                  |
|   [=, &x]   |         变量x以引用形式捕获，其余变量以传值形式捕获          |
|   [&, x]    |         变量x以值的形式捕获，其余变量以引用形式捕获          |

#### (2) params list 形参列表

1. **参数列表中不能有默认参数**
2. **不支持可变参数**
3. **所有参数必须有参数名**

#### (3) mutable指示符

​	**用来说用是否可以修改捕获的变量,若想修改以值的形式传入的变量，则需要声明mutable关键字，该关键字用以说明表达式体内的代码可以修改值捕获的变量**

#### (4) exception：异常设定

**exception 声明用于指定函数抛出的异常，如抛出整数类型的异常，可以使用 throw(int)**

#### (5) return type : 返回类型

#### (6)function body : 函数体

```c++
//[函数对象参数] (操作符重载函数参数) mutable 或 exception 声明 -> 返回值类型 {函数体}

int main()
{
	std:: function<void()> call;
    
    call = [](){
        printf("This is test\n");
    };
    
    call();
    return 0;
}

/*---------------------输出-------------------------*/
//  This is test
```



### 3）优化

删除CELLTask类型，任务类型用匿名函数对象替代

#### (1) CellTaskServer 更改：

```c++
//执行任务的服务类型
class CellTaskServer 
{
	typedef std::function<void()> CellTask;
private:
	//任务数据
	std::list<CellTask> _tasks;
	//任务数据缓冲区
	std::list<CellTask> _tasksBuf;
	//改变数据缓冲区时需要加锁
```

#### (2) CellServer 更改：

```
void addSendTask(CellClient* pClient, netmsg_DataHeader* header)
{
	_taskServer.addTask([pClient, header]() {
		pClient->SendData(header);
		delete header;
	});
}
```

#### (3) 删除网络消息发送任务class CellSendMsg2ClientTask:public CellTask



# 七、添加心跳检测

## 1. 意义：

在外网复杂环境下，当某客户端断开连接后，服务端并不能实时感知，因此需要一个解决方案，即心跳检测

## 2. 心跳检测：

在客户端连接到服务端时，服务端为客户端创建一个死亡倒计时，客户端在此

时间内需向服务端发送一条网络消息（心跳消息） ，心跳消息可以是一条特定的消息，但为了简单起见，心跳消息可以是任意一条消息，若超过死亡计时仍未收到消息，则服务端将此客户端移除，避免浪费资源

### 3) 添加获取当前时间函数

```c++
class CELLTime
{
public:
	//获取当前时间戳 (毫秒)
	static time_t getNowInMilliSec()
	{
		return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}
};
```

### 4) 在CELLClient中添加死亡倒计时

```c++
//客户端心跳检测死亡计时时间
#define CLIENT_HREAT_DEAD_TIME 60000
//心跳死亡计时
time_t _dtHeart;

void resetDTHeart()
{
	_dtHeart = 0;
}

//心跳检测
bool checkHeart(time_t dt)
{
    //dt为两次检测的时间差，当前时间加上时间差等于最新的存活时间
	_dtHeart += dt;
    //若超过了死亡计时时间
	if (_dtHeart >= CLIENT_HREAT_DEAD_TIME)
	{
		printf("checkHeart dead:s=%d,time=%d\n",_sockfd, _dtHeart);
		return true;
	}
	return false;
}
```

### 5) 在CELLServer中添加死亡倒计时

在OnRun()成员函数中对所有客户端进行心跳检测，在OnRun()函数末尾调用CheckTime()

```c++
	//旧的时间戳
	time_t _oldTime = CELLTime::getNowInMilliSec();
	void CheckTime()
	{
		//当前时间戳
		auto nowTime = CELLTime::getNowInMilliSec();
        //两次检测的时间差
		auto dt = nowTime - _oldTime;
		_oldTime = nowTime;

		for (auto iter = _clients.begin(); iter != _clients.end(); )
		{
			if (iter->second->checkHeart(dt))
			{
				if (_pNetEvent)
					_pNetEvent->OnNetLeave(iter->second);
				_clients_change = true;
				delete iter->second;
				auto iterOld = iter;
				iter++;
				_clients.erase(iterOld);
				continue;
			}
			iter++;
		}
	}
```

### 6) 添加新的消息类型

```c++
enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_C2S_HEART,
	CMD_S2C_HEART,
	CMD_ERROR
};

struct netmsg_c2s_Heart : public netmsg_DataHeader
{
	netmsg_c2s_Heart()
	{
		dataLength = sizeof(netmsg_c2s_Heart);
		cmd = CMD_C2S_HEART;
	}
};

struct netmsg_s2c_Heart : public netmsg_DataHeader
{
	netmsg_s2c_Heart()
	{
		dataLength = sizeof(netmsg_s2c_Heart);
		cmd = CMD_S2C_HEART;
	}
};
```



### 7) MyServer中重写OnNetMsg

```c++
virtual void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header)
	{
		EasyTcpServer::OnNetMsg(pCellServer, pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			pClient->resetDTHeart();
			//send recv 
			netmsg_Login* login = (netmsg_Login*)header;
			//printf("收到客户端<Socket=%d>请求：CMD_LOGIN,数据长度：%d,userName=%s PassWord=%s\n", cSock, login->dataLength, login->userName, login->PassWord);
			//忽略判断用户密码是否正确的过程
			netmsg_LoginR ret;
			pClient->SendData(&ret);
			//netmsg_LoginR* ret = new netmsg_LoginR();
			//pCellServer->addSendTask(pClient, ret);
		}//接收 消息---处理 发送   生产者 数据缓冲区  消费者 
		break;
		case CMD_LOGOUT:
		{
			netmsg_Logout* logout = (netmsg_Logout*)header;
			//printf("收到客户端<Socket=%d>请求：CMD_LOGOUT,数据长度：%d,userName=%s \n", cSock, logout->dataLength, logout->userName);
			//忽略判断用户密码是否正确的过程
			//netmsg_LogoutR ret;
			//SendData(cSock, &ret);
		}
		break;
		case CMD_C2S_HEART:
		{
			pClient->resetDTHeart();
			netmsg_s2c_Heart ret;
			pClient->SendData(&ret);
		}
        break;
		default:
		{
			printf("<socket=%d>收到未定义消息,数据长度：%d\n", pClient->sockfd(), header->dataLength);
			//netmsg_DataHeader ret;
			//SendData(cSock, &ret);
		}
		break;
		}
	}
```



# 八、添加定时发送缓存数据

在CELLServer中添加

```c++ 
    //在间隔指定时间后
    //把发送缓冲区内缓存的消息数据发送给客户端
    #define CLIENT_SEND_BUFF_TIME 200
	
	void resetDTSend()
	{
		_dtSend = 0;
	}

//定时发送消息检测
	bool checkSend(time_t dt)
	{
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME)
		{
			//printf("checkSend:s=%d,time=%d\n", _sockfd, _dtSend);
			//立即将发送缓冲区的数据发送出去
			SendDataReal();
			//重置发送计时
			resetDTSend();
			return true;
		}
		return false;
	}

	//上次发送消息数据的时间
	time_t _dtSend;
```

 在CELLServer中调用

```c++
	//旧的时间戳
	time_t _oldTime = CELLTime::getNowInMilliSec();
	void CheckTime()
	{
		//当前时间戳
		auto nowTime = CELLTime::getNowInMilliSec();
		auto dt = nowTime - _oldTime;
		_oldTime = nowTime;

		for (auto iter = _clients.begin(); iter != _clients.end(); )
		{
			//心跳检测
			if (iter->second->checkHeart(dt))
			{
				if (_pNetEvent)
					_pNetEvent->OnNetLeave(iter->second);
				_clients_change = true;
				delete iter->second;
				auto iterOld = iter;
				iter++;
				_clients.erase(iterOld);
				continue;
			}
			//定时发送检测
			iter->second->checkSend(dt);
			iter++;
		}
	}

```

 

# 九、封装简单的信号量来控制Sever的关闭

```c++
#ifndef _CELL_SEMAPHORE_HPP_
#define _CELL_SEMAPHORE_HPP_

#include<chrono>
#include<thread>

//信号量
class CELLSemaphore
{
public:
	void wait()
	{
		_isWaitExit = true;
		//阻塞等待OnRun退出
		while (_isWaitExit)
		{//信号量
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
		}
	}
	//
	void wakeup()
	{
		if (_isWaitExit)
			_isWaitExit = false;
		else
			printf("CELLSemaphore wakeup error.");
	}

private:
	bool	_isWaitExit = false;
};

#endif // !_CELL_SEMAPHORE_HPP_
```



# 十、select模型异步发送数据

本方案中，SOCKET设置为阻塞模式下，在阻塞模式下，在I/O操作完成前，执行的操作函数一直等候而不会立即返回，该函数所在的线程会阻塞在这里，因此当有一个客户端阻塞时，其他客户端都无法正常收发数据，造成阻塞。



## 发送数据逻辑

目前的发送数据逻辑为：

​		**判断发送缓冲区是否已经写满，若没有写满，则将数据放入缓冲区，若缓冲区写满了，则将缓冲区中的数据发送出去，再将未发送的部分加入缓冲区，同时，还有定时发送功能，即达到特定时间后对缓冲区数据进行一次发送**

- 缓冲区满了——进行发送
- 时间到了——进行发送

**进行发送时，并未检测此时socket是否可写，若此时socket不可写，则send会阻塞**

而之所以会选择这种发送方式，在于其有如下优点：

- 保证数据随时可写，并确定在接收端正常情况下数据一定发送出去，不会存在接收端正常，但由于缓冲区满了导致数据发不出去的情况，**只是若此刻socket不可写，send会处于阻塞的状态**，控制收发过程较为简单

**本机测试环境可控，且客户端由自己编写，当数据发送异常时便断开连接，客户端处于可控的状态，一定会对数据进行接收，因此采用此方式不会出现异常，但若是面对不可控的客户端，此方式便不可行，一个客户端阻塞会导致其他所有客户端阻塞**

若想满足异步发送，则不能采用此逻辑

```c++
//若要发送的数据超过了缓冲区剩余的容量，则返回SOCKET_ERROR,提示错误，否则将数据放入缓冲区
//缓冲区的控制根据业务需求的差异而调整
	//发送数据
	int SendData(netmsg_DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		//要发送的数据长度
		int nSendLen = header->dataLength;
		//要发送的数据
		const char* pSendData = (const char*)header;

		if (_lastSendPos + nSendLen <= SEND_BUFF_SZIE)
		{
			//将要发送的数据 拷贝到发送缓冲区尾部
			memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
			//计算数据尾部位置
			_lastSendPos += nSendLen;

			if (_lastSendPos == SEND_BUFF_SZIE)
			{
				_sendBuffFullCount++;
			}

			return nSendLen;
		}else{
			_sendBuffFullCount++;
		}
		return ret;
	}
```

**同时，在CELLServer中添加socket可写检测**

```c++
	void WriteData(fd_set& fdWrite)
	{
#ifdef _WIN32
		for (int n = 0; n < fdWrite.fd_count; n++)
		{
			auto iter = _clients.find(fdWrite.fd_array[n]);
			if (iter != _clients.end())
			{
				if (-1 == iter->second->SendDataReal())
				{
					OnClientLeave(iter->second);
					_clients.erase(iter);
				}
			}
		}
#else
		for (auto iter = _clients.begin(); iter != _clients.end(); )
		{
			if (FD_ISSET(iter->second->sockfd(), &fdWrite))
			{
				if (-1 == iter->second->SendDataReal())
				{
					OnClientLeave(iter->second);
					auto iterOld = iter;
					iter++;
					_clients.erase(iterOld);
					continue;
				}
			}
			iter++;
		}
#endif
	}
```

 

# 十一、封装消息缓冲区

为了以后更改网络模型以后，也能方便的使用消息缓冲区，决定将消息缓冲区封装起来

```c++
#ifndef _CELL_BUFFER_HPP_
#define _CELL_BUFFER_HPP_

#include"CELL.hpp"

class CELLBuffer
{
public:
	CELLBuffer(int nSize = 8192)
	{
		_nSize = nSize;
		_pBuff = new char[_nSize];
	}

	~CELLBuffer()
	{
		if (_pBuff)
		{
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}

	char* data()
	{
		return _pBuff;
	}

	bool push(const char* pData, int nLen)
	{
		////写入大量数据不一定要放到内存中
		////也可以存储到数据库或者磁盘等存储器中
		//if (_nLast + nLen > _nSize)
		//{
		//	//需要写入的数据大于可用空间
		//	int n = (_nLast + nLen) - _nSize;
		//	//拓展BUFF
		//	if (n < 8192)
		//		n = 8192;
		//	char* buff = new char[_nSize+n];
		//	memcpy(buff, _pBuff, _nLast);
		//	delete[] _pBuff;
		//	_pBuff = buff;
		//}

		if (_nLast + nLen <= _nSize)
		{
			//将要发送的数据 拷贝到发送缓冲区尾部
			memcpy(_pBuff + _nLast, pData, nLen);
			//计算数据尾部位置
			_nLast += nLen;

			if (_nLast == SEND_BUFF_SZIE)
			{
				++_fullCount;
			}

			return true;
		}
		else {
			++_fullCount;
		}

		return false;
	}

	void pop(int nLen)
	{
		int n = _nLast - nLen;
		if (n > 0)
		{
			memcpy(_pBuff, _pBuff + nLen, n);
		}
		_nLast = n;
		if (_fullCount > 0)
			--_fullCount;
	}

	int write2socket(SOCKET sockfd)
	{
		int ret = 0;
		//缓冲区有数据
		if (_nLast > 0 && INVALID_SOCKET != sockfd)
		{
			//发送数据
			ret = send(sockfd, _pBuff, _nLast, 0);
			//数据尾部位置清零
			_nLast = 0;
			//
			_fullCount = 0;
		}
		return ret;
	}

	int read4socket(SOCKET sockfd)
	{
		if (_nSize - _nLast > 0)
		{
			//接收客户端数据
			char* szRecv = _pBuff + _nLast;
			int nLen = (int)recv(sockfd, szRecv, _nSize - _nLast, 0);
			//printf("nLen=%d\n", nLen);
			if (nLen <= 0)
			{
				return nLen;
			}
			//消息缓冲区的数据尾部位置后移
			_nLast += nLen;
			return nLen;
		}
		return 0;
	}

	bool hasMsg()
	{
		//判断消息缓冲区的数据长度大于消息头netmsg_DataHeader长度
		if (_nLast >= sizeof(netmsg_DataHeader))
		{
			//这时就可以知道当前消息的长度
			netmsg_DataHeader* header = (netmsg_DataHeader*)_pBuff;
			//判断消息缓冲区的数据长度大于消息长度
			return _nLast >= header->dataLength;
		}
		return false;
	}
private:
	//第二缓冲区 发送缓冲区
	char* _pBuff = nullptr;
	//可以用链表或队列来管理缓冲数据块
	//list<char*> _pBuffList;
	//缓冲区的数据尾部位置，已有数据长度
	int _nLast = 0;
	//缓冲区总的空间大小，字节长度
	int _nSize = 0;
	//缓冲区写满次数计数
	int _fullCount = 0;
};

#endif // !_CELL_BUFFER_HPP_

```



# 十二、添加运行日志

```c++
#ifndef _CELL_LOG_HPP_
#define _CELL_LOG_HPP_

#include"CELL.hpp"
#include"CELLTask.hpp"
#include<ctime>
class CELLLog
{
	//Info
	//Debug
	//Warring
	//Error
public:
	CELLLog()
	{
		_taskServer.Start();
	}

	~CELLLog()
	{
		_taskServer.Close();
		if (_logFile)
		{
			Info("CELLLog fclose(_logFile)\n");
			fclose(_logFile);
			_logFile = nullptr;
		}
	}
public:
	static CELLLog& Instance()
	{
		static  CELLLog sLog;
		return sLog;
	}

	void setLogPath(const char* logPath, const char* mode)
	{
		if (_logFile)
		{
			Info("CELLLog::setLogPath _logFile != nullptr\n");
			fclose(_logFile);
			_logFile = nullptr;
		}
			

		_logFile = fopen(logPath, mode);
		if (_logFile)
		{
			Info("CELLLog::setLogPath success,<%s,%s>\n", logPath, mode);
		}
		else {
			Info("CELLLog::setLogPath failed,<%s,%s>\n", logPath, mode);
		}
	}

	static void Info(const char* pStr)
	{
		CELLLog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			if (pLog->_logFile)
			{
				auto t = system_clock::now();
				auto tNow = system_clock::to_time_t(t);
				//fprintf(pLog->_logFile, "%s", ctime(&tNow));
				std::tm* now = std::gmtime(&tNow);
				fprintf(pLog->_logFile, "%s", "Info ");
				fprintf(pLog->_logFile, "[%d-%d-%d %d:%d:%d]", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
				fprintf(pLog->_logFile, "%s", pStr);
				fflush(pLog->_logFile);
			}
			printf("%s", pStr);
		});
	}

	template<typename ...Args>
	static void Info(const char* pformat, Args ... args)
	{
		CELLLog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			if (pLog->_logFile)
			{
				auto t = system_clock::now();
				auto tNow = system_clock::to_time_t(t);
				//fprintf(pLog->_logFile, "%s", ctime(&tNow));
				std::tm* now = std::gmtime(&tNow);
				fprintf(pLog->_logFile, "%s", "Info ");
				fprintf(pLog->_logFile, "[%d-%d-%d %d:%d:%d]", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
				fprintf(pLog->_logFile, pformat, args...);
				fflush(pLog->_logFile);
			}
			printf(pformat, args...);
		});
	}
private:
	FILE* _logFile = nullptr;
	CELLTaskServer _taskServer;
};

#endif // !_CELL_LOG_HPP_

```

