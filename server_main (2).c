#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define MAX  80
#define PORT 8080
#define SA struct sockaddr
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Function designed for chat between client and server.
void * func(void* arg)
{
	int connfd = *((int *)arg);
	char buff[MAX];
	int n;
	// infinite loop for chat
	for (;;) {

		bzero(buff, MAX);

		// read the message from client and copy it in buffer
		read(connfd, buff, sizeof(buff));

		pthread_mutex_lock(&lock);
		// print buffer which contains the client contents
		printf("From client: %s\n", buff);
		pthread_mutex_unlock(&lock);

		if (strncmp("exit", buff, 4) == 0) {
			break;
		}
		
		//bzero(buff, MAX);
		//n = 0;
		// copy server message in the buffer
		//while ((buff[n++] = getchar()) != '\n');

		// and send that buffer to client
		//write(connfd, buff, sizeof(buff));

		//if msg contains "Exit" then server exit and chat ended.
		//if (strncmp("exit", buff, 4) == 0) {
		//	printf("Server Exit...\n");
		//	break;
		//}
	}
   close(connfd);
   pthread_exit(NULL);	
}

// Driver function
int main()
{
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
		printf("socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(sockfd, 50)) != 0) {
		printf("Listen failed...\n");
		exit(0);
	}
	else
		printf("Server listening..\n");
	len = sizeof(cli);

    pthread_t thread_ids[10];

    int i = 0;
    while(1) {
        // Accept the data packet from client and verification
		connfd = accept(sockfd, (SA*)&cli, &len);
		if (connfd < 0) {
			printf("server accept failed...\n");
			exit(0);
		}
		else {
			printf("server accept the client...\n");
		}
		if(pthread_create(&thread_ids[i++], NULL, func, &connfd) != 0 ) {
			printf("Failed to create thread\n");
			exit(0);
		}
        if( i >= 10) {
			i = 0;
			while(i < 10)
			{
				pthread_join(thread_ids[i++],NULL);
			}
			break;
		}
	}

	// After chatting close the socket
	close(sockfd);
}
