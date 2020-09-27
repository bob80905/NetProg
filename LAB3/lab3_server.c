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
	while (1) {
		clilen = sizeof(cliaddr);
		connfd = Accept(listenfd, (SA *) &cliaddr, &clilen);
		printf("Accepted connection\n");
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
		send(connfd, buf, length, 0);


		char* recvmsg = calloc(MAXLINE, sizeof(char));
		int recvbytes = recv(connfd, recvmsg, length, 0);
		
		if(recvbytes == 0){
			printf("str_cli: client disconnected\n");
			Close(connfd);
			Close(listenfd);
			free(recvmsg);
			free(buf);
			exit(0);
		}


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