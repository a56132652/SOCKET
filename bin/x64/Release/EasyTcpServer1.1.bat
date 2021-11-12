::服务端IP地址
@set strIP=any
rem 服务端端口
@set nPort=4567
::消息处理线程数
@set nThread=1
::客户端上限
@set nClient=1

EasyTcpServer1.1 %strIP% %nPort% %nThread% %nClient%

@pause