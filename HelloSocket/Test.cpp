#define WIN32_LEAN_AND_MEAN
#include<windows.h>
#include<WinSock2.h>

//#pragma comment(lib,"ws2_32.lib")			//声明静态链接库
int main()
{
	WORD ver = MAKEWORD(2,2);				//创建WINDOWS版本号
	WSADATA dat;
	WSAStartup(ver, &dat);					//启动网络环境,此函数调用了一个WINDOWS的静态链接库，因此需要加入静态链接库文件


	WSACleanup();							//关闭Socket网络环境
	return 0;
}
