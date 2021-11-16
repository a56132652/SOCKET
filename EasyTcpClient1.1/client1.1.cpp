#include"EasyTcpClient.hpp"
#include"MessageHeader.hpp"
#include"CELLTimestamp.hpp"
#include"CELLConfig.hpp"
#include<stdio.h>
#include<thread>
#include<atomic>
#include<vector>

using namespace std;

//�����IP��ַ
const char* strIP = "127.0.0.1";
//����˶˿�
uint16_t nPort = 4567;
//�����߳�����
int nThread = 1;
//�ͻ�������
int nClient = 1;
/*
::::::���ݻ���д�뷢�ͻ�����
::::::�ȴ�socket��дʱ��ʵ�ʷ���
::ÿ���ͻ�����nSendSleep(����)ʱ����
::����д��nMsg��Login��Ϣ
::ÿ����Ϣ100�ֽڣ�Login��
*/
//�ͻ���ÿ�η�������Ϣ
int nMsg = 1;
//д����Ϣ���������ļ��ʱ��
int nSendSleep = 1;
//��������ʱ��
int nWorkSleep = 1;
//�ͻ��˷��ͻ�������С
int nSendBuffSize = SEND_BUFF_SIZE;
//�ͻ��˽��ջ�������С
int nRecvBuffSize = RECV_BUFF_SIZE;

class MyClient : public EasyTcpClient
{
public:
	MyClient()
	{
		_bCheckMsgID = CELLConfig::Instance().hasKey("-checkMsgID");
	}
	//��Ӧ������Ϣ
	virtual void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			LoginResult* login = (LoginResult*)header;
			if (_bCheckMsgID)
			{
				if (login->msgID != _nRecvMsgID)
				{//��ǰ��ϢID�ͱ�������Ϣ������ƥ��
					CELLLog_Error("OnNetMsg socket<%d> msgID<%d> _nRecvMsgID<%d> %d", _pClient->sockfd(), login->msgID, _nRecvMsgID, login->msgID - _nRecvMsgID);
				}
				++_nRecvMsgID;
			}
			//CELLLog_Info("<socket=%d> recv msgType��CMD_LOGIN_RESULT", (int)_pClient->sockfd());
		}
		break;
		case CMD_LOGINOUT_RESULT:
		{
			LoginoutResult* logout = (LoginoutResult*)header;
			//CELLLog_Info("<socket=%d> recv msgType��CMD_LOGOUT_RESULT", (int)_pClient->sockfd());
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			NewUserJoin* userJoin = (NewUserJoin*)header;
			//CELLLog_Info("<socket=%d> recv msgType��CMD_NEW_USER_JOIN", (int)_pClient->sockfd());
		}
		break;
		case CMD_ERROR:
		{
			CELLLog_Info("<socket=%d> recv msgType��CMD_ERROR", (int)_pClient->sockfd());
		}
		break;
		default:
		{
			CELLLog_Info("error, <socket=%d> recv undefine msgType", (int)_pClient->sockfd());
		}
		}
	}

	int SendTest(Login* login)
	{
		int ret = 0;
		//���ʣ�෢�ʹ�������0
		if (_nSendCount > 0)
		{
			login->msgID = _nSendMsgID;
			ret = SendData(login);
			if (SOCKET_ERROR != ret)
			{
				++_nSendMsgID;
				//���ʣ�෢�ʹ�������һ��
				--_nSendCount;
			}
		}
		return ret;
	}

	bool checkSend(time_t dt)
	{
		_tRestTime += dt;
		//ÿ����nSendSleep����
		if (_tRestTime >= nSendSleep)
		{
			//���ü�ʱ
			_tRestTime -= nSendSleep;
			//���÷��ͼ���
			_nSendCount = nMsg;
		}
		return _nSendCount > 0;
	}
private:
	//������Ϣid����
	int _nRecvMsgID = 1;
	//������Ϣid����
	int _nSendMsgID = 1;
	//����ʱ�����
	time_t _tRestTime = 0;
	//������������
	int _nSendCount = 0;
	//�����յ��ķ������ϢID�Ƿ�����
	bool _bCheckMsgID = false;
};

std::atomic_int sendCount(0);
std::atomic_int readyCount(0);
std::atomic_int nConnect(0);

void WorkThread(CELLThread* pThread, int id)
{
	//n���߳� idֵΪ 1~n
	CELLLog_Info("thread<%d>,start", id);
	//�ͻ�������
	vector<MyClient*> clients(nClient);
	//���㱾�߳̿ͻ�����clients�ж�Ӧ��index
	int begin = 0;
	int end = nClient;
	for (int n = begin; n < end; n++)
	{
		//�ж��߳��Ƿ��������У���������Exit����ʱ��10000���ͻ��˻�û�����꣬��ʱֱ���˳������ټ�������
		if (!pThread->isRun())
			break;
		clients[n] = new MyClient();
		//���߳�ʱ����CPU
		CELLThread::Sleep(0);
	}
	for (int n = begin; n < end; n++)
	{
		//�ж��߳��Ƿ��������У����߳�δ���У�ֱ���˳�
		if (!pThread->isRun())
			break;
		if (INVALID_SOCKET == clients[n]->InitSocket(nSendBuffSize, nRecvBuffSize))
			break;
		if (SOCKET_ERROR == clients[n]->Connect(strIP, nPort))
			break;
		nConnect++;
		CELLThread::Sleep(0);
	}
	//�����������
	CELLLog_Info("thread<%d>,Connect<begin=%d, end=%d ,nConnect=%d>", id, begin, end, (int)nConnect);

	readyCount++;
	while (readyCount < nThread && pThread->isRun())
	{//�ȴ������߳�׼���ã��ٷ�������
		CELLThread::Sleep(10);
	}

	//��Ϣ
	Login login;
	//�����������ֵ
	strcpy(login.userName, "lyd");
	strcpy(login.PassWord, "lydmm");
	//
	//�շ����ݶ���ͨ��onRun�߳�
	//SendDataֻ�ǽ�����д�뷢�ͻ�����
	//�ȴ�select����дʱ�Żᷢ������
	//�ɵ�ʱ���
	auto t2 = CELLTime::getNowInMilliSec();
	//�µ�ʱ���
	auto t0 = t2;
	//������ʱ��
	auto dt = t0;
	CELLTimestamp tTime;
	while (pThread->isRun())
	{
		t0 = CELLTime::getNowInMilliSec();
		dt = t0 - t2;
		t2 = t0;
		//����while (pThread->isRun())ѭ����Ҫ��������
		//����work
		{
			int count = 0;
			//ÿ��ÿ���ͻ��˷���nMsg������
			for (int m = 0; m < nMsg; m++)
			{
				//ÿ���ͻ���1��1����д����Ϣ
				for (int n = begin; n < end; n++)
				{
					if (clients[n]->isRun())
					{
						if (clients[n]->SendTest(&login) > 0)
						{
							++sendCount;
						}
					}
				}
			}
			//sendCount += count;
			for (int n = begin; n < end; n++)
			{
				if (clients[n]->isRun())
				{	//��ʱ����Ϊ0��ʾselect���״̬����������
					if (!clients[n]->OnRun(0))
					{	//���ӶϿ�
						nConnect--;
						continue;
					}
					//��ⷢ�ͼ����Ƿ���Ҫ����
					clients[n]->checkSend(dt);
				}
			}
		}
		CELLThread::Sleep(nWorkSleep);
	}
	//--------------------------
	//�ر���Ϣ�շ��߳�
	//tRun.Close();
	//�رտͻ���
	for (int n = begin; n < end; n++)
	{
		clients[n]->Close();
		delete clients[n];
	}
	CELLLog_Info("thread<%d>,exit", id);
	--readyCount;
}

int main(int argc, char* args[])
{
	//����������־����
	CELLLog::Instance().setLogPath("clientLog", "w", false);

	CELLConfig::Instance().Init(argc, args);

	strIP = CELLConfig::Instance().getStr("strIP", "127.0.0.1");
	nPort = CELLConfig::Instance().getInt("nPort", 4567);
	nThread = CELLConfig::Instance().getInt("nThread", 1);
	nClient = CELLConfig::Instance().getInt("nClient", 10000);
	nMsg = CELLConfig::Instance().getInt("nMsg", 10);
	nSendSleep = CELLConfig::Instance().getInt("nSendSleep", 100);
	nSendBuffSize = CELLConfig::Instance().getInt("nSendBuffSize", SEND_BUFF_SIZE);
	nRecvBuffSize = CELLConfig::Instance().getInt("nRecvBuffSize", RECV_BUFF_SIZE);

	//�����ն������߳�
	//���ڽ�������ʱ�û������ָ��
	CELLThread tCmd;
	tCmd.Start(nullptr, [](CELLThread* pThread) {
		while (true)
		{
			char cmdBuf[256] = {};
			scanf("%s", cmdBuf);
			if (0 == strcmp(cmdBuf, "exit"))
			{
				//pThread->Exit();
				CELLLog_Info("�˳�cmdThread�߳�");
				break;
			}
			else {
				CELLLog_Info("��֧�ֵ����");
			}
		}
		});

	//����ģ��ͻ����߳�
	vector<CELLThread*> threads;
	for (int n = 0; n < nThread; n++)
	{
		CELLThread* t = new CELLThread();
		t->Start(nullptr, [n](CELLThread* pThread) {
			WorkThread(pThread, n + 1);
			});
		threads.push_back(t);
	}
	//ÿ������ͳ��
	CELLTimestamp tTime;
	while (tCmd.isRun())
	{
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			CELLLog_Info("thread<%d>,clients<%d>,connect<%d>,time<%lf>,send<%d>", nThread, nClient, (int)nConnect, t, (int)sendCount);
			sendCount = 0;
			tTime.update();
		}
		CELLThread::Sleep(1);
	}
	//
	for (auto t : threads)
	{
		t->Close();
		delete t;
	}
	CELLLog_Info("�������˳�����");
	return 0;
}
