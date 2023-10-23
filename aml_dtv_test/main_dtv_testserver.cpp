#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include "cmd_process.h"
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif
#define ERR_EXIT(m) \
	do { \
		perror(m); \
		exit(EXIT_FAILURE); \
	} while (0)
#define PORT 9527
#define MAX_CONN 5
#define BUFFER_SIZE 1024
pthread_mutex_t mutex;  // 定义互斥锁，全局变量
pthread_mutex_t mutex_play = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_play = PTHREAD_COND_INITIALIZER;
	
void *conn_thread(void * fd)
{
	pthread_detach(pthread_self());//线程分离
	int new_conn = *((int *)fd);
	// 解锁，pthread_mutex_lock()唤醒，不阻塞
    pthread_mutex_unlock(&mutex); 
	char buffer[BUFFER_SIZE] = "";
	while(1)
	{
		int count = 0;
		memset(buffer, 0, BUFFER_SIZE);
		count = read(new_conn, buffer,  BUFFER_SIZE);
		if(count < 0)
		{
			fprintf(stderr, "read error\n");
			continue;
		} else  if (count ==  0)
        {
            fprintf(stderr, "client close\n");
			break;
        }
		fprintf(stderr, "get msg：%s \n", buffer);
		ParseCmd(new_conn, buffer);
		fprintf(stderr, " read End \n");
	}
	close(new_conn);
	fprintf(stderr, "conn_thread end\n");
	return NULL;
}

void signHandler(int iSignNo)
{
    UNUSED(iSignNo);
    fprintf(stderr, "signHandler:%d , getpid() = %d \n", iSignNo, getpid());
	pthread_cond_signal(&cond_play);//Ctrl退出,如果在播放的话,先把播放的资源给释放了
    signal(SIGINT, SIG_DFL);
	usleep(1000 * 500);//最好是播放那边结束再返回,但怎么说,没播放的时候怎么判断
	fprintf(stderr, "raise(SIGINT) \n");
	raise(SIGINT);
}

int main(int argc, char **argv) {
	fprintf(stderr, "aml_dtv_testserver START!!!\n");
	signal(SIGINT, signHandler);
	int listen_fd;//初始化套接字
	if ((listen_fd = socket(AF_INET, SOCK_STREAM,  0)) <  0)
		ERR_EXIT( "socket error");
	struct sockaddr_in server_add;
	memset(&server_add,  0,  sizeof(server_add));
	server_add.sin_family = AF_INET;//Internet地址族=AF_INET(IPv4协议)
	server_add.sin_port = htons(PORT);//主机字节序转换为网络字节序
	(server_add.sin_addr).s_addr = htonl(INADDR_ANY);
	//套接字与端口绑定
	if(bind(listen_fd, ( struct sockaddr *)&server_add,  sizeof(server_add)) <  0)
        ERR_EXIT( "bind error");
	//开启监听,最大连接数
	if (listen(listen_fd, MAX_CONN) <  0)
        ERR_EXIT( "listen error");
	int conn;
	pid_t pid;
	pthread_mutex_init(&mutex, NULL); // 初始化互斥锁，互斥锁默认是打开的
	while (1)
    {
		struct sockaddr_in client_addr;
		socklen_t size = sizeof(client_addr);
		// 上锁，在没有解锁之前，pthread_mutex_lock()会阻塞
        pthread_mutex_lock(&mutex);
        conn = accept(listen_fd,  (struct sockaddr *)&client_addr,  &size);
		if (conn == - 1)
        {
			fprintf(stderr, "next accept\n");
			continue;
        }
		fprintf(stderr, "accept client ip : %s:%d\n",inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
		int pthread_id;
		int ret = pthread_create((pthread_t *)&pthread_id, NULL, conn_thread, (void *)&conn);
		if(ret == -1){
			fprintf(stderr, "pthread_create error\n");
			close(conn);
			continue;
		}else{
			fprintf(stderr, "pthread_create success next accept\n");
		}
    }
	close(listen_fd);
	fprintf(stderr, "aml_dtv_testserver EXIT!!!\n");
	return 0;
}