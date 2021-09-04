#ifndef _CELL_HPP_
#define _CELL_HPP_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define FD_SETSIZE      2506
#include<windows.h>
#include<WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>

#define SOCKET int
#define INVALID_SOCKET (int)(~0)
#define SOCKET_ERROR (-1)
#endif

#ifndef RECV_BUFF_SIZE
//缓冲区最小单元大小
#define RECV_BUFF_SIZE 10240 * 5
#define SEND_BUFF_SIZE 10240 * 5

#endif // !RECV_BUFF_SIZE

class CellServer;

#include"MessageHeader.hpp"
#include"CELLTimestamp.hpp"
#include"CELLTask.hpp"


#endif // !_CELL_HPP_
