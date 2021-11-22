#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#include"CELL.hpp"
#include"CELLNetWork.hpp"
#include"MessageHeader.hpp"
#include"CELLClient.hpp"
#include"CELLFDSet.hpp"

class EasyTcpClient
{
public:
	EasyTcpClient()
	{
		_isConnect = false;
	}

	virtual ~EasyTcpClient()
	{
		Close();
	}
	//��ʼ��socket
	SOCKET InitSocket(int sendSize = SEND_BUFF_SIZE, int recvSize = RECV_BUFF_SIZE)
	{
		CELLNetWork::Init();

		if (_pClient)
		{
			CELLLog_Info("warning, initSocket close old socket<%d>...", (int)_pClient->sockfd());
			Close();
		}
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == sock)
		{
			CELLLog_Error("create socket failed...");
		}
		else {
			CELLNetWork::make_reuseaddr(sock);
			//CELLLog_Info("create socket<%d> success...", (int)sock);
			_pClient = new CELLClient(sock, sendSize, recvSize);
		}
		return sock;
	}

	//���ӷ�����
	int Connect(const char* ip, unsigned short port)
	{
		if (!_pClient)
		{
			if (INVALID_SOCKET == InitSocket())
			{
				return SOCKET_ERROR;
			}
		}
		// 2 ���ӷ����� connect
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		//CELLLog_Info("<socket=%d> connecting <%s:%d>...", (int)_pClient->sockfd(), ip, port);
		int ret = connect(_pClient->sockfd(), (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			CELLLog_Info("<socket=%d> connect <%s:%d> failed...", (int)_pClient->sockfd(), ip, port);
		}
		else {
			_isConnect = true;
			//CELLLog_Info("<socket=%d> connect <%s:%d> success...", (int)_pClient->sockfd(), ip, port);
		}
		return ret;
	}

	//�ر��׽���closesocket
	void Close()
	{
		if (_pClient)
		{
			delete _pClient;
			_pClient = nullptr;
		}
		_isConnect = false;
	}

	//����������Ϣ
	bool OnRun(int microseconds = 1)
	{
		if (isRun())
		{
			SOCKET _sock = _pClient->sockfd();


			_fdRead.zero();
			_fdRead.add(_sock);

			_fdWrite.zero();

			timeval t = { 0,microseconds };
			int ret = 0;
			if (_pClient->needWrite())
			{

				_fdWrite.add(_sock);
				ret = select(_sock + 1, _fdRead.fdset(), _fdWrite.fdset(), nullptr, &t);
			}
			else {
				ret = select(_sock + 1, _fdRead.fdset(), nullptr, nullptr, &t);
			}

			if (ret < 0)
			{
				CELLLog_Error("<socket=%d>OnRun.select exit", (int)_sock);
				Close();
				return false;
			}

			if (_fdRead.has(_sock))
			{
				if (SOCKET_ERROR == RecvData(_sock))
				{
					CELLLog_Error("<socket=%d>OnRun.select RecvData exit", (int)_sock);
					Close();
					return false;
				}
			}

			if (_fdWrite.has(_sock))
			{
				if (SOCKET_ERROR == _pClient->SendDataReal())
				{
					CELLLog_Error("<socket=%d>OnRun.select SendDataReal exit", (int)_sock);
					Close();
					return false;
				}
			}
			return true;
		}
		return false;
	}

	//�Ƿ�����
	bool isRun()
	{
		return _pClient && _isConnect;
	}

	//�������� ����ճ�� ��ְ�
	int RecvData(SOCKET cSock)
	{
		if (isRun())
		{
			//���տͻ�������
			int nLen = _pClient->RecvData();
			if (nLen > 0)
			{
				//ѭ�� �ж��Ƿ�����Ϣ��Ҫ����
				while (_pClient->hasMsg())
				{
					//����������Ϣ
					OnNetMsg(_pClient->front_msg());
					//�Ƴ���Ϣ���У�����������ǰ��һ������
					_pClient->pop_front_msg();
				}
			}
			return nLen;
		}
		return 0;
	}

	//��Ӧ������Ϣ
	virtual void OnNetMsg(netmsg_DataHeader* header) = 0;

	//��������
	int SendData(netmsg_DataHeader* header)
	{
		if (isRun())
			return _pClient->SendData(header);
		return SOCKET_ERROR;
	}

	int SendData(const char* pData, int len)
	{
		if (isRun())
			return _pClient->SendData(pData, len);
		return SOCKET_ERROR;
	}
protected:
	CELLFDSet _fdRead;
	CELLFDSet _fdWrite;
	CELLClient* _pClient = nullptr;
	bool _isConnect = false;
};

#endif