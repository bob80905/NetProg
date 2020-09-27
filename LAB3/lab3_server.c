#include <stdint.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <stdio.h>

#include "lib/unp.h" // Change before submitting

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
	servaddr.sin_port = htons((SERV_PORT + atoi(argv[1]))); // Add to 9877

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);

	while (1) {
		clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;
			else perror("accept error");
		}

		Close(listenfd);

		// Read from stdin, then send to client
		char buf[MAXLINE];
		fgets(buf, MAXLINE, stdin);
		int n = send(connfd, buf, sizeof(buf), 0);

		Close(connfd);
	}

	return EXIT_SUCCESS;
}