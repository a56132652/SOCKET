#ifndef _CELL_STREAM_HPP_
#define _CELL_STREAM_HPP_
#include<cstdint>
//字节流BYTE
class CELLStream
{
public:
	//用于
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
	////读取Read
	//int8_t ReadInt8(); //char size-t c# char2 char 1
	//int16_t ReadInt16();	//short
	//int32_t ReadInt32();	//int
	//float ReadFloat();
	//double ReadDouble();
	////写入Write
	//bool WriteInt8();	//char
	//bool WriteInt16();	//short
	//bool WriteInt32();	//int
	//bool WriteFloat();
	//bool WriteDouble();
private:
	//数据缓冲区
	char* _pBuff = nullptr;
	//缓冲区总的空间大小，字节长度
	int _nSize = 0;
	//已写入数据的尾部位置，已有数据长度
	int _nWritePos;
	//已读数据的尾部位置、
	int _nReadPos;
	//_pBuf是外部传入的数据块时是否应该被释放
	bool _bDelete = true;
};
#endif // !_CELL_STREAM_HPP_
