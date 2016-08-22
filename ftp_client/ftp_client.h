#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <dirent.h>
#include <errno.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#define  SOCKETFD  int

/******************各命令标识*******************/
enum{GET = 1, PUT = 2, PWD = 3, GDIR = 4, CD = 5, HELP = 6, LS = 7, QUIT = 8};
enum{READY_T=200,OK_T=202, FAILL_T=204};

/*********文件传输的头**********/
struct head
{
	int   f_size;
	int   f_name_len;
};
/**服务器套接字**/
SOCKETFD fd; 
/**数据传输套接字**/
SOCKETFD datafd;
SOCKETFD dfd;
/***服务器地址*****/			
struct sockaddr_in sadr;
/***数据传输地址*****/	
struct sockaddr_in data_adr;
/***记录当前工作状态******/
int flag;
//文件传输的标记
int is_data;		
char id;
//文件的名字
char file_name[256];
//服务器端口
int port;

/******************上传文件到服务器*******************/
int put_file(char *file);
/******************下载服务器传过来的文件******************/
int dowload(char *file);
/******************显示帮助信息*******************/
void show_help();
/******************解释命令*******************/
int get_cmd(char *msg);
/*******************文件传输的线程********************/
void *transfer_routine(void *arg);
/******************接收服务器信息线程*******************/
void *rec_msg(void *arg);
/***********等待数据TCP连接***************/
void *data_routine(void *arg);
/****释放资源，退出******/
void quit();
/**** 控制台命令输入 ******/
void cmd_console();
/******************主函数*******************/
int main(int argc, char const *argv[]);

#endif