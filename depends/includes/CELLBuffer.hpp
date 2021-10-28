#ifndef _CELL_BUFFER_HPP_
#define	_CELL_BUFFER_HPP_

#include "Cell.hpp"

class CELLBuffer
{
public:
	CELLBuffer(int nSize = 8192)
	{
		_nSize = nSize;
		_pBuff = new char[_nSize];
	}
	~CELLBuffer()
	{
		if (_pBuff)
		{
			delete[] _pBuff;
			_pBuff = _pBuff;
		}
	}

	char* data()
	{
		return _pBuff;
	}

	bool push(const char* pData, int nLen)
	{
		if (_nLast + nLen <= _nSize)
		{
			//��Ҫ���͵����ݿ��������ͻ�����β��
			memcpy(_pBuff + _nLast, pData, nLen);
			//����β��λ���ƶ�
			_nLast += nLen;

			if (_nLast == SEND_BUFF_SIZE)
			{
				++_fullCount;
			}
			return true;
		}
		else {
			++_fullCount;
		}
		return false;
	}

	//
	void pop(int nLen)
	{
		int n = _nLast - nLen;
		if (n > 0)
		{
			memcpy(_pBuff, _pBuff + nLen, n);
		}
		_nLast = n;
		if (_fullCount > 0)
			--_fullCount;
	}
	//���������ͻ����������ݷ��͸��ͻ���
	int write2socket(SOCKET sockfd)
	{
		int ret = 0;
		//������������
		if (_nLast > 0 && INVALID_SOCKET != sockfd)
		{
			//��������
			ret = send(sockfd, _pBuff, _nLast, 0);
			//����β��λ�� ��0
			_nLast = 0;
			//
			_fullCount = 0;

		}
		return ret;
	}
	//
	int read4socket(SOCKET sockfd)
	{
		if (_nSize - _nLast > 0)
		{
			char* szRecv = _pBuff + _nLast;
			// ���տͻ�������
			int nLen = (int)recv(sockfd, szRecv, _nSize - _nLast, 0);
			//CELLLog::Info("Len=%d\n", nLen);
			if (nLen <= 0)
			{
				return nLen;
			}
			//��Ϣ������������β��λ�ú���
			_nLast += nLen;
			return nLen;
		}
		return 0;
	}
	//
	bool hasMsg()
	{
		//�ж���Ϣ�����������ݳ��ȴ�����Ϣͷnetmsg_DataHeader����
		if (_nLast >= sizeof(DataHeader))
		{
			//��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
			DataHeader* header = (DataHeader*)_pBuff;
			//�ж���Ϣ�����������ݳ����Ƿ������Ϣ����
			return _nLast >= header->dataLength;
		}
		return false;
	}
private:
	//�ڶ������� ���ͻ�����
	char* _pBuff = nullptr;
	//�������������������������ݿ�
	//list<char*> _pBuffList;
	//������������β��λ�ã��������ݳ���
	int _nLast = 0;
	//�������ܵĿռ��С���ֽڳ���
	int _nSize = 0;
	//������д����������
	int _fullCount = 0;
};
#endif // !_CELL_BUFFER_HPP_
