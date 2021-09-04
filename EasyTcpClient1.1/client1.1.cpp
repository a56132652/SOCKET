#include"EasyTcpClient.hpp"
#include"MessageHeader.hpp"
#include"CELLTimestamp.hpp"
#include<stdio.h>
#include<thread>
#include<atomic>


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


//客户端数量
const int cCount = 1000;
//发送线程数量
const int tCount = 4;
//客户端数组
EasyTcpClient* client[cCount];
std::atomic_int sendCount = 0;
std::atomic_int readyCount = 0;

//接收线程
void recvThread(int begin, int end)
{
	while (g_bRun)
	{
		for (int i = begin; i < end; i++)
		{
			client[i]->OnRun();
		}
	}
}
//发送线程
void sendThread(int id)
{
	printf("thread<%d> ,start\n", id);
	//4个线程，ID 1 - 4
	int c = cCount / tCount;
	int begin = (id - 1) * c;
	int end = id * c;

	for (int i = begin; i < end; i++)
	{
		client[i] = new EasyTcpClient();
	}  
	for (int i = begin; i < end; i++)
	{

		client[i]->Connect("127.0.0.1", 4567);	//Linux:192.168.43.129	yql:192.168.1.202  benji:192.168.43.1
	}
	 
	printf("thread<%d> Connect<begin=%d, end=%d>\n", id, begin,end);

	readyCount++;
	while (readyCount < tCount) {
		//等待其他线程准备好发送数据
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}
	
	//启动接收线程
	std::thread t1(recvThread, begin, end);
	t1.detach();

	Login login[10];
	//strcpy(login[1].userName, "yql");
	//strcpy(login[1].PassWord, "yqlyyds");
	for (int i = 0; i < 10; i++)
	{
		strcpy(login[i].userName, "yql");
		strcpy(login[i].PassWord, "yqlyyds");
	}
	const int nLen = sizeof(login);


	while (g_bRun)
	{
		for (int i = begin; i < end; i++)
		{
			if (SOCKET_ERROR != client[i]->SendData(login, nLen)) {
					sendCount++;
			}
		}
		//std::chrono::milliseconds t(10);
		//std::this_thread::sleep_for(t);
	}

	for (int i = 0; i < cCount; i++)
	{
		client[i]->Close();	//Linux:192.168.43.129
		delete client[i];
	}

	printf("thread<%d> ,exit\n", id);
}

int main()
{
	//启动UI线程
	std::thread t1(cmdThread);
	t1.detach();

	//启动发送线程
	for (int i = 0; i < tCount; i++)
	{
		std::thread t1(sendThread,i+1);
		t1.detach();
	}
	CELLTimestamp tTime;

	while (g_bRun)
	{
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0) {
			printf("thread<%d>,clients<%d>,time<%lf>,send<%d>\n",tCount, cCount, t, (int)(sendCount.load() / t));
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}

	printf("已退出。\n");
	getchar();
	return 0;

}
