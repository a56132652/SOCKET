#ifndef _CELL_STREAM_HPP_
#define _CELL_STREAM_HPP_
#include<cstdint>
#include"CELLLog.hpp"
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
	//��ȡRead
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
		//������־
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
		//��ȡ����Ԫ�ظ���
		uint32_t len1 = 0;
		Read(len1,false);
		if (len1 < len)
		{
			//���������ֽڳ���
			auto nLen = len1 * sizeof(T);
			//�ж��ܲ��ܶ���
			if (canRead(nLen))
			{
				//�����Ѷ�λ��+���鳤��ռ�пռ�
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

	

	 
	////д��Write
	template<typename T>
	bool Write(T n)	//int
	{
		//����Ҫд�����ݵĴ�С
		size_t nLen = sizeof(T);
		//�ж��ܲ���д��
		if (canWrite(nLen))
		{
			//��Ҫд������ݿ�����������β��
			memcpy(_pBuff + _nWritePos, &n, nLen);
			//����β��λ���ƶ�
			push(nLen);
			return true;
		}
		CELLLog::Info("error, CELLStream::Write failed.");
		return false;
	}

	template<typename T>
	bool WriteArray(T* pData, uint32_t len)	//֮����ѡ��uint32_t��������size_t,��Ϊsize_t��32λ��64λϵͳ�´�С��ͬ�������鳤�ȴ���uint32_t����ʾ�����Χ����Ӧ�ý��зְ�
	{
		//����Ҫд��������ֽڳ��� 
		auto nLen = sizeof(T) * len;
		if (canWrite(nLen + sizeof(uint32_t)))
		{
			//��д�����鳤�ȣ������ȡ����
			WriteInt32(len);
			//��д������ʵ������
			memcpy(_pBuff + _nWritePos, pData, nLen);
			//����β��λ���ƶ�
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
