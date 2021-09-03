#include "EasyTcpServer.hpp"
#include "thread"
#include "Alloctor.h"
//#include "Alloctor.h"

bool g_bRun = true;
//����������
void  cmdThread()
{
	while (true)
	{
		char cmdBuf[256] = { };
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("�˳�cmdThread�߳�\n");
			break;
		}
		else {
			printf("��֧�ֵ�������������롣\n");
		}
	}
}

class MyServer : public EasyTcpServer
{
public:
	//ֻ�ᱻһ���̴߳��� ��ȫ
	virtual void OnNetJoin(ClientSocketPtr& pClient)
	{
		EasyTcpServer::OnNetJoin(pClient); 
	}
	//cellSetver 4 ����̴߳��� ����ȫ
	//���ֻ����һ��cellServer���ǰ�ȫ��
	virtual void OnNetLeave(ClientSocketPtr& pClient)
	{
		EasyTcpServer::OnNetLeave(pClient);
	}
	//cellSetver 4 ����̴߳��� ����ȫ
	//���ֻ����һ��cellServer���ǰ�ȫ��
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocketPtr& pClient, DataHeader* header)
	{
		EasyTcpServer::OnNetMsg(pCellServer, pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//printf("�յ��ͻ���<socket:%d>���CMD_LOGIN�����ݳ��� : %d,userName = %s ,PassWord = %s \n", pClient->sockfd(), login->dataLength, login->userName, login->PassWord);
			//LoginResult* ret = new LoginResult();
			//pClient->SendData(ret);
			auto ret = std::make_shared<LoginResult>();
			pCellServer->addSendTask(pClient,(DataHeaderPtr&)ret);
		}
		break;
		case CMD_LOGINOUT:
		{
			//Loginout* loginout = (Loginout*)header;
			//printf("�յ��ͻ���<socket:%d>���CMD_LOGINOUT�����ݳ��� : %d,userName = %s \n", cSock, loginout->dataLength, loginout->userName);
			//LoginoutResult ret = {  };
			//pClient->SendData(&ret);
		}
		break;
		default:
		{
			printf("<socket:%d>�յ�δ������Ϣ�����ݳ��ȣ�%d\n", pClient->sockfd(), header->dataLength);
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

