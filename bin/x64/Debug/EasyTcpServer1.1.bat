@echo off
::�����IP��ַ
::@set strIP=any
rem ����˶˿�
::@set nPort=4567
::��Ϣ�����߳���
::@set nThread=1
::�ͻ�������
::@set nClient=1

::�����IP��ַ
::��һ���Ⱥź��涼����ΪΪ�ַ������൱�� cmd = "strIP=any"
set cmd=strIP=127.0.0.1
:: ����˶˿�
set cmd=%cmd% nPort=4567	
::��Ϣ�����߳���
set cmd=%cmd% nThread=2		 
::�ͻ�������          
set cmd=%cmd% nClient=3		

set cmd=%cmd% -p		
::�������� �������
EasyTcpServer1.1 %cmd%

@pause