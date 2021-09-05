#ifndef _CELLSERVER_HPP_
#define _CELLSERVER_HPP_

#include "Cell.hpp"
#include "INetEvent.hpp"
#include "CELLTask.hpp"
#include "MessageHeader.hpp"
#include <map>
#include <vector>

/********************************************************************************************************************************/
/*****************************---------------------CellServer(消息处理类)--------------------************************************/
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
	//关闭socket
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
			// 8 关闭套接字closesocket
			closesocket(_sock);

			//-----------------
			//清除Windows socket环境
			WSACleanup();							//关闭Socket网络环境
#else
			for (auto iter : _clients)
			{
				close(iter.second->sockfd());
				delete iter.second;
			}
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
	//int _nCount = 0;
	//备份客户socket fd_set
	fd_set _fdRead_back;
	//客户列表是否变化
	bool _clients_change;
	SOCKET _maxSock;
	bool onRun()
	{
		_clients_change = true;
		while (isRun())
		{
			if (_clientsBuff.size() > 0)
			{
				//从缓冲队列里取出客户数据
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
			fd_set fdRead;
			//清空
			FD_ZERO(&fdRead);
			if (_clients_change)
			{
				_clients_change = false;
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
			int ret = select(_maxSock + 1, &fdRead, 0, 0, nullptr);
			if (ret < 0)
			{
				printf("select任务结束。\n");
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
			std::vector<CellClient*> temp;
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
				delete pClient;
			}
#endif	

		}
	}

	//接收数据 处理粘包 拆分包
	int RecvData(CellClient* pClient)
	{
		char* szRecv = pClient->msgBuf() + pClient->getLastPos();
		// 5 接收客户端数据
		int nLen = (int)recv(pClient->sockfd(), szRecv, (RECV_BUFF_SIZE)-pClient->getLastPos(), 0);
		_pNetEvent->OnNetRecv(pClient);
		//printf("Len=%d\n", nLen);
		if (nLen <= 0)
		{
			//printf("客户端<Socket:%d>已退出，任务结束。\n", pClient->sockfd());
			return -1;
		}
		//将收到的数据拷贝到消息缓冲区
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		//消息缓冲区的数据尾部位置后移
		pClient->setLastPos(pClient->getLastPos() + nLen);
		//判断消息缓冲区的数据长度是否大于消息头DataHeader长度
		while (pClient->getLastPos() >= sizeof(DataHeader))
		{
			//这时就可以知道当前消息的长度
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			//判断消息缓冲区的数据长度是否大于消息长度
			if (pClient->getLastPos() > header->dataLength)
			{
				//剩余未处理消息缓冲区数据的长度
				int nSize = pClient->getLastPos() - header->dataLength;
				//处理网络消息
				OnNetMsg(pClient, header);
				//将消息缓冲区剩余未处理数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, pClient->getLastPos() - header->dataLength);
				//消息缓冲区的数据尾部前移
				pClient->setLastPos(nSize);
			}
			else
			{
				//消息缓冲区剩余数据不够一条完整数据
				break;
			}
		}
		return 0;
	}

	//响应网络消息
	virtual void OnNetMsg(CellClient* pClient, DataHeader* header)
	{
		_pNetEvent->OnNetMsg(this, pClient, header);
	}

	void addClient(CellClient* pClient)
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

	void addSendTask(CellClient* pClient, DataHeader* header)
	{
		_taskServer.addTask([pClient, header]() {
			pClient->SendData(header);
			delete header;
			});
	}
private:  
	SOCKET _sock;
	//正式客户队列
	std::map<SOCKET, CellClient*> _clients;
	//缓冲客户队列
	std::vector<CellClient*> _clientsBuff;
	//缓冲队列的锁
	std::mutex _mutex;
	std::thread _thread;
	//网络事件对象
	INetEvent* _pNetEvent;
	//
	CellTaskServer _taskServer;
};
#endif // !_CELLSERVER_HPP_
