#ifndef _MEMORYMGR_HPP_
#define	_MEMORYMGR_HPP_
#include<stdlib.h>
#include<assert.h>
#include<mutex>


#ifdef _DEBUG
#include<stdio.h>

#define xPrintf(...) printf(__VA_ARGS__)

#else

#define xPrintf(x) 

#endif // DEBUG

#define  MAX_MEMORY_SIZE 1024

class MemoryAlloc;

//内存块 最小单元
class MemoryBlock
{
public:
	//内存块编号
	int nID;
	//引用次数
	int nRef;
	//所属大内存块（池）
	MemoryAlloc* pAlloc;
	//下一块位置
	MemoryBlock* pNext;
	//是否在内存池中
	bool bPool;
private:
	//预留
	char c1;
	char c2;
	char c3;
};
//int n = sizeof(MemoryBlock);

//内存池
class MemoryAlloc
{
public:
	MemoryAlloc()
	{
		_pBuf = nullptr;
		_pHeader = nullptr;
		_nSize = 0;
		_nBlockSize = 0;
	}

	~MemoryAlloc()
	{
		if (_pBuf)
			free(_pBuf);
	}

	//申请内存
	void* allocMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (!_pBuf)
		{
			initMemory();
		}
		MemoryBlock* pReturn = nullptr;
		if (nullptr == _pHeader)
		{
			pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));            
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
		}
		else {
			pReturn = _pHeader; 
			_pHeader = _pHeader->pNext;
			assert(0 == pReturn->nRef); 
			pReturn->nRef = 1;
		}
		//xPrintf("allocMemory: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		return ((char*)pReturn + sizeof(MemoryBlock));
	}

	//释放内存
	void freeMemory(void* pMem)
	{
		char* pDat = (char*)pMem;
		MemoryBlock* pBlock = (MemoryBlock*)(pDat - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);

		if (pBlock->bPool)
		{
			std::lock_guard<std::mutex> lg(_mutex);
			if (--pBlock->nRef != 0)
			{
				return;
			}
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else {
			if (--pBlock->nRef != 0)
			{
				return;
			}
			free(pBlock);
		}
	}
	//初始化
	void initMemory()
	{
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;
		//计算内存池的大小
		size_t bufSize = (_nSize + sizeof(MemoryBlock)) * _nBlockSize;
		//向系统申请池内存
		_pBuf = (char*)malloc(bufSize);
		//初始化内存池
		_pHeader = (MemoryBlock*)_pBuf;
		_pHeader->bPool = true;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->pAlloc = this;
		_pHeader->pNext = nullptr;

		//遍历内存块进行初始化
		MemoryBlock* pTemp1 = _pHeader;
		for (int n = 1; n < _nBlockSize; n++)
		{
			MemoryBlock*  pTemp2 = (MemoryBlock*)(_pBuf + (n * (_nSize + sizeof(MemoryBlock))));
			pTemp2->bPool = true;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->pAlloc = this;
			pTemp2->pNext = nullptr;
			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}
protected:
	//内存池地址
	char* _pBuf;
	//头部内存单元
	MemoryBlock* _pHeader;
	//内存单元大小
	size_t _nSize;
	//内存单元的数量
	size_t _nBlockSize;

	std::mutex _mutex;
};

//便于在声明成员变量时初始化MemoryAlloc的成员数据 
template<size_t nSize,size_t nBlockSize>
class MemoryAlloctor : public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		const size_t n = sizeof(void*);

		_nSize = (nSize/n)*n + (nSize % n ? n : 0);
		_nBlockSize = nBlockSize;
	}
};

//内存管理工具
class MemoryMgr
{
private:
	MemoryMgr() 
	{
		init_szAlloc(0, 64, &_mem64);
		init_szAlloc(65, 128, &_mem128);
		init_szAlloc(129, 256, &_mem256);
		init_szAlloc(257, 512, &_mem512);
		init_szAlloc(513, 1024, &_mem1024);
	}

	~MemoryMgr()
	{

	} 
	
public:  
	static MemoryMgr& Instance()
	{//单例模式
		static MemoryMgr mgr;
		return mgr;
	}
	//申请内存
	void* allocMem(size_t nSize)
	{
		if (nSize <= MAX_MEMORY_SIZE)
		{
			return _szAlloc[nSize]->allocMemory(nSize);
		}
		else
		{
			MemoryBlock* pReturn = nullptr;
			pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			//xPrintf("allocMem: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
			return ((char*)pReturn + sizeof(MemoryBlock));
		}
	}

	//释放内存
	void freeMem(void* pMem)
	{
		char* pDat = (char*)pMem;
		MemoryBlock* pBlock = (MemoryBlock*)(pDat - sizeof(MemoryBlock));
		//xPrintf("freeMem: %llx, id=%d\n", pBlock, pBlock->nID);
		if (pBlock->bPool)
		{
			pBlock->pAlloc->freeMemory(pMem);
		}
		else {
			if(--pBlock->nRef == 0)
				free(pBlock);
		}
	}
	//增加内存块的引用计数
	void addRef(void* pMem)
	{
		char* pDat = (char*)pMem;
		MemoryBlock* pBlock = (MemoryBlock*)(pDat - sizeof(MemoryBlock));
		++pBlock->nRef;
	}

private:
	//初始化内存池映射数组
	void init_szAlloc(int nBegin , int nEnd , MemoryAlloc* pMemA)
	{
		for (int n = nBegin; n <= nEnd; n++)
		{
			_szAlloc[n] = pMemA;
		}
	}
private:
	MemoryAlloctor<64, 100000> _mem64;
	MemoryAlloctor<128, 100000> _mem128;
	MemoryAlloctor<256, 100000> _mem256;
	MemoryAlloctor<512, 100000> _mem512;
	MemoryAlloctor<1024, 100000> _mem1024;
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE+ 1];
};


#endif