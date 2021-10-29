#include <iostream>
#include "Cell.hpp"
#include "EasyTcpClient.hpp"

class MyClient : public EasyTcpClient
{
public:
	//ÏìÓ¦ÍøÂçÏûÏ¢
	virtual void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			LoginResult* login = (LoginResult*)header;
			//CELLLog::Info("<socket=%d> recv msgType£ºCMD_LOGIN_RESULT\n", (int)_pClient->sockfd());
		}
		break;
		case CMD_LOGINOUT:
		{
			Loginout* logout = (Loginout*)header;
			//CELLLog::Info("<socket=%d> recv msgType£ºCMD_LOGOUT_RESULT\n", (int)_pClient->sockfd());
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			NewUserJoin* userJoin = (NewUserJoin*)header;
			//CELLLog::Info("<socket=%d> recv msgType£ºCMD_NEW_USER_JOIN\n", (int)_pClient->sockfd());
		}
		break;
		case CMD_ERROR:
		{
			CELLLog::Info("<socket=%d> recv msgType£ºCMD_ERROR\n", (int)_pClient->sockfd());
		}
		break;
		default:
		{
			CELLLog::Info("error, <socket=%d> recv undefine msgType\n", (int)_pClient->sockfd());
		}
		}
	}
private:

};


int main()
{
	MyClient client;
	client.Connect("127.0.0.1", 4567);
	//client.SendData();
	while (client.isRun()) 
	{
		client.OnRun();
		
		CELLThread::Sleep(10);
	}
	return 0;
}
