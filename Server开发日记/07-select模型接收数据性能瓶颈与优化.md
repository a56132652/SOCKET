# select模型接收数据性能瓶颈与优化



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

为了更方便的使用生产者-消费者设计模式，采用了将任务抽象出来的方法，将任务定义为一个类，这里先定义一个任务基类`CELLTask`，之后所有的具体任务类都以`CELLTask`为基类。

### 6.1 定义任务基类 CELLTask

- 与之前的客户端连接不同，任务会频繁的增加减少，因此采用链表`list`存储任务

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

### 6.2 定义任务服务类型 CELLTaskServer

**该类与消息处理类`CellServer`功能类似**

**根据生产者——消费者模式，该类需要一个正式任务队列以及一个缓冲任务队列，以及操作临界资源时的锁**

**操作方法：**

- 添加任务`addTask()`:该操作需要加锁
  - 添加任务至缓冲区
- 启动工作线程`Start()`
- 工作函数`OnRun()`:处理任务
  - 从缓冲队列中取出任务至正式队列
  - 然后处理任务
  - 任务处理完成后清空正是队列

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

### 6.3 分离向客户端发送数据业务

#### （1）定义网络消息发送任务

- 该类继承于任务基类 `CellTask`

- 对于发送消息任务，需要知道具体往哪个客户端对象发送消息，以及发送什么消息

  - 定义成员

    ```c++
    //客户端对象	
    ClientSocket* _pClient;
    //要发送的消息
    DataHeader* _pHeader;
    ```

- 重写`doTask()`

  - 


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

```c++
CeLLTaskServer _taskServer;
```

#### （3）调用

```c++
LoginResult* ret = new LoginResult();
pCellServer->addSendTask(pClient, ret);
```

### 6.4 隐患

频繁的申请与释放内存，降低了效率，还可能形成内存碎片

![image-20211016225851183](C:\Users\Sakura\AppData\Roaming\Typora\typora-user-images\image-20211016225851183.png)

