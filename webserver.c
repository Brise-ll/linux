
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

	//不断接收客户端连接，并为每个客户创建线程
	while (1) {

		//wait for client connection
		printf("waiting for incoming client...\n");

		struct sockaddr_in clientSocketAddr;
		int server_addr_len = sizeof(struct sockaddr_in);

		//接收客户端连接，返回一个socket. 直到成功接收到一个连接为止
		int clientSocketFd = accept(serverSocketFd,
				(struct sockaddr *) &clientSocketAddr, &server_addr_len);

		if (clientSocketFd < 0) {
			//接收失败
			printf("Error: accept failed.\n");
			continue;
		}

		//创建线程与client交互
		pthread_t pid;
		pthread_create(&pid, NULL, workerThread, &clientSocketFd);
	}
}

void workerThread(void* para){

}

int main(){

	startServer();
	return 0;
}


