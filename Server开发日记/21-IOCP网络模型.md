# 	IOCP网络模型

 

![网络通信引擎应用select模型](F:\A3-git_repos\SOCKET\Server开发日记\网络通信引擎应用select模型-16467430499261.png)



![在网络通信引擎中应用epoll网络模型](F:\A3-git_repos\SOCKET\在网络通信引擎中应用epoll网络模型.png)



![在网络通信引擎中应用IOCP网络模型](F:\A3-git_repos\SOCKET\Server开发日记\在网络通信引擎中应用IOCP网络模型-16472583242552.png)





## 1. 创建Socket

```c++
//当使用socket函数创建套接字时，会默认设置WSA_FLAG_OVERLAPPED标志
SOCKET sockServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//注意这里也可以用 WSASocket函数创建socket
//最后一个参数需要设置为重叠标志（WSA_FLAG_OVERLAPPED）
SOCKET sockServer = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
```

**也可以使用`WSASocket()`创建SOCKET，考虑到跨平台的需求，这里使用普通socket()函数创建**

## 2. 设置对外IP与端口信息并绑定sockaddr与ServerSocket

```c++
	// 2.1 设置对外IP与端口信息 
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);//host to net unsigned short
	_sin.sin_addr.s_addr = INADDR_ANY;
	// 2.2 绑定sockaddr与ServerSocket
	if (SOCKET_ERROR == bind(sockServer, (sockaddr*)&_sin, sizeof(_sin)))
	{
		printf("错误,绑定网络端口失败...\n");
	}
	else {
		printf("绑定网络端口成功...\n");
	}
```



## 3. 监听ServerSocket

```c++
	// 3 监听ServerSocket
	if (SOCKET_ERROR == listen(sockServer, 64))
	{
		printf("错误,监听网络端口失败...\n");
	}
	else {
		printf("监听网络端口成功...\n");
	}
```



## 4. 创建完成端口

```c++
//功能：创建完成端口和关联完成端口
 HANDLE WINAPI CreateIoCompletionPort(
     *    __in   HANDLE FileHandle,              // 已经打开的文件句柄或者空句柄，一般是客户端的句柄
     *    __in   HANDLE ExistingCompletionPort,  // 已经存在的IOCP句柄
     *    __in   ULONG_PTR CompletionKey,        // 完成键，包含了指定I/O完成包的指定文件
     *    __in   DWORD NumberOfConcurrentThreads // 真正并发同时执行最大线程数，一般推介是CPU核心数*2
     * );
```



**`  HANDLE _completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);`**



## **5. 关联IOCP与ServerSo**cket

```c++
 HANDLE WINAPI CreateIoCompletionPort(  
    __in      HANDLE  FileHandle,             // 这里当然是连入的这个套接字句柄了   
     __in_opt  HANDLE  ExistingCompletionPort, // 这个就是前面创建的那个完成端口   
     __in      ULONG_PTR CompletionKey,        // 这个参数就是类似于线程参数一样，在   
                                               // 绑定的时候把自己定义的结构体指针传递   
                                               // 这样到了Worker线程中，也可以使用这个   
                                               // 结构体的数据了，相当于参数的传递   
     __in      DWORD NumberOfConcurrentThreads // 这里同样置0   
);  
```

- FileHandle：是我们要监测的文件句柄，注意是这个句柄不一定非得是socket，ICOP可以监测很多异步调用的返回，不过在这里我们只用来做网络IO复用，所以传递的是已经准备好的socket(一定是listen调用之后的或者accept接收完成的,而且是异步的socket)
- ExistingCompletionPort：第一步调用时候返回的IOCP的句柄
- CompletionKey：这个其实是IOCP交给用户保存网络会话上下文的一个渠道，类似于epoll中epoll_event的data成员，当我们阻塞监测到接收到新事件到来时，会从IOCP中得到这个我们传进去的值。这个只是保存网络会话上下文的其中之一，一会儿我们会看到还可以通过别的方式保存针对每个连接类似于上下文这样的信息。其类型是ULONG_PTR，所以我们可以传递任意指针进去

**此时完成键参数CompletionKey是一个普通的sockServer**

```c++
	auto ret = CreateIoCompletionPort((HANDLE)sockServer, 
                                      _completionPort, 	 
                                      (ULONG_PTR)sockServer,
                                      0);
	if (!ret)
	{
		printf("关联IOCP与ServerSocket失败,错误码=%d\n", GetLastError());
		return -1;
	}
```

## 6. 向IOCP投递接收连接的任务

```c++
#include <MSWSock.h>
BOOL AcceptEx (       
    SOCKET sListenSocket,   
    SOCKET sAcceptSocket,   
    PVOID lpOutputBuffer,   
    DWORD dwReceiveDataLength,   
    DWORD dwLocalAddressLength,   
    DWORD dwRemoteAddressLength,   
    LPDWORD lpdwBytesReceived,   
    LPOVERLAPPED lpOverlapped   
);  
```

- 参数1--sListenSocket, 用来监听的Socket了；
- 参数2--sAcceptSocket, 用于接受连接的socket，是AcceptEx高性能的关键所在。
  - `AcceptEx()`与普通`accept()`相比，它是先创建好一个socketA，当有客户端连接申请时，`AcceptEx()`直接将socketA与该客户端绑定,减少了连接过程中的资源消耗，提升了通信效率
- 参数3--lpOutputBuffer,接收缓冲区，这也是AcceptEx比较有特色的地方，既然AcceptEx不是普通的accpet函数，那么这个缓冲区也不是普通的缓冲区，这个缓冲区包含了三个信息：一是客户端发来的第一组数据，二是server的地址，三是client地址
- 参数4--dwReceiveDataLength，**参数`lpOutputBuffer`**中用于**存放数据的空间大小**。如果此参数=0，则Accept时将不会等待数据到来，而直接返回，如果此参数不为0，那么一定得等接收到数据了才会返回…… 所以通常当需要Accept接收数据时，就需要将该参数设成为：`sizeof(lpOutputBuffer) - 2*(sizeof sockaddr_in +16)`，即**总长度减去两个地址空间的长度**
- 参数5--dwLocalAddressLength，存放本地址地址信息的空间大小；
- 参数6--dwRemoteAddressLength，存放本远端地址信息的空间大小；
- 参数7--lpdwBytesReceived，out参数，对我们来说没用，不用管；
- 参数8--lpOverlapped，本次重叠I/O所要用到的重叠结构。

```c++
SOCKET socketClient = socket(AF_INET， SOCK_STREAM，IPPROTO_TCP);
char buffer[1024] = {};
OVERLAPPED overlapped = {};
if(FALSE == AcceptEx(sockServer,
                     sockClient,
                     buffer,
                     100,
                     sizeof(sockaddr_in + 16),
                     sizeof(sockaddr_in + 16),
                     NULL,
                     &overlapped
                    ))
{
    int err = WSAGetLastError();
    if(ERROR_IO_PENDING != err)
    {
        printf("AcceptEx failed with error %d\n", err);
        return 0;
    }
}
AcceptEx()
```



## 7  获取已完成的任务

使用GetQueuedCompletionStatus监控完成端口

```c++
BOOL WINAPI GetQueuedCompletionStatus(  
    __in   HANDLE          CompletionPort,    // 这个就是我们建立的那个唯一的完成端口   
    __out  LPDWORD         lpNumberOfBytes,   //该事件从socket中读取或写入的数据，注意这里和epoll有巨大区别，epoll是事件触发之后，我们自己来负责读写数据，而IOCP则是数据读写完成后再来通知我们(Reactor和Proactor的差别)。  
    __out  PULONG_PTR      lpCompletionKey,   // 这个是我们建立完成端口的时候绑定的那个自定义结构体参数   
    __out  LPOVERLAPPED    *lpOverlapped,     // 这个是我们在连入Socket的时候一起建立的那个重叠结构   
    __in   DWORD           dwMilliseconds     // 等待完成端口的超时时间，如果线程不需要做其他的事情，那就INFINITE就行了   
    );  
```

- CompletionPort ： IOCP句柄

- lpNumberOfBytes：该事件从socket中读取或写入的数据，注意这里和epoll有巨大区别，epoll是事件触发之后，我们自己来负责读写数据，而IOCP则是数据读写完成后再来通知我们(Reactor和Proactor的差别)。

- lpCompletionKey：第二次调用CreateIoCompletionPort时传递的第三个参数，我们可以从这个参数里获取到网络连接的上下文之类的信息

- lpOverlapped：Windows定义好的结构体，叫做重叠结构体，这里我们可以自定义一个结构体，成员变量中包含一个OVERLAPPED类型的变量，在IOCP事件返回时通过CONTAINING_RECORD宏来获取自定义结构体的指针，这是第二个可以绑定用户数据(连接上下文的地方，另外一个是GetQueuedCompletionStatus绑定socket时传递的第三个参数)。

  - ```c++
    typedef struct _OVERLAPPED {
        ULONG_PTR Internal;
        ULONG_PTR InternalHigh;
        union {
            struct {
                DWORD Offset;
                DWORD OffsetHigh;
            } DUMMYSTRUCTNAME;
            PVOID Pointer;
        } DUMMYUNIONNAME;
    
        HANDLE  hEvent;
    } OVERLAPPED, *LPOVERLAPPED;
    ```

    

`GetQueuedCompletionStatus()`会让Worker线程进入不占用CPU的睡眠状态，直到完成端口上出现了需要处理的网络操作或者超出了等待的时间限制为止

```c++
//绑定完成端口
auto ret = CreateIoCompletionPort((HANDLE)sockServer, 
                                      _completionPort, 	 
                                      (ULONG_PTR)sockServer,
                                      0);
```

- 由于关联完成端口时，完成键参数是 sockServer

- 当`GetQueuedCompletionStatus()`成功执行返回时，通过`if(sockServer == sock)`判断有新连接，客户端socket保存在事先定义的 clientSocket中

- ```c++
  SOCKET socketClient = socket(AF_INET， SOCK_STREAM，IPPROTO_TCP);
  ```





## 8 关联完成端口与clientSocket

**参数 lpCompletionKey就是建立完成端口的时候绑定的那个自定义结构体参数 ，如下：**

```c++
#define nClient 10
enum IO_TYPE
{
	ACCEPT = 10,
	RECV,
	SEND
};

//数据缓冲区空间大小
#define DATA_BUFF_SIZE 1024
struct IO_DATA_BASE
{
	//重叠体
	OVERLAPPED overlapped;
	//
	SOCKET sockfd;
	//数据缓冲区
	char buffer[DATA_BUFF_SIZE];
	//实际缓冲区数据长度
	int length;
	//操作类型
	IO_TYPE iotype;
};
```

**向IOCP投递接受连接的任务**

```c++
//向IOCP投递接受连接的任务
void postAccept(SOCKET sockServer, IO_DATA_BASE* pIO_DATA)
{
	pIO_DATA->iotype = IO_TYPE::ACCEPT;
	pIO_DATA->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (FALSE == AcceptEx(sockServer
		, pIO_DATA->sockfd
		, pIO_DATA->buffer
		, 0
		, sizeof(sockaddr_in) + 16
		, sizeof(sockaddr_in) + 16
		, NULL
		, &pIO_DATA->overlapped
	))
	{
		int err = WSAGetLastError();
		if (ERROR_IO_PENDING != err)
		{
			printf("AcceptEx failed with error %d\n", err);
			return;
		}
	}
}
```

  由于是异步操作，我们是在线程启动的地方投递的这个操作， 等我们再次见到这些个变量的时候，就已经是在Worker线程内部了，因为Windows会直接把操作完成的结果传递到Worker线程里，这样咱们在启动的时候投递了那么多的IO请求，这从Worker线程传回来的这些结果，到底是对应着哪个IO请求的呢？

因此，Windows内核用一个**标志**来绑定每一个IO操作，这样到了Worker线程内部的时候，收到网络操作完成的通知之后，再通过这个标志来找出这组返回的数据到底对应的是哪个IO操作的。

这里的标志就是我们在上文定义的结构体**IO_DATA_BASE**

在完成端口的世界里，这个结构体有个专属的名字“单IO数据”，也就是说每一个重叠I/O都要对应的这么一组参数，至于这个结构体怎么定义无所谓，而且这个结构体也不是必须要定义的，但是没它……还真是不行，我们可以把它理解为线程参数，就好比你使用线程的时候，线程参数也不是必须的，但是不传还真是不行

### **使用GetQueuedCompletionStatus()监控完成端口状态**

****

```c++
/* BOOL WINAPI GetQueuedCompletionStatus(  
    __in   HANDLE          CompletionPort,    // 这个就是我们建立的那个唯一的完成端口   
    __out  LPDWORD         lpNumberOfBytes,   //这个是操作完成后返回的字节数   
    __out  PULONG_PTR      lpCompletionKey,   // 这个是我们建立完成端口的时候绑定的那个自定义结构体参数   
    __out  LPOVERLAPPED    *lpOverlapped,     // 这个是我们在连入Socket的时候一起建立的那个重叠结构   
    __in   DWORD           dwMilliseconds     // 等待完成端口的超时时间，如果线程不需要做其他的事情，那就INFINITE就行了   
    );  
*/
DWORD bytesTrans = 0;
		SOCKET sock = INVALID_SOCKET;
		IO_DATA_BASE* pIOData;
		
		if (FALSE == GetQueuedCompletionStatus(_completionPort, &bytesTrans, (PULONG_PTR)&sock, (LPOVERLAPPED*)&pIOData, 1))
		{
			int err = GetLastError();
			if (WAIT_TIMEOUT == err)
			{
				continue;
			}
			if (ERROR_NETNAME_DELETED == err)
			{
				printf("关闭 sockfd=%d\n", pIOData->sockfd);
				closesocket(pIOData->sockfd);
				continue;
			}
			printf("GetQueuedCompletionStatus failed with error %d\n", err);
			break;
		}

```

**当GetQueuedCompletionStatus()返回时，通过自定义结构体中的数据判断操作类型**

```c++
//7.1 接受链接 完成
		if (IO_TYPE::ACCEPT == pIOData->iotype)
		{
			printf("新客户端加入 sockfd=%d\n", pIOData->sockfd);
			//7.2 关联IOCP与ClientSocket
			auto ret = CreateIoCompletionPort((HANDLE)pIOData->sockfd, _completionPort, (ULONG_PTR)pIOData->sockfd, 0);
			if (!ret)
			{
				printf("关联IOCP与ClientSocket=%d失败\n", pIOData->sockfd);
				closesocket(pIOData->sockfd);
				continue;
			}
			//7.3 向IOCP投递接收数据任务
			postRecv(pIOData);
		}
		//8.1 接收数据 完成 Completion
		else if (IO_TYPE::RECV == pIOData->iotype)
		{
			if (bytesTrans <= 0)
			{//客户端断开处理
				printf("关闭 sockfd=%d, RECV bytesTrans=%d\n", pIOData->sockfd, bytesTrans);
				closesocket(pIOData->sockfd);
				continue;
			}
			printf("收到数据: sockfd=%d, bytesTrans=%d msgCount=%d\n", pIOData->sockfd, bytesTrans, ++msgCount);
			pIOData->length = bytesTrans;
			//8.2 向IOCP投递发送数据任务
			postSend(pIOData);
		}
		//9.1 发送数据 完成 Completion
		else if (IO_TYPE::SEND == pIOData->iotype)
		{
			if (bytesTrans <= 0)
			{//客户端断开处理
				printf("关闭 sockfd=%d, SEND bytesTrans=%d\n", pIOData->sockfd, bytesTrans);
				closesocket(pIOData->sockfd);
				continue;
			}
			printf("发送数据: sockfd=%d, bytesTrans=%d msgCount=%d\n", pIOData->sockfd, bytesTrans, msgCount);
			//9.2 向IOCP投递接收数据任务
			postRecv(pIOData);
		}
		else {
			printf("未定义行为 sockfd=%d", sock);
		}
```



## 9 投递接收数据请求

```c++
int WSARecv(  
    SOCKET s,                      // 当然是投递这个操作的套接字   
     LPWSABUF lpBuffers,            // 接收缓冲区    
                                        // 这里需要一个由WSABUF结构构成的数组   
     DWORD dwBufferCount,           // 数组中WSABUF结构的数量，设置为1即可   
     LPDWORD lpNumberOfBytesRecvd,  // 如果接收操作立即完成，这里会返回函数调用所接收到的字节数   
     LPDWORD lpFlags,               // 说来话长了，我们这里设置为0 即可   
     LPWSAOVERLAPPED lpOverlapped,  // 这个Socket对应的重叠结构   
     NULL                           // 这个参数只有完成例程模式才会用到，   
                                        // 完成端口中我们设置为NULL即可   
);  
```

- **LPWSABUF lpBuffers：需要我们自己new 一个 WSABUF 的结构体传进去**

```c++
typedef struct _WSABUF {  
               ULONG len; /* the length of the buffer */  
               __field_bcount(len) CHAR FAR *buf; /* the pointer to the buffer */  
  
        } WSABUF, FAR * LPWSABUF; 
```

- **LPWSAOVERLAPPED lpOverlapped：重叠结构**
  - 自定义一个重叠结构传入，请求结束时，这个重叠结构中会被分配有效的系统参数

```c++
	//向IOCP投递接收数据的任务
void postRecv(IO_DATA_BASE* pIO_DATA)
{
	pIO_DATA->iotype = IO_TYPE::RECV;
	WSABUF wsBuff = {};
	wsBuff.buf = pIO_DATA->buffer;
	wsBuff.len = DATA_BUFF_SIZE;
	DWORD flags = 0;
	ZeroMemory(&pIO_DATA->overlapped, sizeof(OVERLAPPED));

	if(SOCKET_ERROR == WSARecv(pIO_DATA->sockfd, &wsBuff, 1, NULL, &flags, &pIO_DATA->overlapped, NULL))
	{
		int err = WSAGetLastError();
		if (ERROR_IO_PENDING != err)
		{
			printf("WSARecv failed with error %d\n", err);
			return;
		}
	}
}
```



## 10 投递发送数据请任务

```c++
//向IOCP投递发送数据的任务
void postSend(IO_DATA_BASE* pIO_DATA)
{
	pIO_DATA->iotype = IO_TYPE::SEND;
	WSABUF wsBuff = {};
	wsBuff.buf = pIO_DATA->buffer;
	wsBuff.len = pIO_DATA->length;
	DWORD flags = 0;
	ZeroMemory(&pIO_DATA->overlapped, sizeof(OVERLAPPED));

	if (SOCKET_ERROR == WSASend(pIO_DATA->sockfd, &wsBuff, 1, NULL, flags, &pIO_DATA->overlapped, NULL))
	{
		int err = WSAGetLastError();
		if (ERROR_IO_PENDING != err)
		{
			printf("WSASend failed with error %d\n", err);
			return;
		}
	}
}	
```



## 11 预加载AcceptEx()

**使用`WSAloctl将AcceptEx加载到内存中`**

```c++
//将AcceptEx函数加载内存中，调用效率更高
LPFN_ACCEPTEX lpfnAcceptEx = NULL;
void loadAcceptEx(SOCKET ListenSocket)
{
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes = 0;
	int iResult = WSAIoctl(ListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx, sizeof(GuidAcceptEx),
		&lpfnAcceptEx, sizeof(lpfnAcceptEx),
		&dwBytes, NULL, NULL);

	if (iResult == SOCKET_ERROR) {
		printf("WSAIoctl failed with error: %u\n", WSAGetLastError());
	}
}
```

