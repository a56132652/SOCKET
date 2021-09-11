#ifndef _CELL_SEMAPHORE_HPP_
#define _CELL_SEMAPHORE_HPP_
#include<chrono>
#include<thread>
#include<mutex>
#include<condition_variable>

class CellSemaphore
{
public:
	CellSemaphore()
	{

	}
	~CellSemaphore()
	{

	}

	void wait()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (--_wait < 0)
		{
			//阻塞等待
			_cv.wait(lock, [this]()->bool{
				return _wakeup > 0;
				});
			--_wakeup;  
		}


	}

	void wakeup()
	{
		std::lock_guard <std::mutex> lock(_mutex);
		if (++_wait <= 0)
		{
			++_wakeup;
			_cv.notify_one();
		}
	}
private:
	std::mutex _mutex;
	//条件变量--阻塞等待
	std::condition_variable _cv;
	//等待计数
	int _wait = 0;
	//唤醒计数
	int _wakeup = 0;
};


#endif // !_CELL_SEMAPHORE_HPP_
