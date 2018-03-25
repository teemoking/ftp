# ftp
文件传输项目基于tcp/ip协议，通过流式套接字编程和MySQL数据库，实现文件的上传下载删除、用户管理等功能，同时采用秒传，断点重传的技术提高效率。
	上传（update）
客户端 update+filename    普通文件   文件大小   文件MD码  	向服务器发送信息
客户端发送：Update  a.txt 	服务器接收：update  a.txt  190  32023101301010313
  
	删除(rm)
客户端 rm+filename
 
	切换目录（cd） 
对客户端每次将要发送的数据进行处理，保证客户端发送的信息是在用户目录下进行的。
客户端：cd /local
客户端：update c.txt		服务器接收：update /local/c.txt
 
	查看当前文件信息（ls）
在客户端发送ls ，系统将fork一个进程，通过匿名管道将输出信息返回父进程，由父进程将信息发回客户端。
 
	秒传
客户端上传文件的MD5码和服务目录下所有文件的MD5码进行比较，若存在相同的文件，则在当前路径下建立硬链接，同时向客户端发送上传完成的消息。
 
	文件夹上传
目录文件 发送文件夹名   	递归发送目录下的普通文件和目录文件
客户端发送：Update  local         服务器接收：update local  derictory
 
	断点重传
未上传完成的文件名均为filename.ufin，若客户端再一次上传，服务器检查当前路径下发现存在未上传完成的同名文件，确定未上传完成的文件大小，使客户端从上次发送结束的地方发送。文件上传完成更改文件名。
 
