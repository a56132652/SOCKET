#ifndef  _CELL_TASK_H_
#define  _CELL_TASK_H_
#include<thread>
#include<mutex>
#include<list>
#include<functional>
#include"CELLThread.hpp"


//执行任务的服务类型
class CELLTaskServer
{
public:
	//所属serverID
	int serverID = -1;
private:
	typedef std::function<void()> CELLTask;
private:
	//任务数据
	std::list<CELLTask> _tasks;
	//任务数据缓冲区
	std::list<CELLTask> _taskBuf;
	//改变数据缓冲区时需要加锁
	std::mutex _mutex;
	//
	CELLThread _thread;
public:
	//添加任务
	void addTask(CELLTask task)
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
		//CELLLog_Info("CELLTaskServer%d.Close begin", serverID);
		
		_thread.Close();

		//CELLLog_Info("CELLTaskServer%d.Close end", serverID);
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
				CELLThread::Sleep(1);
				continue;
			}
			//处理任务
			for (auto pTask : _tasks)
			{
				pTask();
			}
			//清空任务
			_tasks.clear();
		}
		//处理缓冲队列中的任务
		for (auto pTask : _taskBuf)
		{
			pTask();
		}
		//CELLLog_Info("CELLTaskServer%d.OnRun exit", serverID);
	} 
};

#endif //  _CELL_TASK_H_
