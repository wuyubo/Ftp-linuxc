#include "ftpserver.h"


/**服务器套接字**/
static SOCKETFD serverfd;
/***服务器地址*****/
static struct sockaddr_in sadr;
/***记录客户个数*****/
static socklen_t len;
static char this_path[100];
/*********客户端的个数***************/
int clt_count;
int port;
/***默认工作路径是挂载在开发板的U盘****/
QString current_path = "file";
int is_fchge = 0;
char clientip[3][20]={0};

extern int t_count;
extern pthread_mutex_t t_mutex ;

void *main_routine(void *arg)
{
	int ret;
	/******1.创建socket套接字******/
	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if(serverfd == -1)	perror("1.socket:"), exit(-1);
	printf("---------socket server success\n");
	/******2.bind 绑定******/
	sadr.sin_family = AF_INET;
	//端口设为2121
	sadr.sin_port = htons(2121);
	//本地地址
	sadr.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(serverfd, (struct sockaddr *)&sadr, sizeof(sadr));
	if(ret == -1) perror("2.bind: "), exit(-1);
	printf("---------blind addr success\n");
	/******3.listen 监听******/
	ret = listen(serverfd, 3);
	if(ret == -1) perror("3.listen: "), exit(-1);
	printf("---------listen.......... \n");
	printf("---------服务器启动成功！ \n");
	
	//初始化客户端数为0
	clt_count = 0;
	len = sizeof(sadr);
	//设默认工作目录为 ./file
	ret = cd_dir("./file");
	/***4.accept********/
	accept_client();
	close(serverfd);
}
/****等待客户端连接******/
void accept_client()
{
	int ret;
	pthread_t tid;
	int ack;
	while(1)
	{
		client *clt = new client;
		//等待客户连接
		clt->fd = accept(serverfd, (struct sockaddr *)&(clt->adr), &len);
		if(clt->fd == -1)
		{
			perror("3.listen: ");
			continue;
		}			
		printf(" 有客户连接 : %d, IP: %s : %u\n", clt->fd, inet_ntoa(clt->adr.sin_addr), ntohs(clt->adr.sin_port));
		//为了限制访问量，超过3个客户连接都拒绝
        if(clt_count >= 3)
		{
			//发拒绝访问信号
			ack=FAILL_T;
			send(clt->fd, &ack, sizeof(ack), 0);
			printf("已经连接3个客户\n");
			//等待回应后断开连接
			recv(clt->fd, &ack, 4, MSG_WAITALL);
			shutdown(clt->fd, SHUT_RDWR);
			close(clt->fd);
			free(clt);
			clt_count--;
			continue;
		}
		clt_count++;
		//记录新客户的ip
		strcpy(clientip[clt_count-1], inet_ntoa(clt->adr.sin_addr));
		//设置新客户的序号
        clt->id = clt_count-1;
		//发允许访问信号
		ack=OK_T;
		send(clt->fd, &ack, sizeof(ack), 0);
		//创建一个线程来处理新连接客户的命令
        ret = pthread_create(&tid, NULL, &client_routine, (void*) clt); 
	}
}

/******************改变当前工作目录*******************/
int cd_dir(char *dir)
{
	printf("chdir %s\n", dir);
	//改变目录
	if(chdir(dir) == -1)
		perror("chdir err");
    else
        printf("chdir %s success\n", dir);
	//获得当前工作目录给this_path
    getcwd(this_path, sizeof(this_path));
	//this_path传给current以更新界面
    current_path = QString::fromUtf8(this_path);
	return 0;
}
/*******在客户要改变镜像目录时，检查目是否存在********/
int try_cddir(char *dir, int len)
{
	//考虑到 ../为上一级目录， ./为当前目录, 相对路径和绝对路径
	if(dir[len-2] == '.' || dir[len-1] == '.')//../ 或./的情况需要进行剪裁
	{
		int i = len-1;
		if(dir[len-3] == '.')//../
		{	
			dir[i--] = '\0';
			for(; i>0; i--)
			{
				if(dir[i] == '/')
				{
					dir[i] = '\0';
					break;
				}					
				dir[i] = '\0'; 
			}
			for(; i>0; i--)
			{
				if(dir[i] == '/')
				{
					dir[i] = '\0';
					break;
				}					
				dir[i] = '\0'; 
			}
		}
		else	//./
		{
			dir[i--] = '\0';
			for(; i>0; i--)
			{
				if(dir[i] == '/')
				{
					dir[i] = '\0';
					break;
				}					
				dir[i] = '\0'; 
			}
		}
		printf("chdir %s success\n", dir);
		return 0;
	}
	
	//不是以上两种情
	DIR *dp = opendir(dir);
	if(dp == NULL)
	{
		perror("chdir err");
		closedir(dp);
		return -1;
	}
	closedir(dp);
	printf("chdir %s success\n", dir);	
	return 0;
}

/******************列出当前目录的文件及目录*******************/
int get_dir(client *clt)
{
	//打开目录
	DIR *dp = opendir(clt->dir);
	struct dirent *ep;
	int r;
	int ack;
	char type[104] = "---";
	
	if(dp == NULL)
	{
		perror("opendir() failed");
		return -1;
	}
	//设置错误码为0
	errno = 0;
	ack = READY_T;
	//发送确认信号
	r = send(clt->fd, &ack, sizeof(ack), 0);
	while(1)
	{
		//读取目录的内容
		ep = readdir(dp);
		//如果为空且错误码为0,则结束
		if(ep == NULL && errno == 0)
		{
			printf("dir end\n");
			//发送结束标志，数量为0
			char end=0;
			r = send(clt->fd, &end, 1, 0);
			return 0;
		}
		//错误码为0
		else if(ep == NULL)
		{
			perror("readdir() failed");
			return -1;
		}      
		//不考虑隐藏文件 包括 .（当前目录） 和 ..（上一级目录）
		if(ep->d_name[0] == '.')
		{
			continue;
		}
		
		printf("%s\n", ep->d_name);
		bzero(type, sizeof(type));
		type[0] = '*';
		//内容是目录时做标记
		if(ep->d_type == DT_DIR)
			type[1] = 'd';
		else
			type[1] = '-';
		type[2] = '-';
		type[3] = '-';
		strncat(type, ep->d_name, strlen(ep->d_name)+5);
		//发送的第一个字节为文名的长度
		type[0] = strlen(type);
		//发送
		r = send(clt->fd, type, strlen(type), 0);
		if(r == -1) perror("send err");
		
	}
	//关闭目录
	closedir(dp);
	return 0;
}

/******************传送客户请求的文件*******************/
int put_file(int *tfd, char *file)
{
	int ffd;//file 文件描述符
	char buf[256];
	int total_c;
	int sent_c = 0;
	int snd_c;
	int ret;
	int ack;
	/*打开要发送的文件*/
	ffd = open(file, O_RDONLY);
	if(ffd == -1) 
	{
		printf("open %s failled\n", file);
		goto SNDERR;
	}

	/*发送READY_T的信号*/
	ack = READY_T;
	ret = send(*tfd, &ack, 4, 0);
	printf("open file success!\n");
	/*等待对方发送确认的应答*/
    ret = recv(*tfd, &ack, 4, MSG_WAITALL);
	printf(" %d\n", ack);
	if(ack != OK_T)
		goto SNDERR;

	/*发送文件总大小*/
	total_c = lseek(ffd, 0, SEEK_END);
	lseek(ffd, 0, SEEK_SET);
	ret = send(*tfd, &total_c, sizeof(total_c), 0);
	
	printf("have %.2f KB to send\n", (float)total_c/1024);

	/*开始发送文件*/
	printf("sending.......\n");
	snd_c = 255;
	while(1)
	{
		bzero(buf, sizeof(buf));
		ret = read(ffd, &buf, snd_c);
		send(*tfd, buf, snd_c, 0);
		total_c -= ret;
		sent_c  += ret;
		if(total_c < snd_c)
			snd_c = total_c;
		if(total_c <= 0)
			break;
	}
	/*发送发送完成信号*/
	ack=OK_T;
	ret = send(*tfd, &ack, 4, 0);
	close(ffd);
	/*等待对方接收完成的应答*/
	ret = recv(*tfd, &ack, 4, 0);
	printf("send %.2f KB\n", (float)sent_c/1024);
	printf("send %s success\n", file);
	return 0;

SNDERR:
	perror("send err");
	ack = FAILL_T;
	ret = send(*tfd, &ack, 4, 0);
	close(ffd);
	return -1;
	
}

/******************下载客户端传过来的文件******************/
int dowload(int *tfd, char *file)
{
	int file_c;
	int get_c;
	int got_c = 0;
	char buf[256];
	int ret;
	int ffd;
	int ack;

	/*等待对方发送READY_T的信号*/
	ret = recv(*tfd, &ack, 4, MSG_WAITALL);
	printf(" %d\n", ack);
	if(ack == FAILL_T)
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
	ret = send(*tfd, &ack, 4, 0);
	
	/*等待对方发送文件总大小*/
	ret = recv(*tfd, &file_c, 4, MSG_WAITALL);
	printf("have %.2f KB to download\n", (float)file_c/1024);
	printf("downloading %s.......\n", file);
	
	/*开始接收*/
	get_c = 255;
	while(1)
	{
		bzero(buf, sizeof(buf));

		if(file_c < get_c)
				get_c = file_c;

		ret = recv(*tfd, buf, get_c, MSG_WAITALL);
		if(ret > 0)
		{
			ret = write(ffd, buf, ret);
			file_c -= ret;
			got_c += ret;
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
	ret = recv(*tfd, &ack, 4, 0);
	/*发送以接收完毕确认信号*/
	ack = OK_T;
	ret = send(*tfd, &ack, 4, 0);
	printf("download %.2f KB\n", (float)got_c/1024);
	close(ffd);
	return 0;
/**错误处理**/
RECVERR:
	perror("download err");
	ack = FAILL_T;
	ret = send(*tfd, &ack, 4, 0);
	close(ffd);
	return -1;
}
/*******************文件传输的线程********************/
void *transfer_routine(void *arg)
{
	struct Transfer *ts = (struct Transfer *)arg;
	
	printf("transfer_routine -- %d, %s\n", ts->cmd, ts->file_name);
	char cur_file[200];
	//把文件名拼接到镜像目录后面
	strcpy(cur_file, ts->clt->dir);
	strcat(cur_file, "/");
	strcat(cur_file, ts->file_name);
    t_count = 1;
	switch(ts->cmd)
	{
		case GET:	//发送文件
			put_file(&ts->clt->dfd, cur_file);
			break;
		case PUT:	//接收文件
			dowload(&ts->clt->dfd, cur_file);
            is_fchge = 1;
			break;
	}
	//关闭tcp
    t_count = 0;
	shutdown(ts->clt->dfd, SHUT_RDWR);
	close(ts->clt->dfd);
	ts->clt->dfd = -1;
	printf("tranfer close\n");
}

/******************接收客户端命令线程*******************/
void *client_routine(void *arg)
{
	client *clt = (client *) arg;
	int r;
	int data_port;
	char r_buf[256];
	char dir_name[20];
	pthread_t tid;
	struct Transfer *ts = new Transfer;
	ts->clt = clt;
	int ack;
	//数据tcp套接字初始化为-1
	clt->dfd = -1;
	//设置当前路径为该客户端的路径镜像
	stpcpy(clt->dir, this_path);
	
	clt->data_adr.sin_family = AF_INET;
	clt->data_adr.sin_addr.s_addr = clt->adr.sin_addr.s_addr;
	//接收客户端数据传输tcp端口
	r = recv(clt->fd, &data_port, 4, MSG_WAITALL);
	clt->data_adr.sin_port = htons(data_port);
	
	while(1)
	{
		bzero(r_buf, sizeof(r_buf));
		/****阻塞等待客户端发命令过来****/
		r = recv(clt->fd, r_buf, 255, 0);
		
		if(r == -1|| r == 0)
		{
			printf("有客户退出\n");
			//关闭套接字，释放内存
			close(clt->fd);
			free(clt);
            bzero(clientip[clt->id], sizeof(clientip[clt->id]));
			clt_count--;
			break;
		}
		r_buf[r] = '\0';
		/****打印接收到的内容****/
		printf("%s:%d\n", inet_ntoa(clt->adr.sin_addr), r_buf[0]);
		/****判断命令类型*******/
		switch(r_buf[0])
		{
			/****客户端需要获取一个文件****/
			case GET:
				//连接数据连接TCP通道
				if(clt->dfd == -1)
				{
					clt->dfd = socket(AF_INET, SOCK_STREAM, 0);
					connect(clt->dfd, (struct sockaddr *)&clt->data_adr, sizeof(clt->data_adr));
				}
				//连接成功则进行文件传输
				if(clt->dfd > 0)
				{	
					//设置为发送模式
					ts->cmd = GET;
					//获取要发送的文件名
					stpcpy(ts->file_name , r_buf+1);
					//发送准ready信号
					ack = READY_T;
					send(clt->fd, &ack, sizeof(ack), 0);
					//创建文件传输线程
					r = pthread_create(&tid, NULL, &transfer_routine, (void*) ts);
					if(r == -1) perror("transfer err");
				}	
				break;
			/****客户端需要上传一个文件****/
			case PUT:
				//连接数据连接TCP通道
				if(clt->dfd == -1)
				{
					clt->dfd = socket(AF_INET, SOCK_STREAM, 0);
					if(connect(clt->dfd, (struct sockaddr *)&clt->data_adr, sizeof(clt->data_adr))==-1)
						break;
				}
				if(clt->dfd != -1)
				{
					//设置为下载模式
					ts->cmd = PUT;
					bzero(ts->file_name, 100);
					//获取要发送的文件名
					stpcpy(ts->file_name , r_buf+1);
					//发送准ready信号
					ack = READY_T;
					send(clt->fd, &ack, sizeof(ack), 0);
					//创建文件传输线程
					r = pthread_create(&tid, NULL, &transfer_routine, (void*) ts);
					if(r == -1) perror("transfer err");
				}
				break;
			/****客户端需要获取当前目录内容****/
			case GDIR:
				printf("=======显示目录文件========\n");
				get_dir(clt);
				printf("===========================\n");
				break;
			/****客户端需要获取当前路径****/
			case PWD:
				r = send(clt->fd, clt->dir, strlen(clt->dir), 0);
				break;
			/****客户端需要改变当前路径****/
			case CD:
				//截取后面的路径
				strcpy(dir_name, r_buf+1);
				char tmp_dir[100];
				//把要改变的相对路径和镜像拼接
				strcpy(tmp_dir, clt->dir);
				strcat(tmp_dir, "/");
				strcat(tmp_dir, dir_name);
				printf("%s\n", tmp_dir);
				//测试目录存在，则记录改变后的目录
				if(try_cddir(tmp_dir, strlen(tmp_dir)) != -1) 
				{
					bzero(clt->dir, sizeof(clt->dir));
					stpcpy(clt->dir, tmp_dir);
					//发送成功信息
					r = send(clt->fd, tmp_dir, strlen(tmp_dir), 0);
				}	
				break;

		}
	}
}

