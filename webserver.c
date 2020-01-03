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

void* workerThread(void* para);

void startServer() {
	//����������socket��ַ
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	//����������socket
	int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);

	//��socket��ip��port�󶨣� ���port������ռ�ã����ʧ��
	if (bind(serverSocketFd, (struct sockaddr *) &server_addr,
			sizeof(struct sockaddr_in)) == -1) {
		printf("bind server socket error\n");
		return;
	}

	//��ʼ������10Ϊδ���Ӷ�����������ƣ���������10��clientͬʱ���ӣ�10�����޸ģ����Ϊ512
	if (listen(serverSocketFd, 10) == -1) {
		printf("listen server socket error\n");
		return;
	}

	//wait for client connection
	printf("waiting for incoming client...\n");

	//���Ͻ��տͻ������ӣ���Ϊÿ���ͻ������߳�
	while (1) {

		struct sockaddr_in clientSocketAddr;
		int server_addr_len = sizeof(struct sockaddr_in);

		//���տͻ������ӣ�����һ��socket. ֱ���ɹ����յ�һ������Ϊֹ
		int clientSocketFd = accept(serverSocketFd,
				(struct sockaddr *) &clientSocketAddr, &server_addr_len);

		if (clientSocketFd < 0) {
			//����ʧ��
			printf("Error: accept failed.\n");
			continue;
		}

		//�����߳���client����
		pthread_t pid;
		pthread_create(&pid, NULL, workerThread, (void*) &clientSocketFd);
	}
}

//����socket��������
int sendData(int socketFd, const void* buf, size_t length) {
	int status = send(socketFd, buf, length, 0); //�������ݣ�����ʵ�ʷ��͵��ֽ���

	//��鷢�ͽ���Ƿ�����
	if (status <= 0) {
		return 0;
	}
	return 1;
}

//����socket��������
int recvData(int socketFd, void* buf, size_t length) {
	int receivedBytes = 0;

	//�������գ�ֱ���յ�length���ֽ�Ϊֹ
	while (receivedBytes < length) {
		void* currentBuf = buf + receivedBytes; //���յ�������Ҫ��ŵ���ʼ��ַ
		int remainBytes = length - receivedBytes; //��Ҫ���յ�ʣ���ֽ���
		int status = recv(socketFd, currentBuf, remainBytes, 0); //�������ݣ�����ʵ�ʽ��յ��ֽ���

		//�����ս���Ƿ�����
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

void send_404(int clientsfd) {
	sprintf(buff, "HTTP/1.1 404 Not Found\r\n");
	send(clientsfd, buff, strlen(buff), 0);
}

//send file content to socket file descriptor
void send_file(int clientsfd, FILE* fp) {
	sprintf(buff, "\r\n");
	send(clientsfd, buff, strlen(buff), 0);

	size_t n = fread(buff, sizeof(char), BUFSIZE, fp);
	while (n > 0) {
		send(clientsfd, buff, n, 0);
		n = fread(buff, sizeof(char), BUFSIZE, fp);
	}
	fclose(fp);
}

void* workerThread(void* para) {
	char protocol[BUFSIZE];
	char filename[BUFSIZE];
	char contentLengthLabel[BUFSIZE];
	int contentLength;

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
		//get file name
		sscanf(line, "%*s%s", filename);
		strcpy(buff, filename + 1);
		strcpy(filename, buff);

		//debug message, print file name
		printf("the file name requesting: %s\n", filename);

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

