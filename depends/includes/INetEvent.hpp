#ifndef _I_NET_EVENT_HPP_
#define _I_NET_EVENT_HPP_

#include "Cell.hpp"
#include "CELLClient.hpp"
//�Զ���
class CELLServer;

//�����¼��ӿ�
class INetEvent
{
public:
	//���麯��
	//�ͻ��˼����¼�
	virtual void OnNetJoin(CELLClient* pClient) = 0;
	//�ͻ����뿪�¼�
	virtual void OnNetLeave(CELLClient* pClient) = 0;
	//�ͻ�����Ϣ�¼�
	virtual void OnNetMsg(CELLServer* pCELLServer, CELLClient* pClient, netmsg_DataHeader* header) = 0;
	//�ͻ���recv�¼�
	virtual void OnNetRecv(CELLClient* pClient) = 0;
};

#endif // !_I_NET_EVENT_HPP_
