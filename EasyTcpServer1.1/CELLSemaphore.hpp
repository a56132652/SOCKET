#ifndef _CELL_SEMAPHORE_HPP_
#define _CELL_SEMAPHORE_HPP_
#include<chrono>
#include<thread>

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
		_isWaitExit = true;

		while (_isWaitExit)
		{
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
		}
	}

	void wakeup()
	{
		_isWaitExit = false;
	}
private:
	bool _isWaitExit = false;
};


#endif // !_CELL_SEMAPHORE_HPP_
