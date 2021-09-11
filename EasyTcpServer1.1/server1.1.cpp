#include "Cell.hpp"
#include "CellClient.hpp"
#include <string>
#include "EasyTcpServer.hpp"

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
	virtual void OnNetJoin(CellClient* pClient)
	{
		EasyTcpServer::OnNetJoin(pClient); 
	}
	//cellSetver 4 ����̴߳��� ����ȫ
	//���ֻ����һ��cellServer���ǰ�ȫ��
	virtual void OnNetLeave(CellClient* pClient)
	{
		EasyTcpServer::OnNetLeave(pClient);
	}
	//cellSetver 4 ����̴߳��� ����ȫ
	//���ֻ����һ��cellServer���ǰ�ȫ��
	virtual void OnNetMsg(CellServer* pCellServer, CellClient* pClient, DataHeader* header)
	{
		EasyTcpServer::OnNetMsg(pCellServer, pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			pClient->resetDtHeart();
			Login* login = (Login*)header;
			//printf("�յ��ͻ���<socket:%d>���CMD_LOGIN�����ݳ��� : %d,userName = %s ,PassWord = %s \n", cSock, login->dataLength, login->userName, login->PassWord);
			LoginResult ret;
			pClient->SendData(&ret);
			//LoginResult* ret = new LoginResult();
			//pCellServer->addSendTask(pClient,ret);
		}
		break;
		case CMD_LOGINOUT:
		{
			Loginout* loginout = (Loginout*)header;
			//printf("�յ��ͻ���<socket:%d>���CMD_LOGINOUT�����ݳ��� : %d,userName = %s \n", cSock, loginout->dataLength, loginout->userName);
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
	printf("���˳�\n");
	while (true)
	{
		Sleep(1);
	}
	/*
	CellTaskServer task;
	task.Start();
	Sleep(100);
	task.Close();
	while (true)
	{
		Sleep(1);
	}
	*/
	return 0;
}

