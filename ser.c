/*
*@文件传输项目基于tcp/ip协议，通过流式套接字编程和MySQL数据库，实现文件的上传下载删除、用户管理等功能，同时采用秒传，断点重传的技术提高效率。
*@ 
*@编译环境 ：linux 5.6  gcc -o ser ser.c md5.h md5.c -lpthread 
*@开始时间：2018-3-1
*@基本完成：2018年3月25日17:22:19
*@author: AK
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define		HOST_PORT	6000
#define		HOST_IP     "127.0.0.1"
#define		MAX_LISTEN	5
#define		BUFF_SIZE   512
#define		ARGV_COUNT  10
#define		PATH_SIZE   128
#define     COMMAND_SIZE  128
#define		MD5_BUFF_SIZE  33
#define 	FIEL_NAME_SIZE 50
#define 	LOCAL_DIR    "/home/teemo/mytest/programe/ftp/server"
int create_sockfd();//创建tcp流式套接字
void* thread_handle(void *arg);//多线程处理客户端与服务器交互，arg为连接套接字描述符
void updatefile(int c,char* argv[]);//上传文件，c为连接套接字描述符，argv为处理后的上传信息
void update(int c,char* argv[]);//处理上传的文件或文件夹信息，c为连接套接字描述符，argv为处理后的上传信息
void downloadfile(int c,char* argv[]);//下载文件
void copyprocess(int c,char* argv[]);//系统命令的实现，c为连接套接字描述符，argv为处理后的上传信息
int flashfile(char* upmd5,char* local,char* newpath);//判断上传文件是否为已存在文件,upmd5为上传文件的md5码，local为当前文件的目录，返回1为可以秒传，返回0为不可以秒传

int main()
{
	int sockfd = create_sockfd();//创建套接字，监听
	assert(sockfd != -1);
	

	while(1)
	{
		struct sockaddr_in sa;
		int len = sizeof(struct sockaddr_in);
		int c = accept(sockfd,(struct sockaddr*)&sa,&len);//连接套接字
		if(c <= 0)
		{
			continue;
		}
		pthread_t pt;
		pthread_create(&pt,NULL,thread_handle,(void*)c);//多线程提高并发
		pthread_detach(pt);
	}
	
	exit(0);
}




int create_sockfd()
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	assert(sockfd != -1);

	struct sockaddr_in sa;
	memset(&sa,0,sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(HOST_PORT);
	sa.sin_addr.s_addr = inet_addr(HOST_IP);
	
	int res = bind(sockfd,(struct sockaddr*)&sa,sizeof(sa));//绑定监听套接字，127.0.0.1:6000
	assert(res != -1);

	listen(sockfd,MAX_LISTEN);
	return sockfd;
}





void* thread_handle(void *arg)
{
	int c = (int)arg;
	//char path_cur[PATH_SIZE] = ".";//当前路径
	while(1)
	{
		//char path_cli[PATH_SIZE] = {0};//用户路径
		char buff[BUFF_SIZE] = {0};
		int count = recv(c,buff,BUFF_SIZE-1,0);
		if(count <= 0)//客户端是否断开
		{
			printf("one cli over\n");
			break;
		}
		printf("recv :%s\n",buff);//显示收到的信息

		char* argv[ARGV_COUNT] = {0};//对收到的信息进行字符串处理
		char *p = NULL;
		int i = 1;
		argv[0] = strtok_r(buff," ",&p);
		for( i; i<ARGV_COUNT; i++)
		{			
			//if(*p == '\0') break;
			argv[i] = strtok_r(NULL," ",&p);
		}	
		
		//切换到当前用户目录
		//char path[PATH_SIZE] = {0};
		/*if(argv[1] != NULL)	
		{
			strcat(path_cli,path_cur);
			strcat(path_cli,"/");
			strcat(path_cli,argv[1]);
			argv[1] = path_cli;
		}

		if(strcmp(argv[0],"cd") == 0 )
		{
			strcpy(path_cur,argv[1]);
		}
		else */
		if(strcmp(argv[0],"update") == 0)//上传文件
		{
			update(c,argv);
		}
		else if(strcmp(argv[0],"download") == 0)//下载文件
		{
			downloadfile(c,argv);
		}
		else//系统命令，存在 ps -ef | grep ./ser类似的命令
		{
			/*char* argv[3] = {0};
			char *p = NULL;			
			argv[0] = strtok_r(buff," ",&p);
			argv[1] = strtok_r(NULL," ",&p);
			argv[2] = strtok_r(NULL," ",&p);
			i = 0;
			for(; i<ARGV_COUNT;i++)
			{
				if(strcmp(argv[i],"./") == 0)
				argv[i] = NULL;
			}
			*/
			copyprocess(c,argv);
		}
		//send(c,"ok",2,0);
	}
	close(c);//终止连接，关闭套接字
	pthread_exit(NULL);//退出线程
}


void update(int c,char* argv[])
{
	if(strcmp(argv[2],"directory") == 0)//文件夹
	{
		char path[PATH_SIZE] = {0};
		sprintf(path,"./%s",argv[1]);
		mkdir(path,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);//创建普通文件
		send(c,"ok",2,0);
	}
	else//普通文件直接上传
	{
		updatefile(c,argv);
		//printf("updatefile %s\n",argv[1]);
	}
}

void updatefile(int c,char* argv[])
{
	/*char *argv[4] = {0};
	char *p = NULL;
	argv[0] = strtok_r(buff," ",&p);
	argv[1] = strtok_r(NULL," ",&p);
	argv[2] = strtok_r(NULL," ",&p);
	argv[3] = strtok_r(NULL," ",&p);
	struct dirent **namelist;
	int n = scandir(".",&namelist,0,alphasort);
	char buffmd5[32] = {0};
	Compute_file_md5(argv[1],buffmd5);
	*/
	/*DIR *dp;
	struct *dirent dir;
	struct stat st;
	dp = opendir(".");
	chdir(dp);
	dir = readdir(dp);
	
	lstat(dir->d_name,st);
	if(S_ISDIR(st))
	{
		char dirct[40] = {0};
		sprintf(dirct,"dirctory %s",dir->d_name);
	 	send(c,dirct,strlen(dirct),0);
	}
	struct stat st;
	char tem[50] = {0};
	sprintf(tem,"./%s",argv[1]);
	lstat(tem,&st);
	if(S_ISDIR(st))*/

	
	if(flashfile(argv[3],LOCAL_DIR,argv[1]) == 1)//通过计算文件的MD5码，判断是否可以进行秒传
	{
		//printf("一个文件已秒传\n");
		send(c,"final",5,0);	
		return ;
	}
	
	char oldname[FIEL_NAME_SIZE] = {0};
	strcpy(oldname,argv[1]);
	char newname[FIEL_NAME_SIZE] = {0};
	strcpy(newname,argv[1]);
	strcat(newname,".ufiu");
	
	int fd = open(newname,O_CREAT|O_WRONLY,0600);
	assert(fd != -1);

	int tem = lseek(fd,0,SEEK_END);
	char recv_ok[BUFF_SIZE] = {0};
	sprintf(recv_ok,"okok#%d",tem);
	
	send(c,recv_ok,strlen(recv_ok),0);	//服务器确认可以上传
	int filesize = atoi(argv[2])-tem;
	while(filesize > 0)
	{
		int count = 0;
		char buff[BUFF_SIZE] = {0};
		if(filesize > BUFF_SIZE)
		{	
			count  = recv(c,buff,BUFF_SIZE-1,0);
		}
		else//文件接收剩余大小 <= buff的大小
		{
			count  = recv(c,buff,filesize,0);
		}
		if(count <= 0)//客户端关闭
		{
			close(fd);
			printf("one client over\n");
			return;
		}
		write(fd,buff,count);
		filesize -= count;//防止接收发送黏包
	}
	send(c,"final",5,0);
	rename(newname,oldname);
	close(fd);
}

void downloadfile(int c,char* argv[])
{
	printf("downloadfile ...  \n");
}
void copyprocess(int c,char* argv[])
{
	int fd[2];
	pipe(fd);//采用匿名管道，进行父子进程间通信

	pid_t pid = fork();//fork 复制进程
	assert(pid != -1);
	if(pid == 0)
	{
		close(fd[0]);//关闭读端
		dup2(fd[1],1);//绑定标准输出和标准错误输出为写端
		dup2(fd[1],2);
		
		printf("    \n");
		int res = execvp(argv[0],argv);//替换要执行的命令
		if( res = -1 ) printf("command error\n");//命令替换出现错误
		//printf("");
		close(fd[1]);//
		exit(0);
	}

	wait(NULL);//防止出现僵死进程
	close(fd[1]);//关闭写端
	char buff[BUFF_SIZE] = {0};
	int command = read(fd[0],buff,BUFF_SIZE-1);
	while(command > 0)
	{	
		char com_num[COMMAND_SIZE] = {0};//发送从管道中读出的信息大小
		sprintf(com_num,"%d",command);
		send(c,com_num,strlen(com_num),0);
		char rev_num[COMMAND_SIZE] = {0};
		recv(c,rev_num,COMMAND_SIZE-1,0);//客户端确认
		if(strcmp(com_num,rev_num) != 0)
		break;

		send(c,buff,command,0);//发送管道信息
		memset(buff,0,BUFF_SIZE);
		command = read(fd[0],buff,BUFF_SIZE-1);
	}
	
	//send(c,"0",1,0);
	//printf("one client over\n");
	close(fd[0]);
}


int flashfile(char* upmd5,char* local,char* newpath)//生成文件的md5码,upmd5为上传文件的md5码，local为当前目录
{
	/*
	struct dirent **namelist;
	int n = scandir(".",&namelist,0,alphasort);

	//int fd = open("md5.txt",O_CREATE|O_WRONLY,0600);
	int i = 0;
	for(; i<n; i++)
	{
		char buff[32] = {0};
		Compute_file_md5(namelist[i+1]->d_name,buff);
		//write(fd,buff,32);
		//printf("%s",namelist[i+1]->d_name);
		if((strcmp(argv,buff)) == 0)
		{
			printf("update file md5  =%s\n");
			printf("%s  md5  = %s\n",namelist[i+1]->d_name,buff);
			return 1;
		}
		free(namelist[i+1]);
	}
	free(namelist);
	
	int rfd = open("md5.txt",O_RDONLY);
	while()
	char md5buff[32] = {0};
	read(rfd,md5buff,32);
	*/
	//dirfile(local);
	DIR* dp;
	struct dirent *dir;
	struct stat sta;
	dp = opendir(local);//打开当前目录流
	if(dp == NULL)//打开文件不能为空
	{
		printf("opendir error %s can't open\n",local);
		return 0;
	}
	
	while((dir = readdir(dp)) != NULL)
	{
		char path[PATH_SIZE] = {0};
		sprintf(path,"%s/%s",local,dir->d_name);
		//strcat(path,"/");
		lstat(path,&sta);//文件属性判断
		if(strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0 ) continue;
		if(S_ISDIR(sta.st_mode)) //文件夹递归处理
		{
			char new_local[PATH_SIZE] = {0};
			sprintf(new_local,"%s/%s",local,dir->d_name);
			flashfile(upmd5,new_local,newpath);
		}
		else if(S_ISREG(sta.st_mode))//利用md5判断是否存在相同的文件
		{
			//strcat(path,dir->d_name);
			char md5_buff[MD5_BUFF_SIZE] = {0};
			Compute_file_md5(path,md5_buff);
			if(strcmp(md5_buff,upmd5) == 0)
			{
				link(path,newpath);
				//printf("%s\n%s\n",buff,argv);//确认md5
				return 1;
			}
		}
	}
	closedir(dp);//关闭目录流
	return 0;
}
