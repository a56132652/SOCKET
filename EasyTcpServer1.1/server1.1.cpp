#include "Cell.hpp"
#include "CELLClient.hpp"
#include <string>
#include "EasyTcpServer.hpp"

//bool g_bRun = true;
////����������
//void  cmdThread()
//{
//}

class MyServer : public EasyTcpServer
{
public:
	//ֻ�ᱻһ���̴߳��� ��ȫ
	virtual void OnNetJoin(CELLClient* pClient)
	{
		EasyTcpServer::OnNetJoin(pClient); 
	}
	//cellSetver 4 ����̴߳��� ����ȫ
	//���ֻ����һ��CELLServer���ǰ�ȫ��
	virtual void OnNetLeave(CELLClient* pClient)
	{
		EasyTcpServer::OnNetLeave(pClient);
	}
	//cellSetver 4 ����̴߳��� ����ȫ
	//���ֻ����һ��CELLServer���ǰ�ȫ��
	virtual void OnNetMsg(CELLServer* pCELLServer, CELLClient* pClient, DataHeader* header)
	{
		EasyTcpServer::OnNetMsg(pCELLServer, pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			pClient->resetDtHeart();
			Login* login = (Login*)header;
			//CELLLog_Info("�յ��ͻ���<socket:%d>���CMD_LOGIN�����ݳ��� : %d,userName = %s ,PassWord = %s \n", cSock, login->dataLength, login->userName, login->PassWord);
			LoginResult ret;
			if (SOCKET_ERROR == pClient->SendData(&ret))
			{
				//���ͻ��������ˣ���Ϣû����ȥ
				CELLLog_Info("<Socket=%d> Send Full\n", pClient->sockfd());
			}
			//LoginResult* ret = new LoginResult();
			//pCELLServer->addSendTask(pClient,ret);
		}
		break;
		case CMD_LOGINOUT:
		{
			Loginout* loginout = (Loginout*)header;
			//CELLLog_Info("�յ��ͻ���<socket:%d>���CMD_LOGINOUT�����ݳ��� : %d,userName = %s \n", cSock, loginout->dataLength, loginout->userName);
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
			CELLLog_Info("<socket:%d>�յ�δ������Ϣ�����ݳ��ȣ�%d\n", pClient->sockfd(), header->dataLength);
			//DataHeader ret;
			//pClient->SendData(&ret);
		}
		break;
		}
	}
};

int main()
{
	CELLLog::Instance().setLogPath("serverLog","w");
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
			CELLLog_Info("�˳�cmdThread�߳�\n");
			break;
		}
		else {
			CELLLog_Info("��֧�ֵ�������������롣\n");
		}
	}

	CELLLog_Info("���˳�\n");
//#ifdef _WIN32
//	while (true)
//		Sleep(10);
//#endif
	return 0;

}

