#ifndef  _CELL_TASK_H_
#define  _CELL_TASK_H_
#include<thread>
#include<mutex>
#include<list>
#include<functional>
#include"CELLThread.hpp"


//ִ������ķ�������
class CELLTaskServer
{
public:
	//����serverID
	int serverID = -1;
private:
	typedef std::function<void()> CELLTask;
private:
	//��������
	std::list<CELLTask> _tasks;
	//�������ݻ�����
	std::list<CELLTask> _taskBuf;
	//�ı����ݻ�����ʱ��Ҫ����
	std::mutex _mutex;
	//
	CELLThread _thread;
public:
	//�������
	void addTask(CELLTask task)
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
		printf("CELLTaskServer%d.Close begin\n", serverID);
		
		_thread.Close();

		printf("CELLTaskServer%d.Close end\n", serverID);
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
			//�������
			_tasks.clear();
		}
		printf("CELLTaskServer%d.OnRun exit\n", serverID);
	} 
};

#endif //  _CELL_TASK_H_
