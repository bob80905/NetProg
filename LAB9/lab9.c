#include	"unp.h"
#include	<netinet/tcp.h>		/* for TCP_MAXSEG */

int main(int argc, char **argv) {
	int	sockfd, rcvbuf, mss;
	socklen_t len;
	struct sockaddr_in servaddr;

	if (argc != 2) {
		perror("Invalid arguments: ./a.out <IPaddress>");
		abort();
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	len = sizeof(rcvbuf);
	getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
	len = sizeof(mss);
	getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len);
	printf("Defaults: SO_RCVBUF = %d, MSS = %d\n", rcvbuf, mss);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(80);		// port 80
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	len = sizeof(rcvbuf);
	getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
	len = sizeof(mss);
	getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len);
	printf("After connect: SO_RCVBUF = %d, MSS = %d\n", rcvbuf, mss);

	close(sockfd);
	exit(0);
}