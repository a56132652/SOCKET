# 优化Server可写检测性能

对于客户端，进行了当前客户端可不可写的判断

```c++
if (_pClient->needWrite())
{
    FD_SET(_sock, &fdWrite);
    ret = select(_sock + 1, &fdRead, &fdWrite, nullptr, &t);
}
    else {
    ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
}
```

但是对于服务端，并未对当前是否可写进行判断，因为目前服务端处于一个高频发送接收的状态，目的是为了达到百万并发，在这种高频发送状态下，默认服务端一直需要写。

**但现在考虑客户端低频发送的情况**

## 1. 当前客户端接收逻辑

```c++
//CellServer
//延迟时间一毫秒
timeval t{ 0,1 };
int ret = select(_maxSock + 1, &fdRead, &fdWrite, nullptr, &t);
```

1. 首先检测fdRead中是否有socket可读，若没有数据可读，则select进程休眠1ms
2. 而对于fdWrite,只要连接建立，正常情况下客户端一直处于可写状态，即每次检测fdWrite会立即返回，表示可写

**由此造成了一个问题，首先检测fdRead时，发现无数据可读，然后延迟1ms,但是继而检测fdWrite时，会立即返回，1ms的超时没有任何意义，并且由于目前并不需要写数据，该段程序（OnRun()）会变成一个死循环，导致CPU占有率急剧上升。因此需要判断当前是否需要写数据，若需要写数据，再去判断当前是否可写**

## 2.代码更改

在CellServer中更改相应代码如下：

```c++
	bool bNeedWrite = false;
			FD_ZERO(&fdWrite);
			for (auto iter : _clients)
			{
				//需要写数据的客户端才加入fd_set检测是否可写
				if (iter.second->needWrite()) {
					bNeedWrite = true;
					FD_SET(iter.second->sockfd(), &fdWrite);
				}
			}
			//memcpy(&fdWrite, &_fdRead_back, sizeof(fd_set));
			//memcpy(&fdExc, &_fdRead_back, sizeof(fd_set));


			timeval t{ 0,1 };
			int ret = 0;
			if (bNeedWrite)
			{
				ret = select(_maxSock + 1, &fdRead, &fdWrite, nullptr, &t);
			}
			else {
				ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, &t);
			}
```

