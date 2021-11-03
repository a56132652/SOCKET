#ifndef _CELL_STREAM_HPP_
#define _CELL_STREAM_HPP_
#include<cstdint>
#include"CELLLog.hpp"
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
	char* data()
	{
		return _pBuff;
	}

	int length()
	{
		return _nWritePos;
	}

	inline bool canRead(int n)
	{
		return _nSize - _nReadPos >= n;
	}

	inline bool canWrite(int n)
	{
		return _nSize - _nWritePos >= n;
	}

	inline void push(int n)
	{
		_nWritePos += n;
	}

	inline void pop(int n)
	{
		_nReadPos += n;
	}

	inline void setWritePos(int n)
	{
		_nWritePos = n;
	}
	//读取Read
	template<typename T>
	bool Read(T& n, bool bOffset = true)
	{
		auto nLen = sizeof(T);
		if (_nReadPos + nLen <= _nSize)
		{
			memcpy(&n, _pBuff + _nReadPos, nLen);
			if(bOffset)
				pop(nLen);
			return true;
		}
		//错误日志
		CELLLog::Info("CELLStream::Read failed.");
		return false;
	}

	template<typename T>
	bool onlyRead(T& n) {
		return Read(n, false);
	}

	template<typename T>
	uint32_t ReadArray(T* pArr, uint32_t len)
	{
		//读取数组元素个数
		uint32_t len1 = 0;
		Read(len1,false);
		if (len1 < len)
		{
			//计算数组字节长度
			auto nLen = len1 * sizeof(T);
			//判断能不能读出
			if (canRead(nLen))
			{
				//计算已读位置+数组长度占有空间
				pop(sizeof(uint32_t));
				memcpy(pArr, _pBuff + _nReadPos, nLen);
				pop(nLen);
				return len1;
			}
		}
		CELLLog::Info("error, CELLStream::ReadArray failed.");
		return 0;
	}
	int8_t ReadInt8(int8_t def = 0) 
	{
		Read(def);
		return def;
	}

	int16_t ReadInt16(int16_t n = 0)	//short
	{
		Read(n);
		return n;
	}

	int32_t ReadInt32(int32_t n = 0)	//int
	{
		Read(n);
		return n;
	}

	float ReadFloat(float n = 0.0f)
	{ 
		Read(n);
		return n;
	}
	double ReadDouble(double n = 0.0f)
	{
		Read(n);
		return n;
	}

	

	 
	////写入Write
	template<typename T>
	bool Write(T n)	//int
	{
		//计算要写入数据的大小
		size_t nLen = sizeof(T);
		//判断能不能写入
		if (canWrite(nLen))
		{
			//将要写入的数据拷贝到缓冲区尾部
			memcpy(_pBuff + _nWritePos, &n, nLen);
			//数据尾部位置移动
			push(nLen);
			return true;
		}
		CELLLog::Info("error, CELLStream::Write failed.");
		return false;
	}

	template<typename T>
	bool WriteArray(T* pData, uint32_t len)	//之所以选择uint32_t，而不是size_t,因为size_t在32位和64位系统下大小不同；若数组长度大于uint32_t所表示的最大范围，则应该进行分包
	{
		//计算要写入数组的字节长度 
		auto nLen = sizeof(T) * len;
		if (canWrite(nLen + sizeof(uint32_t)))
		{
			//先写入数组长度，方便读取数据
			WriteInt32(len);
			//再写入数组实际数据
			memcpy(_pBuff + _nWritePos, pData, nLen);
			//数据尾部位置移动
			push(nLen);
			return true;
		}
		CELLLog::Info("error, CELLStream::WriteArray failed.");
		return false;
	}
	//char
	bool WriteInt8(int8_t n)
	{
		return Write(n);
	}
	//short
	bool WriteInt16(int16_t n)
	{
		return Write(n);
	}
	//int
	bool WriteInt32(int32_t n)
	{
		return Write(n);
	}

	bool WriteFloat(float n)
	{
		return Write(n);
	}

	bool WriteDouble(double n)
	{
		return Write(n);
	}
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
