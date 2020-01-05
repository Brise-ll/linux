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

	sprintf(buff, "Content-Length: %d\r\nContent-Type: text/html\r\n", length);
	send(clientsfd, buff, strlen(buff), 0);

	sprintf(buff, "\r\n");
	send(clientsfd, buff, strlen(buff), 0);

	rewind(fp);
	size_t n = fread(buff, 1, BUFSIZE, fp);
	while (n > 0) {
		send(clientsfd, buff, n, 0);
		n = fread(buff, 1, BUFSIZE, fp);
	}
	fclose(fp);
}

//����token��str�е�λ�ã���������ڷ���-1
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

void* workerThread(void* para) {
	char protocol[BUFSIZE];
	char filename[BUFSIZE];
	char query[BUFSIZE];
	char contentLengthLabel[BUFSIZE];
	int contentLength;
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
		int index = indexOf(line, "?");
		if (index != -1) {
			isQuery = 1;
			strcpy(query, query + index + 1);
		}
	} else if (strcmp(protocol, "POST") == 0) {
		isQuery = 1;
		readLine(socketFd, query);
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

	} else {
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

