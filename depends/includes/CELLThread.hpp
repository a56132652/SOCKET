#ifndef _CELL_THREAD_HPP_
#define _CELL_THREAD_HPP_

#include"CELLSemaphore.hpp"

class CELLThread
{
public:
	static void Sleep(time_t dt) 
	{
		std::chrono::milliseconds t(dt);
		std::this_thread::sleep_for(t);
	}
private:
	typedef std::function<void(CELLThread*)> EventCall;
public:
	void Start(EventCall onCreate = nullptr,
			   EventCall onRun = nullptr ,
		       EventCall onDestory = nullptr)
	{
		std::lock_guard<std::mutex> lock(_mutex); 
		if (!_isRun)
		{
			_isRun = true;
			if (onCreate)
				_onCreate = onCreate;
			if (onRun)
				_onRun = onRun;
			if (onDestory)
				_onDestory = onDestory; 

			std::thread t(std::mem_fn(&CELLThread::OnWork), this);
			t.detach();
		}
	}
	void Close()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun)
		{  
			_isRun = false;
			_sem.wait();
		}
	}
	//在工作函数中退出
	//不需要使用信号量来等待
	//如果使用会阻塞
	void Exit()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun)
		{
			_isRun = false;
		}
	}
	//线程是否处于运行中
	bool isRun()
	{
		return _isRun;
	}
protected:
	//线程运行时的工作函数
	void OnWork()
	{
		if (_onCreate)
			_onCreate(this);
		if (_onRun)
			_onRun(this);
		if (_onDestory)
			_onDestory(this);
		 
		_isRun = false;
		_sem.wakeup();
	}
private:
	EventCall _onCreate;
	EventCall _onRun;
	EventCall _onDestory;
	//不同线程中改变数据是需要加锁
	std::mutex _mutex;
	//控制线程的终止、退出
	CELLSemaphore _sem;
	//线程是否启动运行中
	bool _isRun = false;

};


#endif // !_CELL_THREAD_HPP_
