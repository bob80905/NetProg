#include <stdint.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <stdio.h>

#include "../lib/unp.h" // Change before submitting

int main(int argc, char **argv) {
	if (argc != 2) {
		perror("Invalid arguments");
		abort();
	}

	int listenfd, connfd;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int listening_port = 9877 + atoi(argv[1]);
	servaddr.sin_port = htons(listening_port); // Add to 9877

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, 1);
	int connected = 0;
	while (1) {
		clilen = sizeof(cliaddr);
		if(connected == 0){
			connfd = Accept(listenfd, (SA *) &cliaddr, &clilen);
			printf("Accepted connection\n");
		}
		connected = 1;
		if ( connfd < 0) {

			if (errno == EINTR){
				continue;
			}
			else{
				perror("accept error");
			}
		}

		// Read from stdin, then send to client
		char* buf = calloc(MAXLINE, sizeof(char));
		char* input = fgets(buf, MAXLINE, stdin);
		int length = strlen(input);
		printf("Sending: %s\n", buf);
		send(connfd, buf, length, 0);


		char* recvmsg = calloc(MAXLINE, sizeof(char));
		int recvbytes = recv(connfd, recvmsg, length, 0);
		printf("Received echo: %s\n", recvmsg );
		
		if(recvbytes == 0){
			printf("str_cli: client disconnected\n");
			connected = 0;
			Close(connfd);
			Close(listenfd);
			free(recvmsg);
			free(buf);
			exit(0);
		}


		//loop and see if there's an EOF char
		if(strcmp(recvmsg, buf)!=0){ 
		//if theres an EOF in the string we receive from the client
			printf("recvmsg = %s \n buf = %s \n", recvmsg, buf);
			printf("Shutting down due to EOF\n");
			Close(connfd);
			Close(listenfd);
			free(recvmsg);
			free(buf);
			exit(0);
		}


		free(recvmsg);
		free(buf);
		
	}

	return EXIT_SUCCESS;
}