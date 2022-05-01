# select模型异步发送数据

本方案中，SOCKET设置为阻塞模式下，在阻塞模式下，在I/O操作完成前，执行的操作函数一直等候而不会立即返回，该函数所在的线程会阻塞在这里，因此当有一个客户端阻塞时，其他客户端都无法正常收发数据，造成阻塞。

## 1. 发送数据逻辑

目前的发送数据逻辑为：

​		**判断发送缓冲区是否已经写满，若没有写满，则将数据放入缓冲区，若缓冲区写满了，则将缓冲区中的数据发送出去，再将未发送的部分加入缓冲区，同时，还有定时发送功能，即达到特定时间后对缓冲区数据进行一次发送**

- 缓冲区满了——进行发送
- 时间到了——进行发送

**进行发送时，并未检测此时socket是否可写，若此时socket不可写，则send会阻塞**

而之所以会选择这种发送方式，在于其有如下优点：

- 保证数据随时可写，并确定在接收端正常情况下数据一定发送出去，不会存在接收端正常，但由于缓冲区满了导致数据发不出去的情况，**只是若此刻socket不可写，send会处于阻塞的状态**，控制收发过程较为简单

**本机测试环境可控，且客户端由自己编写，当数据发送异常时便断开连接，客户端处于可控的状态，一定会对数据进行接收，因此采用此方式不会出现异常，但若是面对不可控的客户端，此方式便不可行，一个客户端阻塞会导致其他所有客户端阻塞**

若想满足异步发送，则不能采用此逻辑

## 2. select添加可写查询

```c++
fd_set fdWrite;
memcpy(&fdWrite, &_fdRead_bak, sizeof(fd_set));
int ret = select(_maxSock + 1, &fdRead, &fdWrite, nullptr, &t);
```

## 3. 更改发送数据逻辑

- 在同步阻塞模式下，当消息长度大于缓冲区剩余长度时，先将缓冲区填满并发送，然后将剩余未发送的消息数据加入缓冲区
  - 这种模式下除非客户端异常，是一定可以发送出去的
- 如要满足异步，则只有缓冲区未满时才可以发送数据
  - 发送时还要检测客户端是否可写

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

## 4. 同时，在CELLServer中添加socket可写检测

- 遍历`fdWrite`中的元素，直接向其发送数据，若消息发送不成功，说明客户端异常，直接清除

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

 