#ifndef _CELLClienthpp_
#define _CELLClienthpp_

#include"CELL.hpp"
#include"CELLBuffer.hpp"

//客户端心跳检测死亡计时时间
#define CLIENT_HEART_DEAD_TIME 60000
//在间隔指定时间后
//把发送缓冲区内缓存的消息数据发送给客户端
#define CLIENT_SEND_BUFF_TIME 200 
//客户端数据类型
class CELLClient
{
	//////////用于调试的成员变量
public:
	int id = -1;
	//所属serverid
	int serverID = -1;
	//测试接收发逻辑用
	//用于server检测接收到的消息ID是否连续
	int nRecvMsgID = 1;
	//测试接收发逻辑用
	//用于client检测接收到的消息ID是否连续
	int nSendMsgID = 1;
	///////////////////////////////////
public:
	CELLClient(SOCKET sockfd = INVALID_SOCKET, int sendSize = SEND_BUFF_SIZE, int recvSize = RECV_BUFF_SIZE) :
		_sendBuff(sendSize),
		_recvBuff(recvSize)
	{
		static int n = 1;
		id = n++;
		_sockfd = sockfd;

		resetDtHeart();
		resetDtSend();
	}

	~CELLClient()
	{
		CELLLog_Info("~CELLClient[sId=%d id=%d socket=%d]", serverID, id, (int)_sockfd);
		if (INVALID_SOCKET != _sockfd)
		{
#ifdef _WIN32
			closesocket(_sockfd);
#else
			close(_sockfd);
#endif
			_sockfd = INVALID_SOCKET;
		}
	}

	SOCKET sockfd()
	{
		return _sockfd;
	}

	//接收数据
	int RecvData()
	{
		return _recvBuff.read4socket(_sockfd);
	}
	//
	bool hasMsg()
	{
		return _recvBuff.hasMsg();
	}
	//
	netmsg_DataHeader* front_msg()
	{
		return (netmsg_DataHeader*)_recvBuff.data();
	}

	bool needWrite()
	{
		return _sendBuff.needWrite();
	}

	void pop_front_msg()
	{
		if (hasMsg())
			_recvBuff.pop(front_msg()->dataLength);
		
	}
	//立即将发送缓冲区的数据发送给客户端
	int SendDataReal()
	{
		resetDtSend();
		return _sendBuff.write2socket(_sockfd);
	}

	//缓冲区大小根据业务需求的差异而变化调整
	//发送数据
	int SendData(netmsg_DataHeader* header)
	{
		return SendData((const char*) header, header->dataLength);
	}
	int SendData(const char* pData, int Len)
	{
		if (_sendBuff.push(pData, Len))
		{
			return Len;
		}

		return SOCKET_ERROR;
	}
	 
	void resetDtHeart()
	{
		_dtHeart = 0;
	}

	void resetDtSend()
	{
		_dtSend = 0;
	}

	//心跳检测
	bool checkHeart(time_t dt)
	{
		_dtHeart += dt;
		if (_dtHeart >= CLIENT_HEART_DEAD_TIME) {
			CELLLog_Info("checkHeart dead:s=%d,time=%d", _sockfd, _dtHeart);
			return true;
		}
		return false;
	}
	 
	//定时发送消息检测
	bool checkSend(time_t dt)
	{
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME) {
			//CELLLog_Info("checkSend:s=%d,time=%d", _sockfd, _dtSend);
			//立即发送缓冲区数据
			SendDataReal();
			//重置发送计时
			resetDtSend();
			return true;
		}
		return false;
	}

private:
	SOCKET _sockfd;
	//第二缓冲区 接收消息缓冲区
	CELLBuffer _recvBuff;
	//第二缓冲区 发送缓冲区
	CELLBuffer _sendBuff;  
	//心跳死亡计时
	time_t _dtHeart;
	//上次发送消息数据的时间
	time_t _dtSend;
	//发送缓冲区写满计数
	int _sendBuffFullCount = 0;
};

#endif // CELLClient


