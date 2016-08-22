#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>

#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <errno.h>

#include <sys/stat.h>
#include <QThread>

#define SOCKETFD int

/******************各命令标识*******************/
enum{GET = 1, PUT = 2, PWD = 3, GDIR = 4, CD = 5, HELP = 6, LS = 7, QUIT = 8};
enum{READY_T=200,OK_T=202, FAILL_T=204};

/*********文件传输的头**********/
struct head
{
	int   f_size;
	int   f_name_len;
};

/*********客户端的套接字及地址***************/
typedef struct  client
{
	SOCKETFD fd;
	SOCKETFD dfd;
    int id;
	struct sockaddr_in adr;
	struct sockaddr_in data_adr;
	char dir[100];

}client;

/*********命令传输结构体***************/
struct Transfer
{
	int cmd;
	struct client *clt;
	char file_name[100];
};


/****接收客户端命令线程**********/
void *client_routine(void *arg);
/****改变当前工作目录**************/
int cd_dir(char *dir);
/***在客户要改变镜像目录时，检查目是否存在*****/
int try_cddir(char *dir, int len);
/****列出当前目录的文件及目录**********/	
int get_dir(client *clt);
/****传送客户请求的文件**********/
int put_file(int *tfd, char *file);
/*****下载客户端传过来的文件*******/
int dowload(int *tfd, char *file);
/*****文件传输的线程************/
void *transfer_routine(void *arg);
/****文件传输的主线程**********/
void *main_routine(void *arg);
/****等待客户端连接******/
void accept_client();

#endif




