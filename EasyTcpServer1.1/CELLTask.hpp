#ifndef  _CELL_TASK_H_
#define  _CELL_TASK_H_
#include<thread>
#include<mutex>
#include<list>
#include<functional>
#include"CELLThread.hpp"


//ִ������ķ�������
class CellTaskServer
{
public:
	//����serverID
	int serverID = -1;
private:
	typedef std::function<void()> CellTask;
private:
	//��������
	std::list<CellTask> _tasks;
	//�������ݻ�����
	std::list<CellTask> _taskBuf;
	//�ı����ݻ�����ʱ��Ҫ����
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
	//�������
	void addTask(CellTask task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_taskBuf.push_back(task);
	}
	//���������߳�
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
	//��������
	void OnRun(CELLThread* pThread)
	{
		while (pThread->isRun())
		{
			//�ӻ�����ȡ������
			if (!_taskBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _taskBuf)
				{
					_tasks.push_back(pTask);
				}
				_taskBuf.clear();
			}
			//���û������
			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			//��������
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
