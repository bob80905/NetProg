#include <stdint.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <stdio.h>

#include "../lib/unp.h" // Change before submitting

int PORT = 9877;


typedef struct {
	int fd;
	int port;
}portfd;

int insert(portfd pf, portfd connection_fds[]){
	for (int i = 0; i < 5; i++){
		if(connection_fds[i].fd == -1){
			connection_fds[i].fd = pf.fd;
			connection_fds[i].port = pf.port;
			return 1;
		}
	}
	return -1; //could not connect, too many connections.
}

int getmax(portfd connection_fds[]){
	int m = 0;
	for(int i = 0; i < 5; i++){
		if(connection_fds[i].fd > m){
			m = connection_fds[i].fd;
		}
	}
	return m;
}

int main(int argc, char **argv) {

	portfd connection_fds[5];
	for(int i = 0; i < 5; i++){
		connection_fds[i].fd = -1;
	}

	while(1){

		fd_set readfds;

		FD_ZERO( &readfds );
		FD_SET(0, &readfds);
		//add the servers we are connected to to the select listening array
		for (int i = 0; i < 5; i++){
			if (connection_fds[i].fd != -1){
				FD_SET(connection_fds[i].fd, &readfds);
			}
		}

		int maxfd = getmax(connection_fds);
		printf("Blocking on select...\n");
		select( maxfd+1, &readfds, NULL, NULL, NULL);
		printf("Done with select...\n");
		if (FD_ISSET(0, &readfds)){
			char number[MAXLINE];
			fgets(number, MAXLINE, stdin);
			int port_num = atoi(number);

			int sd = Socket(AF_INET, SOCK_STREAM, 0);


			//make a socket to communicate with the server at port port_num
			struct hostent* hp = gethostbyname("localhost");

			struct sockaddr_in server;
			server.sin_family = AF_INET;
			server.sin_addr.s_addr = htonl(INADDR_ANY);
			server.sin_port = htons(port_num);
			memcpy( (void *)&server.sin_addr, (void *)hp->h_addr, hp->h_length );

			portfd pf;
			pf.fd = sd;
			pf.port = port_num;
			int result = insert(pf, connection_fds); //maybe reference problems.
			
			if(result < 0){
				printf("Could not connect, too many connections.\n");
				continue;
			}
			else{
				Connect(sd, (SA*) &server, sizeof(server));
			}
		}

		//we've received a packet from a server
		else{
			for(int i = 0; i < 5; i++){
				if(connection_fds[i].fd != -1){

					if(FD_ISSET(connection_fds[i].fd, &readfds)){
						char msg[MAXLINE];
						recv:;
						int num_bytes = Recv(connection_fds[i].fd, msg, MAXLINE, 0);
						msg[num_bytes] = '\0';
						if(errno == EINTR){
							goto recv;
						}
						if(num_bytes == 0){
							//server shut down
							connection_fds[i].fd = -1;
							printf("Server on %d closed\n", connection_fds[i].port );
							continue;
						}
						printf("%d %s",connection_fds[i].port, msg);
						send(connection_fds[i].fd, msg, num_bytes, 0);

					}
				}
			}
		}



	}


}