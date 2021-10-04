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

   



