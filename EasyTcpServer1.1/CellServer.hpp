#ifndef _CELLServer_HPP_
#define _CELLServer_HPP_

#include"CELL.hpp"
#include"INetEvent.hpp"
#include"CELLClient.hpp"
#include"CELLSemaphore.hpp"

#include <map>
#include <vector>

class CELLServer
{
public:
	CELLServer(int id)
	{
		_id = id;
		_pNetEvent = nullptr;
		_taskServer.serverID = id;
	}
	~CELLServer()
	{
		printf("CELLServer%d.~CELLServer exit begin\n", _id);
		Close();
		printf("CELLServer%d.~CELLServer exit end\n", _id);
	}

	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;
	}  
	//�ر�socket
	void Close()
	{
		printf("CELLServer%d.Close begin\n", _id);
		_taskServer.Close();
		_thread.Close();
		printf("CELLServer%d.Close end\n", _id);
	}

	//����������Ϣ
	bool onRun(CELLThread* pThread)
	{
		while (pThread->isRun())
		{
			if (_clientsBuff.size() > 0)
			{
				//�ӻ��������ȡ���ͻ�����
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
					pClient->serverID = _id;
					if (_pNetEvent)
						_pNetEvent->OnNetJoin(pClient);
				}
				_clientsBuff.clear();
				_clients_change = true;
			}
			//���û����Ҫ����Ŀͻ��ˣ�������
			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				_oldTime = CELLTime::getTimeInMilliSec();
				continue;
			}
			fd_set fdRead;
			fd_set fdWrite;
			//fd_set fdExc;

			if (_clients_change)
			{
				_clients_change = false;
				//���
				FD_ZERO(&fdRead);
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

			memcpy(&fdWrite, &_fdRead_back, sizeof(fd_set));
			//memcpy(&fdExc, &_fdRead_back, sizeof(fd_set));

			timeval t{ 0,1 };
			int ret = select(_maxSock + 1, &fdRead, &fdWrite, nullptr, &t);
			if (ret < 0)
			{
				printf("CELLServer%d OnRun.select Error exit\n",_id);
				pThread->Exit();
				break;
			}
	/*		else if (ret == 0)
			{
				continue;
			}*/

			ReadData(fdRead);
			WriteData(fdWrite);
			//WriteData(fdExc);
			//printf("CELLServer%d.OnRun.select: fdRead=%d,fdWrite=%d\n", _id, fdRead.fd_count, fdWrite.fd_count);
			//if (fdExc.fd_count > 0)
			//{
			//	printf("###fdExc=%d\n", fdExc.fd_count);
			//}
			CheckTime();
		}
		printf("CELLServer%d.OnRun  exit\n", _id);
	}

	
	void CheckTime()
	{
		auto nowTime = CELLTime::getTimeInMilliSec();
		auto dt = nowTime - _oldTime;
		_oldTime = nowTime;
		 
		for (auto iter = _clients.begin(); iter != _clients.end();)
		{
			//�������
			if (iter->second->checkHeart(dt))
			{
				if (_pNetEvent)
					_pNetEvent->OnNetLeave(iter->second);
				_clients_change = true;
				closesocket(iter->first);
				delete iter->second;
				auto iterOld = iter;
				iter++;
				_clients.erase(iterOld);
				continue;
			}
			////��ʱ���ͼ��
			//iter->second->checkSend(dt);
			iter++;
		}
	}
	void OnClientLeave(CELLClient* pClient)
	{
		if (_pNetEvent)
			_pNetEvent->OnNetLeave(pClient);
		_clients_change = true;
		delete pClient;
	}

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
		}
#endif	
	}

	void ReadData(fd_set& fdRead)
	{
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
	}

	//�������� ����ճ�� ��ְ�
	int RecvData(CELLClient* pClient)
	{
		char* szRecv = pClient->msgBuf() + pClient->getLastPos();
		// 5 ���տͻ�������
		int nLen = (int)recv(pClient->sockfd(), szRecv, (RECV_BUFF_SIZE)-pClient->getLastPos(), 0);
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
	virtual void OnNetMsg(CELLClient* pClient, DataHeader* header)
	{
		_pNetEvent->OnNetMsg(this, pClient, header);
	}

	void addClient(CELLClient* pClient)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	void Start()
	{
		_taskServer.Start();
		_thread.Start(nullptr,
			[this](CELLThread* pThread) {onRun(pThread); },
			[this](CELLThread* pThread) {ClearClients(); });
	}

	size_t getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}

	//void addSendTask(CELLClient* pClient, DataHeader* header)
	//{
	//	_taskServer.addTask([pClient, header]() {
	//		pClient->SendData(header);
	//		delete header;
	//		});
	//}
private:
	void ClearClients()
	{
		for (auto iter : _clients)
		{
			delete iter.second;
		}
		_clients.clear();
		for (auto iter : _clientsBuff)
		{
			delete iter;
		}
		_clientsBuff.clear();
	}
private:  
	//��ʽ�ͻ�����
	std::map<SOCKET, CELLClient*> _clients;
	//����ͻ�����
	std::vector<CELLClient*> _clientsBuff;
	//������е���
	std::mutex _mutex;
	//�����¼�����
	INetEvent* _pNetEvent;
	// 
	CELLTaskServer _taskServer;
	//���ݿͻ�socket fd_set
	fd_set _fdRead_back;
	//�ͻ��б��Ƿ�仯
	bool _clients_change = true;
	//
	SOCKET _maxSock;
	//�ɵ�ʱ���
	time_t _oldTime = CELLTime::getTimeInMilliSec();
	//
	int _id = -1;
	//
	CELLThread _thread;
};
#endif // !_CELLServer_HPP_
