# 优化CELLTaskServer

## 1. function

类模版`std::function`是一种通用、多态的函数封装。`std::function`的实例可以对任何可以调用的目标实体进行存储、复制、和调用操作，这些目标实体包括普通函数、Lambda表达式、函数指针、以及其它函数对象等。`std::function`对象是对C++中现有的可调用实体的一种类型安全的包裹。

通过`std::function`对C++中各种可调用实体的封装，形成一个可调用的`std::function`对象，让我们不再纠结那么多的可调用实体，一切变得简单粗暴。

```c++
#include <functionl>

void funA()
{
    printf("funA\n");
}

void funB(int n)
{
    printf("funB : %d\n",n);
}

int funC(int a , int b)
{
    printf("funC : %d,%d\n",a,b);
    return 0;
}

int main()
{
    std::function<void()> callA = funA;
    std::function<void(int)> callB = funB;
    std::function<itn(int,int)> callC = funC;
    
    callA();
    callB(10);
    int c = callC(10,10);
    return 0;
}

/*-----------------------输出------------------------*/
//  funA
//	funB : 10
//	funC : 10,10
//	c = 0;
```

## 2. lambda表达式

```c++
[capture list] (params list) mutable exception-> return type { function body }
```

**[函数对象参数] (操作符重载函数参数) mutable 或 exception 声明 -> 返回值类型 {函数体}**

### 2.1  capture list ：捕获外部变量列表

​	

|  捕获形式   |                             说明                             |
| :---------: | :----------------------------------------------------------: |
|     []      |                      不捕获任何外部变量                      |
| [变量名, …] | 默认以**值**的形式捕获指定的多个外部变量（用逗号分隔），如果引用捕获，需要显示声明（使用&说明符） |
|   [this]    |                    以值的形式捕获this指针                    |
|     [=]     |                  以值的形式捕获所有外部变量                  |
|     [&]     |                  以引用形式捕获所有外部变量                  |
|   [=, &x]   |         变量x以引用形式捕获，其余变量以传值形式捕获          |
|   [&, x]    |         变量x以值的形式捕获，其余变量以引用形式捕获          |

### 2.2  params list 形参列表

1. **参数列表中不能有默认参数**
2. **不支持可变参数**
3. **所有参数必须有参数名**

### 2.3 mutable指示符

​	**用来说用是否可以修改捕获的变量,若想修改以值的形式传入的变量，则需要声明mutable关键字，该关键字用以说明表达式体内的代码可以修改值捕获的变量**

### 2.4 exception：异常设定

**exception 声明用于指定函数抛出的异常，如抛出整数类型的异常，可以使用 throw(int)**

### 2.5 return type : 返回类型

### 2.6 function body : 函数体

```c++
//[函数对象参数] (操作符重载函数参数) mutable 或 exception 声明 -> 返回值类型 {函数体}

int main()
{
	std:: function<void()> call;
    
    call = [](){
        printf("This is test\n");
    };
    
    call();
    return 0;
}

/*---------------------输出-------------------------*/
//  This is test
```



## 3. 优化

删除CELLTask类型，任务类型用匿名函数对象替代

### 3.1 CellTaskServer 更改：

```c++
//执行任务的服务类型
class CellTaskServer 
{
	typedef std::function<void()> CellTask;
private:
	//任务数据
	std::list<CellTask> _tasks;
	//任务数据缓冲区
	std::list<CellTask> _tasksBuf;
	//改变数据缓冲区时需要加锁
```

### 3.2 CellServer 更改：

```c++
void addSendTask(CellClient* pClient, netmsg_DataHeader* header)
{
	_taskServer.addTask([pClient, header]() {
		pClient->SendData(header);
		delete header;
	});
}
```

### 3.3 删除网络消息发送任务class CellSendMsg2ClientTask:public CellTask

