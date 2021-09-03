#ifndef _CELLObjectPool_hpp_
#define _CELLObjectPool_hpp_
#include<memory>
#include<stdlib.h>
#include<mutex>
#include<assert.h>

#ifdef _DEBUG
	#ifndef xPrintf
		#include<stdio.h>
		#define xPrintf(...) printf(__VA_ARGS__)
	#endif // !xPrintf
#else
	#ifndef xPrintf
		#define xPrintf(...) 
	#endif 
#endif // DEBUG


//便于在声明成员变量时初始化MemoryAlloc的成员数据 
template<class Type, size_t nPoolSize>
class CELLObjectPool
{
private:
	class NodeHeader
	{
	public:
		//内存块编号
		int nID;
		//引用次数
		char nRef;
		//下一块位置
		NodeHeader* pNext;
		//是否在内存池中
		bool bPool;
	private:
		//预留
		char c2;
		char c3;
	};
public:
	CELLObjectPool()
	{
		_pBuf = nullptr;
		initPool();
	}

	~CELLObjectPool()
	{
		if(_pBuf)
			delete[] _pBuf;
	}
	//释放对象内存
	void freeObjMemory(void* pMem)
	{
		char* pDat = (char*)pMem;
		NodeHeader* pBlock = (NodeHeader*)(pDat - sizeof(NodeHeader));
		assert(1 == pBlock->nRef);
		pBlock->nRef = 0;
		if (pBlock->bPool)
		{   
			std::lock_guard<std::mutex> lg(_mutex);
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else {
			if (--pBlock->nRef != 0)
			{
				return;
			}
			delete[] pBlock;
		} 
	}
	//申请对象内存
	void* allocObjMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		NodeHeader* pReturn = nullptr;
		if (nullptr == _pHeader)
		{
			pReturn = (NodeHeader*)new char[sizeof(Type) + sizeof(NodeHeader)];
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pNext = nullptr;
		}
		else {
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;
			assert(0 == pReturn->nRef);
			pReturn->nRef = 1;
		}
		xPrintf("allocObjMemory: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		return ((char*)pReturn + sizeof(NodeHeader));
	}
private:
	//初始化对象池
	void initPool()
	{
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;
		//计算对象池大小
		size_t realSize = sizeof(Type) + sizeof(NodeHeader);
		size_t n = nPoolSize * realSize;
		//申请池内存
		_pBuf = new char[n];
		//初始化内存池
		_pHeader = (NodeHeader*)_pBuf;
		_pHeader->bPool = true;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->pNext = nullptr;
		//遍历内存块初始化
		NodeHeader* pTemp1 = _pHeader;
		for (int n = 1; n < nPoolSize; n++)
		{
			NodeHeader* pTemp2 = (NodeHeader*)(_pBuf + (n *realSize));
			pTemp2->bPool = true;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->pNext = nullptr;
			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}
	//对象池内存缓冲区地址地址
	char* _pBuf;
	//头部内存单元
	NodeHeader* _pHeader;
	//
	std::mutex _mutex;
};

template<class Type,size_t nPoolSize>
class ObjectPoolBase
{
public:
	ObjectPoolBase()
	{

	}
	~ObjectPoolBase()
	{
		  
	}

	void* operator new(size_t nSize)
	{
		return objectPool().allocObjMemory(nSize);
	}

	void operator delete(void* p)
	{
		return objectPool().freeObjMemory(p);
	}

	template<typename ...Args>
	static Type* createObject(Args ... args)
	{ 
		Type* obj = new Type(args...);

		//可以做点想做的事情

		return obj;
	}

	static void destroyObject(Type* obj)
	{
		delete(obj);
	}

private:
	typedef CELLObjectPool<Type, nPoolSize> ClassTypePool;
	static ClassTypePool& objectPool()
	{	//静态对象池对象 
		static ClassTypePool sPool;
		return sPool;
	 }
};




#endif // !_CELLObjectPool_hpp_
