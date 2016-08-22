#include "ftp_client.h"
/*******************************************************
*功能：
*	get：取远方的一个文件
*	put：传给远方一个文件
*	pwd：显示远主当前目录
*	dir：列出远方当前目录
*	cd ：改变远方当前目录
*	？ ：显示你提供的命令
*	ls ：显示本地目录
*	quit：退出返回
*
*********************************************************/


/******************上传文件到服务器*******************/
int put_file(char *file)
{
	int ffd;			//file 文件描述符
	char buf[256];		//缓存区
	int total_c;		//文件总大小
	int sent_c = 0;		//已经发送的字节数
	int snd_c;			//发送字节数
	int ret;			//处理返回值
	int ack;			//信号确认

	/*打开要发送的文件*/
	ffd = open(file, O_RDONLY);
	if(ffd == -1) 
	{
		printf("open %s failled\n", file);
		goto SNDERR;
	}

	/*发送READY_T的信号*/
	ack = READY_T;
	ret = send(datafd, &ack, 4, 0);
	/*等待对方发送确认的应答*/
	ret = recv(datafd, &ack, 4, MSG_WAITALL);
	if(ack == FAILL_T)
		goto SNDERR;
	/*发送文件总大小*/
	total_c = lseek(ffd, 0, SEEK_END);
	lseek(ffd, 0, SEEK_SET);
	printf("have %.2f KB to send\n", (float)total_c/1024);
	ret = send(datafd, &total_c, sizeof(total_c), 0);
	
	

	/*开始发送文件*/
	printf("sending %s .......\n", file);
	snd_c = 255;
	while(1)
	{
		bzero(buf, sizeof(buf));
		//从文件中读取snd_c字节到缓存
		ret = read(ffd, &buf, snd_c);
		//发送snd_c个字节
		ret = send(datafd, buf, snd_c, 0);
		//总字节数-已发送字节数
		total_c -= ret;
		sent_c  += ret;
		if(total_c < snd_c)
			snd_c = total_c;
		if(total_c <= 0)
			break;
	}
	/*发送发送完成信号*/
	ack=OK_T;
	ret = send(datafd, &ack, 4, 0);
	close(ffd);
	/*等待对方接收完成的应答*/
	ret = recv(datafd, &ack, 4, 0);
	printf("send %.2f KB\n", (float)sent_c/1024);
	printf("send %s success\n", file);
	return 0;

//错误处理
SNDERR:
	//打印错误信息
	perror("send err");
	//发送接收错误信号
	ack = FAILL_T;
	ret = send(datafd, &ack, 4, 0);
	//关闭文件
	close(ffd);
	return -1;
	
}
/******************下载服务器传过来的文件******************/
int dowload(char *file)
{
	int file_c;		//文件总大小
	int get_c;		//接收到的字节数
	int got_c=0;	//已经接收的字节数
	char buf[512];	//接收缓存
	int ffd;		//文件描述符
	int ret;		//处理返回值
	int ack;		//信号确认

	/*等待对方发送READY_T的信号*/
	ret = recv(datafd, &ack, 4, MSG_WAITALL);
	if(ack != READY_T)
	{
		printf("file donot exist\n");
		return -1;
	}

	/*打开或创建文件*/
	ffd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(ffd == -1) 
	{
		printf("open file failled\n");
		goto RECVERR;
	}
	/*发送确认信号*/
	ack = OK_T;
	ret = send(datafd, &ack, 4, 0);
	
	/*等待对方发送文件总大小*/
	ret = recv(datafd, &file_c, 4, MSG_WAITALL);
	printf("have %.2f KB to download\n", (float)file_c/1024);
	printf("downloading %s.......\n", file);

	/*开始接收*/
	get_c = 512;
	while(1)
	{
		//清空缓存
		bzero(buf, sizeof(buf));

		if(file_c < get_c)
				get_c = file_c;
		//接收数据
		ret = recv(datafd, buf, get_c, MSG_WAITALL);
		if(ret > 0)
		{
			//把接收到的数据写入文件
			write(ffd, buf, ret);
			//总大小-接收到的字节数
			file_c -= ret;
			//更新收到的总数
			got_c  += ret;
		}
		else if(ret == 0)
		{
			printf("download %s success\n", file);
			break;
		}
		else if(ret == -1)
		{
			printf("internet err!\n");
			break;
		}
	}
	/*等待发送完毕确认信号*/
	ret = recv(datafd, &ack, 4, 0);
	ack = OK_T;
	/*发送以接收完毕确认信号*/
	ret = send(datafd, &ack, 4, 0);
	printf("download %.2f KB\n", (float)got_c/1024);
	close(ffd);
	return 0;

//错误处理
RECVERR:
	//打印错误信息
	perror("download err");
	//发送接收错误信号
	ack = FAILL_T;
	ret = send(datafd, &ack, 4, 0);
	//关闭文件
	close(ffd);
	return -1;
	
}
/******************显示帮助信息*******************/
void show_help()
{
	printf("================帮助命令==================\n");
	printf("   ==get <file> 下载文件==\n   ==put <file> 上传文件== \
		  \n   ==dir 列出远方当前目录==\n   ==pwd 显示远主当前目录==\
		  \n   ==cd <dir> 列出远方当前目录==\n   ==? 显示命令帮助==\
		  \n   ==ls 列出本地目录==\n   ==quit 退出==\n");
	printf("==========================================\n");
}

/******************解释命令*******************/
int get_cmd(char *msg)
{
	int cmd = -1;
	if(strncmp(msg, "get", 3) == 0 || strncmp(msg, "GET", 3) == 0) cmd = GET;
	else if(strncmp(msg, "put", 3) == 0 || strncmp(msg, "PUT", 3) == 0) cmd = PUT;
	else if(strncmp(msg, "pwd", 3) == 0 || strncmp(msg, "PWD", 3) == 0) cmd = PWD;
	else if(strncmp(msg, "dir", 3) == 0 || strncmp(msg, "DIR", 3) == 0) cmd = GDIR;
	else if(strncmp(msg, "cd",  2) == 0 || strncmp(msg, "cd", 2) == 0) cmd = CD;
	else if(strncmp(msg, "?",   1) == 0 )  cmd = HELP;
	else if(strncmp(msg, "ls",  2) == 0 || strncmp(msg, "LS", 2) == 0) cmd = LS;
	else if(strncmp(msg, "quit",4) == 0 || strncmp(msg, "QUIT", 4) == 0) cmd = QUIT;
	else if(strcmp(msg, "q") == 0 || strcmp(msg, "Q") == 0) cmd = QUIT;
	else printf("command not found (? for help)\n");
	return cmd;
}


/*******************文件传输的线程********************/
void *transfer_routine(void *arg)
{
	//判断发送还是接收
	switch(flag)
	{
		case PUT:	//发送文件
			put_file(file_name);
			break;
		case GET:	//接收文件
			dowload(file_name);
			break;
	}
	//输入提示
	write(0,"myftp>", sizeof("myftp>"));
}

/*******************接收并显示服务当前目录的内容********************/
void get_dir()
{
	int r = 1;			//初始接收一个字节
	int get_c = 0;		//已经接收的字节数
	char r_buf[256];	//接收到完整的文件名缓存
	char temp_buf[256];	//接收缓存区
	//清空缓存
	bzero(r_buf, sizeof(r_buf));
	while(1)
	{	//清空缓存
		bzero(temp_buf, sizeof(temp_buf));
		//接收,由于tcp是字节流，因此先接收文件名长度，再接收完整的文件名
		r = recv(fd, temp_buf, r, MSG_WAITALL);
		//更新已接收的字节数
		get_c += r;
		//更新已接收的字节数
		strcat(r_buf, temp_buf);
		//要接收的字节数为0时，则结束
		if(r_buf[0] == 0) break;
		//判断是否已经完整接收到一个文件名
		if((char)get_c != r_buf[0])
		{
			r = (int)r_buf[0]-get_c;
			continue;
		}
		r_buf[get_c] = '\0';
		r_buf[0] = '*';
		//打印
		printf("%s\n", r_buf);
		//初始化
		bzero(r_buf, sizeof(r_buf));
		get_c = 0;
		r = 1;
		
	}
	printf("===================================\n");
	//输入提示
	write(0,"myftp>", sizeof("myftp>"));
}

/******************接收服务器信息线程*******************/
void *rec_msg(void *arg)
{
	char r_buf[256];	//接收缓存
	int r;				//返回值处理
	pthread_t tid;		//线程id
	int *ack = (int *)r_buf;

	while(1)
	{
		//清空缓存
		bzero(r_buf, sizeof(r_buf));
		//接收
		r = recv(fd, r_buf, 255, 0);
		if(r == -1)
		{
			perror("recv fialed");
			continue;
		} 
		r_buf[r] = '\0';
		
		//判断工作方式
		switch(flag)
		{
			case GET:		//下载文件
				if(*ack == READY_T)
				{
					printf("ready to dowload\n");
					pthread_create(&tid, NULL, &transfer_routine, NULL);
				}
				is_data = 0;
				break;
			case PUT:		//上传文件
				if(*ack == READY_T)
				{
					printf("ready to sent\n");
					pthread_create(&tid, NULL, &transfer_routine, NULL);
				}
				is_data = 0;
				break;
			case GDIR:		//获取目录内容
					printf("\n");
					get_dir();
				break;
			case PWD:		//当前路径
				printf("%s\n", r_buf);
				//输入提示
				write(0,"myftp>", sizeof("myftp>"));
				break;
			case CD:		//改变当前路径
				printf("%s\n", r_buf);
				//输入提示
				write(0,"myftp>", sizeof("myftp>"));
				break;
		}
	}
}


/***********等待数据TCP连接***************/
void *data_routine(void *arg)
{
	socklen_t len;
	len = sizeof(data_adr);
	while(1)
	{
		//等待服务器连接
		datafd = accept(dfd, (struct sockaddr *)&(data_adr), &len);
		is_data = 1;
	}
}

/******************列出当前目录的文件及目录*******************/
int ls_dir()
{
	DIR *dp = opendir("./");
	struct dirent *ep;
	int r;
	char type[104] = "---";
	
	if(dp == NULL)
	{
		perror("opendir() failed");
		return -1;
	}
	errno = 0;
	while(1)
	{
		ep = readdir(dp);
		if(ep == NULL && errno == 0)
		{
			break;
		}
		else if(ep == NULL)
		{
			perror("readdir() failed");
			return -1;
		}      
		if(ep->d_name[0] == '.')
		{
			continue;
		}
		bzero(type, sizeof(type));
		type[0] = '*';
		if(ep->d_type == DT_DIR)
			type[1] = 'd';
		else
			type[1] = '-';
		type[2] = '-';
		type[3] = '-';
		strncat(type, ep->d_name, strlen(ep->d_name)+5);
		printf("%s\n", type);
		if(r == -1) perror("send err");
	}
	closedir(dp);
	return 0;
}

/****释放资源，退出******/
void quit()
{
	shutdown(datafd, SHUT_RDWR);
	shutdown(fd, SHUT_RDWR);
	close(datafd);
	close(fd);
	exit(0);
}

/**** 控制台命令输入 ******/
void cmd_console()
{
	int r;				//处理返回值
	char s_buf[256];	//缓存区
	char *sent_buf = s_buf;
	int local_cmd = 0;
	
	while(1)
	{	//清空缓存
		bzero(s_buf, sizeof(s_buf));
		//输入提示
		write(0,"myftp>", sizeof("myftp>"));
		//从终端输入命令
		gets(s_buf);
		if(s_buf[0] == '\0') continue;
		//处理输入的命令,判断命令类型
		switch(get_cmd(s_buf))
		{
			case GET:	//下载文件
				if(strlen(s_buf) < 5)
				{
					printf("pls input get <file>\n");
					break;
				}
				//跳过get三个字符
				sent_buf += 3;	
				//设置传输文件名
				strcpy(file_name, sent_buf+1);
				printf("%s\n",file_name);
				//设置工作模式为下载文件
				flag = GET;
				//设置第一个字符为命令编号
				sent_buf[0] = (char) flag;	
				break;
			case PUT:	//上传文件
				if(strlen(s_buf) < 5)
				{
					printf("pls input put <file>\n");
					break;
				}
				//跳过put三个字符
				sent_buf += 3;
				//设置传输文件名
				strcpy(file_name, sent_buf+1);
				//设置工作模式为上传文件
				flag = PUT;	
				sent_buf[0] = (char) flag;				
				break;
			case GDIR:	//获取目录内容
				flag = GDIR;
				printf("=========服务器当前目录文件=========\n");
				sent_buf[0] = (char) flag;
				sent_buf[1]	= '\0';			
				break;
			case PWD:	//获取当前路径
				flag = PWD;
				//发送命令给服务器
				sent_buf[0] = (char) flag;
				sent_buf[1]	= '\0';
				//r = send(fd, s_buf, strlen(s_buf), 0);
				break;
			case CD:	//改变当前路径
				flag = CD;
				sent_buf += 2;
				sent_buf[0] = (char) flag;
				break;
			case HELP:	//显示命令帮助信息
				show_help();
				local_cmd = 1;
				break;
			case LS:
				printf("=========本地当前目录文件=========\n");
				ls_dir();
				printf("=================================\n");
				local_cmd = 1;
				break;
			case QUIT:	//退出
				quit();
				break;
			default:
				break;
		}	
		if(!local_cmd)
		{
			//发送命令给服务器
			r = send(fd, sent_buf, strlen(sent_buf), 0);
			//指回缓存区的开头
			sent_buf = s_buf;
		}
		local_cmd = 0;
	}
}

/******************主函数*******************/
int main(int argc, char const *argv[])
{

	/***默认服务器IP地址*****/
	char sip[20] = "192.168.1.210";
	
	int r;				//处理返回值
	pthread_t tid;		//线程id
	

	int ack;
	int data_port = 31629;	//数据传输端口

	if(argc < 2)
	{
		printf("pls input: %s [ip]\n", argv[0]);
		exit(0);
	}
	flag = -1;			//工作模式初始化为无
	is_data = 0;
	port = 2121;		//默认端口为2121
	if(argc > 2)
	{
		//自己本地定义端口
		int n = strlen(argv[2]);
		int i;
		data_port = 0;
		for( i=0; i<n; i++)
		{
			if('0'<= argv[2][i] && argv[2][i] <= '9')
			{
				data_port += (argv[2][i] - '0')*(i+1);
			}
		}
	}
	
	strncpy(sip, argv[1], strlen(argv[1])+1);
	
	
	printf("server_ip = %s : %d\n", sip, port);

	//1.socket	创建两个套接字，一个用于连接服务器，一个用于文件传输
	fd = socket(AF_INET, SOCK_STREAM, 0);
	dfd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1 || datafd == -1)	perror("socket err:"), exit(-1);

	//2.connect 
	sadr.sin_family = AF_INET;
	sadr.sin_port = htons(port);
	inet_aton(sip, &sadr.sin_addr);

	data_adr.sin_family = AF_INET;
	data_adr.sin_port = htons(data_port);
	data_adr.sin_addr.s_addr = htonl(INADDR_ANY); 
	
	//绑定地址
	r = bind(dfd, (struct sockaddr *)&data_adr, sizeof(data_adr));
	if(r == -1) perror("2.bind: "), exit(-1);
	r = listen(dfd, 1);
	if(r == -1) perror("3.listen: "), exit(-1);

	//连接服务器
	r = connect(fd, (struct sockaddr *)&sadr, sizeof(sadr));
	if(r == -1) perror("连接服务器错误"), exit(-1);
	
	/*等待对方发送访问权限*/
	r = recv(fd, &ack, 4, MSG_WAITALL);
	if(ack == FAILL_T)
	{
		printf("服务器拒绝连接,请稍后再试\n");
		ack=OK_T;
		send(fd, &ack, sizeof(ack), 0);
		sleep(2);
		return 0;
	}
	else
	{
		printf("连接服务器成功\n");
	}
	
	//创建与服务命令交互线程
	r = pthread_create(&tid, NULL, &rec_msg, NULL);
	//显示命令帮助信息
	show_help();
	//创建等待数据传输TCP连接线程
	r = pthread_create(&tid, NULL, &data_routine, NULL);
	
	//发送端口
	r = send(fd, &data_port, 4, MSG_WAITALL);

	cmd_console();
	
	return 0;
}
