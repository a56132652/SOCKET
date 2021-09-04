#ifndef _I_NET_EVENT_HPP_
#define _I_NET_EVENT_HPP_

#include "Cell.hpp"
#include "CellClient.hpp"
/********************************************************************************************************************************/
/************************-------------------------INetEvent(网络事件接口)-------------------------*******************************/
/********************************************************************************************************************************/
class INetEvent
{
public:
	//纯虚函数
	//客户端加入事件
	virtual void OnNetJoin(CellClient* pClient) = 0;
	//客户端离开事件
	virtual void OnNetLeave(CellClient* pClient) = 0;
	//客户端消息事件
	virtual void OnNetMsg(CellServer* pCellServer, CellClient* pClient, DataHeader* header) = 0;
	//客户端recv事件
	virtual void OnNetRecv(CellClient* pClient) = 0;
};

#endif // !_I_NET_EVENT_HPP_
