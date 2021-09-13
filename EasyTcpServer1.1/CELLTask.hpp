#ifndef  _CELL_TASK_H_
#define  _CELL_TASK_H_
#include<thread>
#include<mutex>
#include<list>
#include<functional>
#include"CELLThread.hpp"


//执行任务的服务类型
class CellTaskServer
{
public:
	//所属serverID
	int serverID = -1;
private:
	typedef std::function<void()> CellTask;
private:
	//任务数据
	std::list<CellTask> _tasks;
	//任务数据缓冲区
	std::list<CellTask> _taskBuf;
	//改变数据缓冲区时需要加锁
	std::mutex _mutex;
	//
	CELLThread _thread;
public:
	CellTaskServer()
	{

	}
	~CellTaskServer()
	{

	}
	//添加任务
	void addTask(CellTask task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_taskBuf.push_back(task);
	}
	//启动工作线程
	void Start()
	{
		_thread.Start(nullptr, [this](CELLThread* pThread) {OnRun(pThread);});
	}

	void Close()
	{
		printf("CellTaskServer%d.Close begin\n", serverID);
		
		_thread.Close();

		printf("CellTaskServer%d.Close end\n", serverID);
	}
protected:
	//工作函数
	void OnRun(CELLThread* pThread)
	{
		while (pThread->isRun())
		{
			//从缓冲区取出数据
			if (!_taskBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _taskBuf)
				{
					_tasks.push_back(pTask);
				}
				_taskBuf.clear();
			}
			//如果没有任务
			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			//处理任务
			for (auto pTask : _tasks)
			{
				pTask();
			}
			_tasks.clear();
		}
		printf("CellTaskServer%d.OnRun exit\n", serverID);
	} 
};

#endif //  _CELL_TASK_H_
