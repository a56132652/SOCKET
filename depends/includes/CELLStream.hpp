#ifndef _CELL_STREAM_HPP_
#define _CELL_STREAM_HPP_
#include<cstdint>
//�ֽ���BYTE
class CELLStream
{
public:
	//����
	CELLStream(char* pData, int nSize, bool bDelete = false)
	{
		_nSize = nSize;
		_pBuff = pData;
		_bDelete = bDelete;
	}

	CELLStream(int nSize = 1024)
	{
		_nSize = nSize;
		_pBuff = new char[_nSize];
		_bDelete = true;
	}

	~CELLStream()
	{
		if (_bDelete && _pBuff)
		{
			delete[] _pBuff;
			_pBuff = _pBuff;
		}
	}

public:
	////��ȡRead
	//int8_t ReadInt8(); //char size-t c# char2 char 1
	//int16_t ReadInt16();	//short
	//int32_t ReadInt32();	//int
	//float ReadFloat();
	//double ReadDouble();
	////д��Write
	//bool WriteInt8();	//char
	//bool WriteInt16();	//short
	//bool WriteInt32();	//int
	//bool WriteFloat();
	//bool WriteDouble();
private:
	//���ݻ�����
	char* _pBuff = nullptr;
	//�������ܵĿռ��С���ֽڳ���
	int _nSize = 0;
	//��д�����ݵ�β��λ�ã��������ݳ���
	int _nWritePos;
	//�Ѷ����ݵ�β��λ�á�
	int _nReadPos;
	//_pBuf���ⲿ��������ݿ�ʱ�Ƿ�Ӧ�ñ��ͷ�
	bool _bDelete = true;
};
#endif // !_CELL_STREAM_HPP_
