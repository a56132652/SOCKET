#ifndef _MessageHeader_hpp_
#define _MessageHeader_hpp_



//指令
enum CMD
{
	CMD_LOGIN = 10,
	CMD_LOGIN_RESULT,
	CMD_LOGINOUT,
	CMD_LOGINOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_C2S_HEART,
	CMD_S2C_HEART,
	CMD_ERROR
};
//消息头
struct DataHeader
{
	DataHeader()
	{
		dataLength = sizeof(DataHeader);
		cmd = CMD_ERROR;
	}
	short dataLength;
	short cmd;
};
//登录
struct Login : public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char PassWord[32];
	char data[32];
	int msgID;
};
//心跳
struct C2S_Heart : public DataHeader
{
	C2S_Heart()
	{
		dataLength = sizeof(C2S_Heart);
		cmd = CMD_C2S_HEART;
	}
};
struct S2C_Heart : public DataHeader
{
	S2C_Heart()
	{
		dataLength = sizeof(S2C_Heart);
		cmd = CMD_S2C_HEART;
	}
};
//登录结果
struct LoginResult : public DataHeader
{
	LoginResult()
	{
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
	char data[92];
	int msgID;

};

//登出
struct Loginout : public DataHeader
{
	Loginout()
	{
		dataLength = sizeof(Loginout);
		cmd = CMD_LOGINOUT;
	}
	char userName[32];
};
//登出结果
struct LoginoutResult : public DataHeader
{
	LoginoutResult()
	{
		dataLength = sizeof(LoginoutResult);
		cmd = CMD_LOGINOUT_RESULT;
		result = 0;
	}
	int result;
};

struct NewUserJoin : public DataHeader
{
	NewUserJoin()
	{
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};



#endif