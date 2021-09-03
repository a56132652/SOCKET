#include "EasyTcpServer.hpp"
#include "thread"
#include "Alloctor.h"
//#include "Alloctor.h"

bool g_bRun = true;
//处理请求函数
void  cmdThread()
{
	while (true)
	{
		char cmdBuf[256] = { };
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("退出cmdThread线程\n");
			break;
		}
		else {
			printf("不支持的命令，请重新输入。\n");
		}
	}
}

class MyServer : public EasyTcpServer
{
public:
	//只会被一个线程触发 安全
	virtual void OnNetJoin(ClientSocketPtr& pClient)
	{
		EasyTcpServer::OnNetJoin(pClient); 
	}
	//cellSetver 4 多个线程触发 不安全
	//如果只开启一个cellServer就是安全的
	virtual void OnNetLeave(ClientSocketPtr& pClient)
	{
		EasyTcpServer::OnNetLeave(pClient);
	}
	//cellSetver 4 多个线程触发 不安全
	//如果只开启一个cellServer就是安全的
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocketPtr& pClient, DataHeader* header)
	{
		EasyTcpServer::OnNetMsg(pCellServer, pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//printf("收到客户端<socket:%d>命令：CMD_LOGIN，数据长度 : %d,userName = %s ,PassWord = %s \n", pClient->sockfd(), login->dataLength, login->userName, login->PassWord);
			//LoginResult* ret = new LoginResult();
			//pClient->SendData(ret);
			auto ret = std::make_shared<LoginResult>();
			pCellServer->addSendTask(pClient,(DataHeaderPtr&)ret);
		}
		break;
		case CMD_LOGINOUT:
		{
			//Loginout* loginout = (Loginout*)header;
			//printf("收到客户端<socket:%d>命令：CMD_LOGINOUT，数据长度 : %d,userName = %s \n", cSock, loginout->dataLength, loginout->userName);
			//LoginoutResult ret = {  };
			//pClient->SendData(&ret);
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
	server.Listen(5);
	server.Start(4);

	std::thread t1(cmdThread);
	t1.detach();	 

	while (g_bRun)
	{	
		server.onRun();

	}
	server.Close();


	getchar();
	return 0;
}

