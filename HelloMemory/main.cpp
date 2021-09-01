#include"Alloctor.h"
#include<stdlib.h>
#include<iostream>
#include<thread>
#include<stdlib.h>
#include<mutex>
#include"CELLTimestamp.hpp"
#include<memory>

using namespace std;

mutex m;
//线程数
const int tCount = 8;
//最大申请次数
const int mCount = 1000000;
//线程平均申请次数
const int nCount = mCount / tCount;

void workFun(int index)
{

	char* data[nCount];
	for (size_t i = 0; i < nCount; i++)
	{
		data[i] = new char[1 + (rand()%128)];
	}

	for (size_t i = 0; i < nCount; i++)
	{
		delete[] data[i];
	}
}

class ClassA
{
public:
	ClassA()
	{
		printf("ClassA\n");
	}

	~ClassA(){
		printf("~ClassA\n");
	}

	int num = 0;
};

shared_ptr<ClassA> fun(shared_ptr<ClassA> pA)
{
	printf("2use_count=%d\n", pA.use_count());
	pA->num++;
	return pA;
}

int main()
{
	/*
	thread t[tCount];
	for (int i = 0; i < tCount; i++)
	{
		t[i] = thread(workFun, i);
	}
	CELLTimestamp tTime;
	for (int i = 0; i < tCount; i++)
	{
		t[i].join();
		//t[i].detach();
	}

	cout << tTime.getElapsedTimeInMilliSec()<< endl;
	cout << "hello,main Thread\n" << endl;
	system("pause");
	*/
	
	{
		shared_ptr<ClassA> b = make_shared<ClassA>();
		printf("1use_count=%d\n", b.use_count());
		b->num = 100;
		shared_ptr<ClassA> c = fun(b);
		printf("3use_count=%d\n", b.use_count());
		printf("num=%d\n", b->num);
	}
	 
	return 0;
}