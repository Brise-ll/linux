
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

void startServer(){
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
}

void workerThread(void* para){

}

int main(){

	startServer();
	return 0;
}


