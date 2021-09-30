#ifndef _CELL_NETWORK_HPP_
#define	_CELL_NETWORK_HPP_

#include"CELL.hpp"
class CELLNetWork
{
private:
	CELLNetWork()
	{
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);				//����WINDOWS�汾��
		WSADATA dat;
		WSAStartup(ver, &dat);					//�������绷��,�˺���������һ��WINDOWS�ľ�̬���ӿ⣬�����Ҫ���뾲̬���ӿ��ļ�
#endif

#ifndef _WIN32
		//if (sognal(SIGPIPE, SIG_IGN) == SIG_ERR)
		//	return (1);
		//�����쳣�źţ�Ĭ������ᵼ�½�����ֹ
		signal(SIGPIPE, SIG_IGN);
#endif // !_WIN32
	}
	~CELLNetWork() 
	{
#ifdef _WIN32
		//�ر��׽���socket
		WSACleanup();
#else

#endif
	}
public:
	static CELLNetWork& Init()
	{
		static CELLNetWork obj;
		return obj;
	} 
};



#endif // ! _CELL_NETWORK_HPP
