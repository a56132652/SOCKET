#ifndef  _CELL_TASK_H_

#include<thread>
#include<mutex>
#include<list>
#include <functional>
//��������-����
class CellTask
{
public:
	CellTask()
	{

	}
	virtual ~CellTask()
	{

	}
	//ִ������
	virtual void doTask()
	{

	}

private:

};

typedef std::shared_ptr<CellTask> CellTaskPtr;
//ִ������ķ�������
class CellTaskServer
{
private:
	//��������
	std::list<CellTaskPtr> _tasks;
	//�������ݻ�����
	std::list<CellTaskPtr> _taskBuf;
	//�ı����ݻ�����ʱ��Ҫ����
	std::mutex _mutex;

public:
	CellTaskServer()
	{

	}
	~CellTaskServer()
	{

	}
	//�������
	void addTask(CellTaskPtr& task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_taskBuf.push_back(task);
	}
	//���������߳�
	void Start()
	{
		std::thread t(std::mem_fn(&CellTaskServer::OnRun), this);
		t.detach();
	}
protected:
	//��������
	void OnRun()
	{
		while (true)
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
				pTask->doTask();
			}
			_tasks.clear();
		}

	}

};

#endif //  _CELL_TASK_H_
