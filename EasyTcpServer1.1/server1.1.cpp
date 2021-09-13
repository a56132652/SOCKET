#include "Cell.hpp"
#include "CELLClient.hpp"
#include <string>
#include "EasyTcpServer.hpp"

//bool g_bRun = true;
////处理请求函数
//void  cmdThread()
//{
//}

class MyServer : public EasyTcpServer
{
public:
	//只会被一个线程触发 安全
	virtual void OnNetJoin(CELLClient* pClient)
	{
		EasyTcpServer::OnNetJoin(pClient); 
	}
	//cellSetver 4 多个线程触发 不安全
	//如果只开启一个CELLServer就是安全的
	virtual void OnNetLeave(CELLClient* pClient)
	{
		EasyTcpServer::OnNetLeave(pClient);
	}
	//cellSetver 4 多个线程触发 不安全
	//如果只开启一个CELLServer就是安全的
	virtual void OnNetMsg(CELLServer* pCELLServer, CELLClient* pClient, DataHeader* header)
	{
		EasyTcpServer::OnNetMsg(pCELLServer, pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			pClient->resetDtHeart();
			Login* login = (Login*)header;
			//printf("收到客户端<socket:%d>命令：CMD_LOGIN，数据长度 : %d,userName = %s ,PassWord = %s \n", cSock, login->dataLength, login->userName, login->PassWord);
			LoginResult ret;
			if (SOCKET_ERROR == pClient->SendData(&ret))
			{
				//发送缓冲区满了，消息没发出去
				printf("<Socket=%d> Send Full\n", pClient->sockfd());
			}
			//LoginResult* ret = new LoginResult();
			//pCELLServer->addSendTask(pClient,ret);
		}
		break;
		case CMD_LOGINOUT:
		{
			Loginout* loginout = (Loginout*)header;
			//printf("收到客户端<socket:%d>命令：CMD_LOGINOUT，数据长度 : %d,userName = %s \n", cSock, loginout->dataLength, loginout->userName);
			//LoginoutResult ret = {  };
			//pClient->SendData(&ret);
		}
		break;
		case CMD_C2S_HEART:
		{
			pClient->resetDtHeart();
			S2C_Heart ret;
			pClient->SendData(&ret);
		}
		break;
		default:
		{
			printf("<socket:%d>收到未定义消息，数据长度：%d\n", pClient->sockfd(), header->dataLength);
			//DataHeader ret;
			//pClient->SendData(&ret);
		}
		break;
		}
	}
};

int main()
{
	MyServer server;
	server.InitSocket(); 
	server.Bind(NULL, 4567);
	server.Listen(64);
	server.Start(4);

/*	std::thread t1(cmdThread); 
	t1.detach();*/	 \

	while (true)
	{
		char cmdBuf[256] = { };
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			server.Close();
			printf("退出cmdThread线程\n");
			break;
		}
		else {
			printf("不支持的命令，请重新输入。\n");
		}
	}

	printf("已退出\n");
#ifdef _WIN32
	while (true)
		Sleep(10);
#endif
	return 0;

}

