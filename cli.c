/*
*@文件传输项目基于tcp/ip协议，通过流式套接字编程和MySQL数据库，实现文件的上传下载删除、用户管理等功能，同时采用秒传，断点重传的技术提高效率。
*@ 
*@编译环境 ：linux 5.6  gcc -o cli cli.c md5.h md5.c 
*@开始时间：2018-3-1
*@基本完成：2018年3月25日17:22:19
*@author: AK
*/
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define		HOST_PORT	6000
#define		HOST_IP     "127.0.0.1"
#define		BUFF_SIZE   512
#define		PATH_SIZE   128
#define		ARGV_COUNT  10
#define		FIEL_SIZE	128
#define		FIEL_MD5_SIZE	33

void file_handle(int sockfd,char cli_req[],char* path_file);//判断是普通文件或目录，进行不同的处理，sockfd为连接套字,cli_req为客户请求，path_file为当前路径文件
void file_sent(int sockfd,char cli_req[],char* path_file);//发送普通文件
void work_handle(int sockfd,char cli_req[],char* client_path);//对用户输入的信息进行处理,进行上传下载，帮助信息
void flashmd5(char* path_file,char* md5_buff);//生成上传文件的MD5码,path_file为路径文件，md5_buff为上传文件的md5码
void client_cmd(int sockfd);//接收服务执行系统命令的结果,sockfd为连接套接字

int main()
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	assert(sockfd != -1);

	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(HOST_PORT);
	sa.sin_addr.s_addr = inet_addr(HOST_IP);
	
	int res = connect(sockfd,(struct sockaddr*)&sa,sizeof(sa));//绑定连接套接字
	assert(res != -1);
	
	char path[PATH_SIZE] = {0};
	while(1)
	{
		int count = 0;
		char buff[BUFF_SIZE] = {0};
		char path_cur[PATH_SIZE] = "./";
		strcat(path_cur,path);
		printf("input command  ");
		fflush(stdin);
		fgets(buff,BUFF_SIZE-1,stdin);

		//int len = strlen(buff);
		if(strcmp(buff,"\n") == 0)//空命令
		continue;
		strtok(buff,"\n");

		
		if(strncmp(buff,"cd",2) == 0)//切换用户目录
		{
			//cd
			strcpy(path,buff+3);
			//strcat(path,buff+3,strlen(buff)-3);
		}
		else//上传下载文件，帮助信息
		{
			work_handle(sockfd,buff,path_cur);
		}

		/*memset(buff,0,512);
		count = recv(sockfd,buff,512,0);
		if(count <= 0)  	break;
		int command = 0; 
		while((command = atoi(buff)) > 0)
		{
			send(sockfd,buff,strlen(buff)+1,0);
			memset(buff,0,512);
			
			recv(sockfd,buff,command,0);
			printf("%s",buff);
			memset(buff,0,512);
			recv(sockfd,buff,512,0);
		}*/

	}
	close(sockfd);
}
void client_cmd(int sockfd)
{
	int count = 1;

	while(1)
	{
		/*count = recv(sockfd,buff,512,0);
		printf("pipe:%s\n",buff);
		send(sockfd,buff,strlen(buff),0);

		int num = atoi(buff);
		memset(buff,0,512);
		recv(sockfd,buff,num,0);
		printf("%s\n",buff);
		memset(buff,0,512);
		*/
		
		char buff[BUFF_SIZE] = {0};
		count = recv(sockfd,buff,BUFF_SIZE-1,0);
		if(count == 0)  return ;
		send(sockfd,buff,strlen(buff),0);
	
		int num = atoi(buff);
		memset(buff,0,BUFF_SIZE);
		count = recv(sockfd,buff,num,0);
		printf("%s",buff);
		if(num <=  BUFF_SIZE || count == 0) break;
	}
	printf("\n");

	return ;
}


void work_handle(int sockfd,char cli_req[],char* client_path)
{ 
	char backup[BUFF_SIZE] = {0};
	char back_cli_req[BUFF_SIZE] = {0};
	strncpy(back_cli_req,cli_req,strlen(cli_req));	
	
	char *argv[ARGV_COUNT] = {0};
	argv[0] = strtok(cli_req," ");
	int i = 1;
	for( ; i<ARGV_COUNT; i++)
	{
		argv[i] = strtok(NULL," ");
	}
	//argv[2] = strtok(NULL," ");
	//argv[3] = strtok_r(NULL," ");
	//argv[2] = strtok_r(NULL,"\n",&p);
	//printf("%s\n",argv[0]);
	//
	//传输文件
	if(strcmp(argv[0],"update") == 0)
	{
		/*DIR *dp;
		char thisdir[50] = {0};
		sprinf(thisdir,"./%s",argv[1]);
		dp = opendir(thisdir);
		struct dirent* dir;
		struct stat st;
		if(dp == NULL)
		{
			file_sent(sockfd,backup,argv[1]);
		}
		else
		{
			char dirct[50] = {0};
			sprintf(dirct,"dirctory %s",argv[1]);
			send(sockfd,dirct,strlen(dirct),0);
			while(dir = readdir(dp) != NULL)
			{
				lstat(dir->d_name,&st);
				if(S_ISDIR(st)) continue;
				char newdir[50] = {0};
				char *temp = strok(dir->d_name,"/");
				sprintf(newdir,"update %s",temp);
				file_sent(sockfd,newdir,temp);
			}
		}
		*/
		char file_path[PATH_SIZE] = {0};
		strcpy(file_path,argv[1]);
		char path_cur[PATH_SIZE] = {0};
		strcat(path_cur,client_path);
		if( argv[1] != NULL )
			{
				if(strcmp(client_path,"./") != 0)
				strcat(path_cur,"/");
				strcat(path_cur,argv[1]);
			}
		argv[1] = path_cur;
		i = 0;
		for( ; i<ARGV_COUNT; i++)
		{
			if(argv[i] == NULL) break;
			strcat(backup,argv[i]);
			strcat(backup," ");
		}
				
		file_handle(sockfd,backup,file_path);
	}
	else if(strcmp(argv[0],"download") == 0)
	{
		printf("download\n");
	}
	else if(strcmp(argv[0],"help") == 0)//查看帮助信息
	{
		printf("========================\n");
		printf("\t**1.update filename\n");
		printf("\t**2.download filename\n");
		printf("\t**3.delete filename \n");
		printf("\t**4.mkdir dirctorname\n");
		printf("\t**5.ls\n");
		printf("\t**6.help\n");
		printf("=========================\n");
	}
	else//调用系统命令
	{
		strcpy(backup,back_cli_req);
		//strcat(backup,"/");
		if(strcmp(argv[0],"pwd") != 0)
		{
		strcat(backup," ");
		if(strcmp(client_path,"./") != 0)
		strcat(backup,client_path);
		}
		send(sockfd,backup,strlen(backup),0);//系统命令
		client_cmd(sockfd);
	}
}

void flashmd5(char* path_file,char* md5_buff)
{
	/*
	struct dirent **namelist;
	int n = scandir(".",&namelist,0,alphasort);
	int i = 0;
	for( ; i<n;i++)
	{
		printf("%s  ",namelist[i+1]->d_name);
		free(namelist[i+1]);
	}
	free(namelist);*/

	Compute_file_md5(path_file,md5_buff);
	//printf("%s\n",buff);
}
void file_handle(int sockfd,char cli_req[],char* path_file)
{
	char path[PATH_SIZE] = {0};
	struct stat st;
	sprintf(path,"%s",path_file);
	lstat(path,&st);
	if(S_ISDIR(st.st_mode))//判断上传文件是否为目录，否则直接上传普通文件
	{
		DIR* dp;
		dp = opendir(path);

		strcat(cli_req," directory");
		send(sockfd,cli_req,strlen(cli_req),0);
		char rev_ok[BUFF_SIZE] = {0};
		recv(sockfd,rev_ok,BUFF_SIZE-1,0);
		if(strcmp(rev_ok,"ok") != 0)
		{
			return ;
		}

		
		struct dirent* dir;
		while( (dir = readdir(dp)) != NULL )//对当前目录中的文件普通文件进行循环处理，对目录文件进行递归处理
		{
			if(strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0) continue;
			char path_cur[PATH_SIZE] = {0};
			strcat(path_cur,path);
			strcat(path_cur,"/");
			strcat(path_cur,dir->d_name);
			lstat(path_cur,&st);
			if( S_ISDIR(st.st_mode) )
			{
				char new_cli_req[BUFF_SIZE] = "update ";
				char new_path_file[PATH_SIZE] = {0};
				//char s[50] = {0};
				//strcat(back0,buff);
				//sprintf(back0,"update %s",dir->d_name);
				//strcat(back0,"/");
				strcat(new_cli_req,path_cur);
				//char *s = strtok(path,"./");
				strcat(new_path_file,path_cur);
				file_handle(sockfd,new_cli_req,new_path_file);
			}
			else
			{
				char buff[BUFF_SIZE] = {0};
				sprintf(buff,"update %s",path_cur);
				file_sent(sockfd,buff,path_cur);
			}
		}

	}
	else
	{
		file_sent(sockfd,cli_req,path_file);
	}
}


void file_sent(int sockfd,char cli_req[],char* path_file)
{
	//lstat(argv[1],);
	int fd = open(path_file,O_RDONLY);
	if(fd == -1)
	{
		printf("filename is error\n");
		return ;
	}
	//文件大小
	int size = lseek(fd,0,SEEK_END);
	char filesize[FIEL_SIZE] = {0};
	sprintf(filesize," %d",size);
	strcat(cli_req,filesize);
	//lseek(fd,0,SEEK_SET);

	char upfile_md5[FIEL_MD5_SIZE] = {0};
	flashmd5(path_file,upfile_md5);//MD5码
	//argv[2] = filemd5;		
	//char md5[33] = {0};
	//sprintf(md5," %s",filemd5);
	strcat(cli_req," ");
	strcat(cli_req,upfile_md5);
	send(sockfd,cli_req,strlen(cli_req),0);//上传文件请求

	char buff[BUFF_SIZE] = {0};
	int count = recv(sockfd,buff,BUFF_SIZE-1,0);
	if(count <= 0)
	{
		close(fd);
		return ;
	}
	//printf("%s\n",buff);//okok	
	if(strcmp(buff,"final") == 0)
	{
		printf("一个文件已秒传\n");
		return ;
	}
	//设置断点续传
	int tem = 0;
	if(strncmp(buff,"okok",4) == 0)
	{
		char *s = strtok(buff,"#");
		s = strtok(NULL,"#");
		tem = atoi(s);	
		lseek(fd,tem,SEEK_SET);
	}
	
	//传输文件	
	memset(buff,0,BUFF_SIZE);
	count  = read(fd,buff,BUFF_SIZE-1);
	int ccur = size - tem;
	while(count > 0)
	{
		send(sockfd,buff,count,0);
		count = read(fd,buff,BUFF_SIZE);
		//if(count < 512) printf("%s\n\n-----------------\n",buff);
		ccur -= count;
		float per =  (size-ccur)*100.0/size;
		printf("update %0.2f%\r",per);
		fflush(stdout);
	}
	printf("update 100.00%\n");
	//传输结束，关闭文件
	memset(buff,0,BUFF_SIZE);
	recv(sockfd,buff,BUFF_SIZE-1,0);
	if(strcmp(buff,"final") == 0)
	{
		printf("文件上传成功\n");
	}
	close(fd);
}
