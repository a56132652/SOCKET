#ifndef _CELL_MSGSTREAM_HPP_
#define _CELL_MSGSTREAM_HPP_
#include "CELLStream.hpp"
#include "MessageHeader.hpp"

//��Ϣ�����ֽ��� 
class CELLReadStream : public CELLStream
{
public:
	CELLReadStream(DataHeader* header)
		:CELLStream((char*)header, header->dataLength)
	{

	}

	CELLReadStream(char* pData, int nSize, bool bDelete = false)
		:CELLStream(pData, nSize,bDelete)
	{
		push(nSize);
		////Ԥ�ȶ�ȡ��Ϣ����
		//ReadInt16();
		////Ԥ�ȶ�ȡ��Ϣ����
		//getNetCmd();
	}

	uint16_t getNetCmd()
	{
		uint16_t cmd = CMD_ERROR;
		Read<uint16_t>(cmd);
		return cmd;
	}

public:

};


class CELLWriteStream :public CELLStream
{
public:
	CELLWriteStream(char* pData, int nSize, bool bDelete = false)
		:CELLStream(pData, nSize, bDelete)
	{
		//Ԥ��ռ����Ϣ��������ռ�
		Write<uint16_t>(0);
	}

	CELLWriteStream(int nSize = 1024)
		:CELLStream(nSize)
	{
		//Ԥ��ռ����Ϣ��������ռ� 
		Write<uint16_t>(0);
	}

	void setNetCmd(uint16_t cmd)
	{
		Write<uint16_t>(cmd);
	}

	bool WriteString(const char* str, int len)
	{
		return WriteArray(str, len);
	}

	bool WriteString(const char* str)
	{
		return WriteArray(str, strlen(str));
	}

	bool WriteString(std::string& str)
	{
		return WriteArray(str.c_str(), str.length());
	}

	void finsh()
	{
		int pos = length();
		setWritePos(0);
		Write<uint16_t>(pos);
		setWritePos(pos);
	}
};

#endif // !_CELL_MSGSTREAM_HPP_
