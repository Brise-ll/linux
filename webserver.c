#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define BUFSIZE 256
char buff[BUFSIZE];

void startServer() {
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

//利用socket发送数据
int sendData(int socketFd, const void* buf, size_t length) {
	int status = send(socketFd, buf, length, 0); //发送数据，返回实际发送的字节数

	//检查发送结果是否正常
	if (status <= 0) {
		return 0;
	}
	return 1;
}

//利用socket接收数据
int recvData(int socketFd, void* buf, size_t length) {
	int receivedBytes = 0;

	//持续接收，直到收到length个字节为止
	while (receivedBytes < length) {
		void* currentBuf = buf + receivedBytes; //接收到的数据要存放的起始地址
		int remainBytes = length - receivedBytes; //需要接收的剩余字节数
		int status = recv(socketFd, currentBuf, remainBytes, 0); //接收数据，返回实际接收的字节数

		//检查接收结果是否正常
		if (status <= 0) {
			return 0;
		}

		receivedBytes += status;
	}

	return 1;
}


int readLine(int clientsfd, char* line) {

	int length = 0;
	recv(clientsfd, buff, 1, 0);
	while (buff[0] != '\n') {
		if (length >= BUFSIZE) {
			return 0;
		}
		line[length++] = buff[0];
		recv(clientsfd, buff, 1, 0);
	}
	line[length] = 0;
	return 1;
}

void send_500(int clientsfd) {
	sprintf(buff, "HTTP/1.1 500 Internal Server Error\r\n");
	send(clientsfd, buff, strlen(buff), 0);
}

void workerThread(void* para) {
	int socketFd = *((int*) para);

	//read first line
	char line[BUFSIZE];
	if (!readLine(socketFd, line)) {
		send_500(socketFd);
		return;
	}

	//debug message, print header
	printf("header: %s\n", line);
}

int main() {

	startServer();
	return 0;
}

