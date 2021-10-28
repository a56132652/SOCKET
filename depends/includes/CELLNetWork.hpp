#ifndef _CELL_NETWORK_HPP_
#define	_CELL_NETWORK_HPP_

#include"CELL.hpp"
class CELLNetWork
{
private:
	CELLNetWork()
	{
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);				//创建WINDOWS版本号
		WSADATA dat;
		WSAStartup(ver, &dat);					//启动网络环境,此函数调用了一个WINDOWS的静态链接库，因此需要加入静态链接库文件
#endif

#ifndef _WIN32
		//if (sognal(SIGPIPE, SIG_IGN) == SIG_ERR)
		//	return (1);
		//忽略异常信号，默认情况会导致进程终止
		signal(SIGPIPE, SIG_IGN);
#endif // !_WIN32
	}
	~CELLNetWork() 
	{
#ifdef _WIN32
		//关闭套接字socket
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
