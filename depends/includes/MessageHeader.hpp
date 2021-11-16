#ifndef _MessageHeader_hpp_
#define _MessageHeader_hpp_



//ָ��
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
//��Ϣͷ
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
//��¼
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
//����
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
//��¼���
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

//�ǳ�
struct Loginout : public DataHeader
{
	Loginout()
	{
		dataLength = sizeof(Loginout);
		cmd = CMD_LOGINOUT;
	}
	char userName[32];
};
//�ǳ����
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