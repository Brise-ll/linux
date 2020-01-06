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

//工作线程函数，在此函数中负责处理客户端的请求
void* workerThread(void* para);

//启动服务器并接受连接
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

	//wait for client connection
	printf("waiting for incoming client...\n");

	//不断接收客户端连接，并为每个客户创建线程
	while (1) {

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
		pthread_create(&pid, NULL, workerThread, (void*) &clientSocketFd);
	}
}

//从socket中读取一行数据
int readLine(int clientsfd, char* line) {

	int length = 0; //读到的数据长度

	//读取一个字符
	recv(clientsfd, buff, 1, 0);

	//如果字符不是'\n'，则持续读
	while (buff[0] != '\n') {
		if (length >= BUFSIZE) {
			//超过缓存，读取失败
			return 0;
		}

		//保存读到的字符
		line[length++] = buff[0];

		//读取下一个字符
		recv(clientsfd, buff, 1, 0);
	}
	line[length] = 0;
	return 1;
}

void send_500(int clientsfd) {
	sprintf(buff, "HTTP/1.1 500 Internal Server Error\r\n");
	send(clientsfd, buff, strlen(buff), 0);
}

void send_200(int clientsfd) {
	sprintf(buff, "HTTP/1.1 200 OK\r\n");
	send(clientsfd, buff, strlen(buff), 0);
}

void send_400(int clientsfd) {
	sprintf(buff, "HTTP/1.1 400 Bad Request\r\n");
	send(clientsfd, buff, strlen(buff), 0);
}

//send file content to socket file descriptor
void send_file(int clientsfd, FILE* fp) {
	int length = 0;
	while (fgetc(fp) != EOF) {
		length++;
	}

	//发送Content-Length, Content-Type
	sprintf(buff, "Content-Length: %d\r\nContent-Type: text/html\r\n", length);
	send(clientsfd, buff, strlen(buff), 0);

	//发送换行符
	sprintf(buff, "\r\n");
	send(clientsfd, buff, strlen(buff), 0);

	//发送文件内容
	rewind(fp);
	size_t n = fread(buff, 1, BUFSIZE, fp);
	while (n > 0) {
		send(clientsfd, buff, n, 0);
		n = fread(buff, 1, BUFSIZE, fp);
	}
	fclose(fp);
}

void send_404(int clientsfd) {
	sprintf(buff, "HTTP/1.1 404 Not Found\r\n");
	send(clientsfd, buff, strlen(buff), 0);

	FILE* fp = fopen("no.html", "rb");
	send_file(clientsfd, fp);
}

//查找token在str中的位置，如果不存在返回-1
int indexOf(char* str, char* token) {
	int len1 = strlen(str);
	int len2 = strlen(token);

	if (len2 > len1) {
		return -1;
	}

	for (int i = 0; i < len1 - len2 - 1; i++) {
		int found = 1;
		for (int j = 0; j < len2; j++) {
			if (str[i + j] != token[j]) {
				found = 0;
			}
		}

		if (found) {
			return i;
		}
	}

	return -1;
}

//从请求字符串中获取数字，如s = "a=1&b=3&c=3"中，getInt(s, "a') == 1;
int getInt(char* str, char* token) {
	int index = indexOf(str, token);

	int result;
	sscanf(str + index + 5, "%d", &result);
	return result;
}

//创建数字计算的结果网页文件
void createResultFile(int result) {
	FILE* fp = fopen("result.html", "w");
	fprintf(fp, "<html>\n");
	fprintf(fp, "<head>\n");
	fprintf(fp, "<title>Your result</title>\n");
	fprintf(fp, "</head>\n");
	fprintf(fp, "<body>\n");
	fprintf(fp, "<h1>your result is: %d</h1>\n", result);
	fprintf(fp, "</body>\n");
	fprintf(fp, "</html>\n");
	fclose(fp);
}

//工作线程函数
void* workerThread(void* para) {
	char protocol[BUFSIZE];
	char filename[BUFSIZE];
	char query[BUFSIZE];
	int isQuery = 0;

	int socketFd = *((int*) para);

	//read first line
	char line[BUFSIZE];
	if (!readLine(socketFd, line)) {
		send_500(socketFd);
		return NULL;
	}

	//debug message, print header
	printf("header: %s\n", line);

	//get protocol type, GET or PUT
	sscanf(line, "%s", protocol);

	if (strcmp(protocol, "GET") == 0) {
		strcpy(query, line);

		//如果包含?, 是计算请求，如http://localhost/compute?num1=3&type=3&num2=3
		int index = indexOf(line, "?");
		if (index != -1) {
			isQuery = 1;
			strcpy(query, query + index + 1);
		}
	} else if (strcmp(protocol, "POST") == 0) {
		//read the remaining lines in header
		//the target of while loop is finding the empty line after header
		while (1) {
			readLine(socketFd, line);

			//printf("line: %s\n", line);
			if (strlen(line) == 1) {
				//find the empty line, only contains '\r'
				break;
			}
		}

		isQuery = 1;
		recv(socketFd, query, BUFSIZE, 0);
	} else {
		send_400(socketFd);
		return NULL;
	}

	//get file name
	sscanf(line, "%*s%s", filename);
	strcpy(buff, filename + 1);
	strcpy(filename, buff);

	//debug message, print file name
	printf("the file name requesting: %s\n", filename);

	if (isQuery) {
		//是计算请求
		printf("query: %s\n", query);

		int num1 = getInt(query, "num1");
		int num2 = getInt(query, "num2");
		int type = getInt(query, "type");

		printf("num1: %d, num2: %d, type: %d\n", num1, num2, type);

		int result = 0;
		if (type == 1) {
			result = num1 + num2;
		} else if (type == 2) {
			result = num1 - num2;
		} else if (type == 3) {
			result = num1 * num2;
		} else {
			result = num1 / num2;
		}

		createResultFile(result);
		send_200(socketFd);
		FILE* fp = fopen("result.html", "rb");
		send_file(socketFd, fp);
	} else {
		//是获取网页的请求

		FILE* fp = fopen(filename, "rb");
		if (!fp) {
			//not found
			send_404(socketFd);
		} else {
			send_200(socketFd);
			send_file(socketFd, fp);
		}
	}

	return NULL;
}

int main() {
	setbuf(stdout, NULL);
	startServer();
	return 0;
}

