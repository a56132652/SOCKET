#define WIN32_LEAN_AND_MEAN
#include<windows.h>
#include<WinSock2.h>

//#pragma comment(lib,"ws2_32.lib")			//������̬���ӿ�
int main()
{
	WORD ver = MAKEWORD(2,2);				//����WINDOWS�汾��
	WSADATA dat;
	WSAStartup(ver, &dat);					//�������绷��,�˺���������һ��WINDOWS�ľ�̬���ӿ⣬�����Ҫ���뾲̬���ӿ��ļ�


	WSACleanup();							//�ر�Socket���绷��
	return 0;
}
