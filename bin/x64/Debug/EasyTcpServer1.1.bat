@echo off
::服务端IP地址
::@set strIP=any
rem 服务端端口
::@set nPort=4567
::消息处理线程数
::@set nThread=1
::客户端上限
::@set nClient=1

::服务端IP地址
::第一个等号后面都将视为为字符串，相当于 cmd = "strIP=any"
set cmd=strIP=127.0.0.1
:: 服务端端口
set cmd=%cmd% nPort=4567	
::消息处理线程数
set cmd=%cmd% nThread=2		 
::客户端上限          
set cmd=%cmd% nClient=3		

set cmd=%cmd% -p		
::启动程序 传入参数
EasyTcpServer1.1 %cmd%

@pause