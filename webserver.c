
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

void startServer(){
	//创建服务器socket地址
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	//创建服务器socket
	int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);

	//将socket与ip和port绑定， 如果port被其它占用，则会失败
	if (bind(serverSocketFd, (struct sockaddr *) &server_addr,
			sizeof(struct sockaddr_in)) == -1) {
		printf("bind server socket error\n");
		return;
	}

	//开始监听，10为未连接队列最大数限制，即最多可有10个client同时连接，10可以修改，最大为512
	if (listen(serverSocketFd, 10) == -1) {
		printf("listen server socket error\n");
		return;
	}
}

void workerThread(void* para){

}

int main(){

	startServer();
	return 0;
}


