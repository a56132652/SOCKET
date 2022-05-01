# Epoll网络模型

参考[(Epoll原理解析_~~ LINUX ~~-CSDN博客_epoll](https://blog.csdn.net/armlinuxww/article/details/92803381)

Seclect网络模型实现了100W条消息的稳定收发

Epoll网络模型主要适用于Linux系统及其衍生网络系统



**同时监视多个 Socket 的简单方法**

服务端需要管理多个客户端连接，而 Recv 只能监视单个 Socket，这种矛盾下，人们开始寻找监视多个 Socket 的方法。Epoll 的要义就是高效地监视多个 Socket。

从历史发展角度看，必然先出现一种不太高效的方法，人们再加以改进，正如 Select 之于 Epoll。先理解不太高效的 Select，才能够更好地理解 Epoll 的本质。

假如能够预先传入一个 Socket 列表，如果列表中的 Socket 都没有数据，挂起进程，直到有一个 Socket 收到数据，唤醒进程。这种方法很直接，也是 Select 的设计思想。

为方便理解，我们先复习 Select 的用法。在下边的代码中，先准备一个数组 FDS，让 FDS 存放着所有需要监视的 Socket。

然后调用 Select，如果 FDS 中的所有 Socket 都没有数据，Select 会阻塞，直到有一个 Socket 接收到数据，Select 返回，唤醒进程。

用户可以遍历 FDS，通过 FD_ISSET 判断具体哪个 Socket 收到数据，然后做出处理。 

```c++
int s = socket(AF_INET, SOCK_STREAM, 0);   
bind(s, ...) 
listen(s, ...) 
 
int fds[] =  存放需要监听的socket 
 
while(1){ 
    int n = select(..., fds, ...) 
    for(int i=0; i < fds.count; i++){ 
        if(FD_ISSET(fds[i], ...)){ 
            //fds[i]的数据处理 
        } 
    } 
```

**操作系统创建Socket**：这个Socket对象包含了**发送缓冲区**、**接收缓冲区**以及**等待队列**，**等待队列是个非常重要的结构，它指向所有需要等待该 Socket 事件的进程。**

假设进程A创建了一个Socket，并在执行Recv时阻塞，此时操作系统就会将进程A从工作队列移动到该Socket的等待队列中，该进程进入阻塞状态；当 Socket 接收到数据后，操作系统将该 Socket 等待队列上的进程重新放回到工作队列，该进程变成运行状态，继续执行代码

当进程A创建了多个Socket，即sock1、sock2、sock3，在调用Select后，操作系统会把进程A分别加入这三个Socket的等待队列中，当任何一个Socket收到数据后，中断程序将唤醒进程，即**将进程A从所有等待队列中移除**，加入到工作队列中。

经由这些步骤，当进程 A 被唤醒后，它知道至少有一个 Socket 接收了数据。程序只需遍历一遍 Socket 列表，就可以得到就绪的 Socket。

**但是该方法有几个主要缺点：**

- 每次调用 Select 都需要将进程加入到所有监视 Socket 的等待队列，每次唤醒都需要从每个队列中移除。这里涉及了两次遍历，而且每次都要将整个 FDS 列表传递给内核，有一定的开销。

**正是因为遍历操作开销大，出于效率的考量，才会规定 Select 的最大监视数量，默认只能监视 1024 个 Socket。**

- 进程被唤醒后，程序并不知道哪些 Socket 收到数据，还需要遍历一次。

**而如何减少遍历，如何保存保存就绪Socket，就是Epoll技术需要解决的问题**

## Epoll的设计思路

- **功能分离**

Select 低效的原因之一是将“**维护等待队列**”和“**阻塞进程**”两个步骤合二为一。

然而大多数应用场景中，需要监视的 Socket 相对固定，并不需要每次都修改。

Epoll 将这两个操作分开，先用 epoll_ctl 维护等待队列，再调用 epoll_wait 阻塞进程。显而易见地，效率就能得到提升。

**Epoll用法**

```c++
int s = socket(AF_INET, SOCK_STREAM, 0);    
bind(s, ...) 
listen(s, ...) 
 
 //用 epoll_create 创建一个 Epoll 对象 Epfd
int epfd = epoll_create(...); 
 //将所有需要监听的socket添加到epfd中 
epoll_ctl(epfd, ...);
 
while(1){ 
    //调用 epoll_wait 等待数据
    int n = epoll_wait(...) 
    for(接收到数据的socket){ 
        //处理 
    } 
} 
```

- **就绪列表**

Select 低效的另一个原因在于程序不知道哪些 Socket 收到数据，只能一个个遍历。如果内核维护一个“就绪列表”，引用收到数据的 Socket，就能避免遍历。

假设计算机共有三个 Socket，收到数据的 Sock2 和 Sock3 被就绪列表 Rdlist 所引用。当进程被唤醒后，只要获取 Rdlist 的内容，就能够知道哪些 Socket 收到数据。

## Epoll函数详解

- int epoll_create(int size)

创建一个epoll的句柄，size用来告诉内核这个监听的数目一共有多大。这个参数不同于select()中的第一个参数，给出最大监听的fd+1的值。需要注意的是，当创建好epoll句柄后，它就是会占用一个fd值，在linux下如果查看/proc/进程id/fd/，是能够看到这个fd的，所以在使用完epoll后，必须调用close()关闭，否则可能导致fd被耗尽。

- int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

  - int epfd : epoll_create()返回值，epoll句柄

  - int op：表示动作，用 3 个宏来表示

    - EPOLL_CTL_ADD：注册新的fd到epfd中
    - EPOLL_CTL_MOD：修改已经注册的fd的监听事件
    - EPOLL_CTL_DEL：从epfd中删除一个fd

  - int fd ： 需要监听的 fd

  - struct epoll_event *event：告诉内核需要监听什么事，结构如下：

    - ```c++
      typedef union epoll_data {
          void *ptr;
          int fd;
          __uint32_t u32;
          __uint64_t u64;
      } epoll_data_t;
      
      struct epoll_event {
          __uint32_t events; /* Epoll events */
          epoll_data_t data; /* User data variable */
      };
      ```

    - `events`可以是几个宏的集合

      - EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭
      - EPOLLOUT：表示对应的文件描述符可以写；
      - EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
      - EPOLLERR：表示对应的文件描述符发生错误；
      - EPOLLHUP：表示对应的文件描述符被挂断；
      - EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
      - EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里

- int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout)

  - 等待事件发生，类似于seclect()调用
  - int epfd ： epoll句柄
  - struct epoll_event * events ：用来从内核得到事件的集合
  - int maxevents ： 告之内核这个events有多大，该值不能大于创建epoll_create()时的size
  - int timeout ： 超时时间（毫秒，0会立即返回，-1将不确定，也有说法说是永久阻塞）
  - 返回值：该函数返回需要处理的事件数目，如返回0表示已超时。

## 基本Epoll示例模板程序

```c++
    for( ; ; )
    {
        nfds = epoll_wait(epfd,events,20,500);
        for(i=0;i<nfds;++i)
        {
            if(events[i].data.fd==listenfd) //有新的连接
            {
                connfd = accept(listenfd,(sockaddr *)&clientaddr, &clilen); //accept这个连接
                ev.data.fd=connfd;
                ev.events=EPOLLIN|EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev); //将新的fd添加到epoll的监听队列中
            }
            else if( events[i].events&EPOLLIN ) //接收到数据，读socket
            {
                n = read(sockfd, line, MAXLINE)) < 0    //读
                ev.data.ptr = md;     //md为自定义类型，添加数据
                ev.events=EPOLLOUT|EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);//修改标识符，等待下一个循环时发送数据，异步处理的精髓
            }
            else if(events[i].events&EPOLLOUT) //有数据待发送，写socket
            {
                struct myepoll_data* md = (myepoll_data*)events[i].data.ptr;    //取数据
                sockfd = md->fd;
                send( sockfd, md->ptr, strlen((char*)md->ptr), 0 );        //发送数据
                ev.data.fd=sockfd;
                ev.events=EPOLLIN|EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev); //修改标识符，等待下一个循环时接收数据
            }
            else
            {
                //其他的处理
            }
        }
    }
```



## Epoll的两种触发模式

- LT（水平触发）模式下，只要这个文件描述符还有数据可读，每次 epoll_wait都会返回它的事件，提醒用户程序去操作；**只要缓冲区有数据，就会一直触发**


- ET（边缘触发）模式下，在它检测到有 I/O 事件时，通过 epoll_wait 调用会得到有事件通知的文件描述符，对于每一个被通知的文件描述符，如可读，则必须将该文件描述符一直读到空，让 errno 返回 EAGAIN 为止，否则下次的 epoll_wait 不会返回余下的数据，会丢掉事件。如果ET模式不是非阻塞的，那这个一直读或一直写势必会在最后一次阻塞。**触发一次需要用户读完，若未读完缓冲区数据，无数据到来时，不会再触发**

- 两种触发方式优劣：

  - 如果采用EPOLLLT模式的话，系统中一旦有大量你不需要读写的就绪文件描述符，它们每次调用epoll_wait都会返回，这样会大大降低处理程序检索自己关心的就绪文件描述符的效率.。而采用EPOLLET这种边缘触发模式的话，当被监控的文件描述符上有可读写事件发生时，epoll_wait()会通知处理程序去读写。如果这次没有把数据全部读写完(如读写缓冲区太小)，那么下次调用epoll_wait()时，它不会通知你，也就是它只会通知你一次，直到该文件描述符上出现第二次可读写事件才会通知你！！！这种模式比水平触发效率高，系统不会充斥大量你不关心的就绪文件描述符。

  

## Epoll原理与工作流程

1. 创建epoll_create方法时，内核会创建一个eventpoll对象，即程序中 Epfd 所代表的对象，eventpoll 对象也是文件系统中的一员，和 Socket 一样，它也会有等待队列，**它还包含一个就序列表rdlist，用于保存就绪Socket的引用**。

   - ```c++
     struct eventpoll {
     　　...
     　　/*红黑树的根节点，这棵树中存储着所有添加到epoll中的事件，
     　　也就是这个epoll监控的事件*/
     　　struct rb_root rbr;
     　　/*双向链表rdllist保存着将要通过epoll_wait返回给用户的、满足条件的事件*/
     　　struct list_head rdllist;
     　　...
     };
     ```

     

2. 维护监视列表：创建 Epoll 对象后，可以用 epoll_ctl 添加或删除所要监听的 Socket。如果通过epoll_ctl 添加 Sock1、Sock2 和 Sock3 的监视，内核会将 eventpoll 添加到这三个 Socket 的等待队列中。当Socket 收到数据后，中断程序会操作 eventpoll 对象，而不是直接操作进程，**select就会直接操作进程，将进程从所有socket等待列表中移除，将进程放入工作队列**。

3. 接收数据：当 Socket 收到数据后，中断程序会给 **eventpoll 的“就绪列表”**添加 Socket 引用。 当Sock2 和 Sock3 收到数据后，中断程序让 Rdlist 引用这两个 Socket，**eventpoll 对象相当于 Socket 和进程之间的中介**，**Socket 的数据接收并不直接影响进程，而是通过改变 eventpoll 的就绪列表来改变进程状态。**当程序执行到 epoll_wait 时，如果 Rdlist 已经引用了 Socket，那么 epoll_wait 直接返回，如果 Rdlist 为空，阻塞进程

4. 进程阻塞和唤醒：假设计算机中正在运行进程 A 和进程 B，在某时刻进程 A 运行到了 epoll_wait 语句，内核会将进程 A 放入 eventpoll 的等待队列中，阻塞进程，当 Socket 接收到数据，中断程序一方面修改 Rdlist，另一方面唤醒 eventpoll 等待队列中的进程，进程 A 再次进入运行状态



## 就绪列表的数据结构

就绪列表引用着就绪的 Socket，所以它应能够快速的插入数据。程序可能随时调用 epoll_ctl 添加监视Socket，也可能随时删除。当删除时，若该 Socket 已经存放在就绪列表中，它也应该被移除。所以就绪列表应是一种能够快速插入和删除的数据结构。双向链表就是这样一种数据结构，Epoll 使用双向链表来实现就绪队列。

## 索引结构

既然 Epoll 将“维护监视队列”和“进程阻塞”分离，也意味着需要有个数据结构来保存监视的 Socket，至少要方便地添加和移除，还要便于搜索，以避免重复添加。红黑树是一种自平衡二叉查找树，搜索、插入和删除时间复杂度都是 O(log(N))，效率较好，Epoll 使用了红黑树作为索引结构

## 总结

**Epoll 在 Select 和 Poll 的基础上引入了 eventpoll 作为中间层，使用了先进的数据结构，是一种高效的多路复用技术**