#include <stdint.h>
#include <sys/types.h> 
#include <sys/socket.h>

#include "lib/unp.h"

/* error code

0         Not defined, see error message (if any).
1         File not found.
2         Access violation.
3         Disk full or allocation exceeded.
4         Illegal TFTP operation.
5         Unknown transfer ID.
6         File already exists.
7         No such user.

*/

#define MAX_DATA_SIZE 513

typedef struct {
     int port;
     pid_t pid;
}Port;


typedef enum {
     op_RRQ = 1,
     op_WRQ = 2,
     op_DATA = 3,
     op_ACK = 4,
     op_ERROR = 5
}opcode; 


// opcode, filename, 0, mode, 0
typedef struct {
	uint16_t opcode;
	char filenameMode[MAX_DATA_SIZE];
}RRQ;


// opcode, filename, 0, mode, 0
typedef struct {
	uint16_t opcode;
	char filenameMode[MAX_DATA_SIZE];
}WRQ;


// opcode, block, data
typedef struct {
	uint16_t opcode;
	uint16_t blockNum;
	char data[MAX_DATA_SIZE];
}DATA;


// opcode block#
typedef struct {
	uint16_t opcode;
	uint16_t blockNum;
}ACK;


// opcode errorcode errmsg 0
typedef struct {
	uint16_t opcode;
	uint16_t errorCode;
	char errorMsg[MAX_DATA_SIZE];
}ERROR;



typedef struct{
	union {
		RRQ rrq;
		WRQ wrq;
		DATA data;
		ACK ack;
		ERROR error;
	} type;
}PACKET;

Port* arr;
int arr_size;


ssize_t send_ACK(int sd, int blockNum, struct sockaddr* host, socklen_t host_len) {
	PACKET p;
	p.type.ack.opcode = htons(op_ACK);
	p.type.ack.blockNum = htons(blockNum);
	ssize_t n;
	if ((n = sendto(sd, &p, 4, 0, host, host_len)) < 0) {		//https://stackoverflow.com/questions/22415077/sendto-invalid-argument
		perror("send ack failed\n");
	}
	return n;
}


ssize_t send_ERROR(int sd, int errorCode, char* errorMsg, struct sockaddr* host, socklen_t host_len) {
	PACKET p;
	p.type.error.opcode = htons(op_ERROR);
	p.type.error.errorCode = htons(errorCode);
	int len;
	if (errorMsg != NULL) {
		strcpy(p.type.error.errorMsg, errorMsg);	//at least 1 byte, strcpy include the null pointer by default
		len = strlen(p.type.error.errorMsg);					// strlen not include the null pointer
	}else{
		len = 0;
	}
	ssize_t n;							
	if ((n = sendto(sd, &p, 4 + len + 1, 0, host, host_len)) < 0) {
		perror("send error failed\n");		
	}
	return n;
}


ssize_t send_DATA(int sd, int blockNum, char* buffer, struct sockaddr* host, socklen_t host_len) {
	PACKET p;
	p.type.data.opcode = htons(op_DATA);
	p.type.data.blockNum = htons(blockNum);
	strcpy(p.type.data.data, buffer);	// strcpy include the null pointer by default
	ssize_t n;
	if ((n = sendto(sd, &p, strlen(p.type.data.data) + 4, 0, host, host_len)) < 0){
		perror("send data failed\n");
	}
	return n;
}

unsigned short int get_opcode(PACKET* p) {	//https://beginnersbook.com/2014/01/c-passing-array-to-function-example/V
	unsigned short int opcode;
	unsigned short int* opcode_ptr;
	opcode_ptr = (unsigned short int* )p;
	opcode = ntohs(*opcode_ptr);
	return opcode;
}


FILE* checkFilenameMode(PACKET* p) {
	char* filename = p->type.rrq.filenameMode;
	char* filemode = strchr(filename,'\0') + 1;		
	if (strcmp(filemode, "octet")!=0) {
		perror("wrong mode\n");
		// send error, error num??
	}
	FILE* fd = fopen(filename, "r");
	return fd;
}



int get_dest_port(Port* arr, unsigned short int port_range_start, unsigned short int port_range_end) {
	// the next highest port?

	for (int i = port_range_start; i <= port_range_end; i++) {	 		
	 	if (arr[i].pid == 0) {
			arr[i].pid = -1;	// -1 means it is reserved for a later pid assignment
	 		return arr[i].port;
		}
		// printf("highest = %d\n", highest);
	}
	return -1; //if all ports are in use, return an error
}


void set_pid(Port* arr, unsigned short int port , int arr_size, pid_t pid) {
	for (unsigned int i = 0; i < arr_size; ++i) {
		if (arr[i].port == port){
			arr[i].pid = pid;
			return;
		}
	}
	return;
}


void remove_pid(Port* arr, pid_t pid, int arr_size) {
	for (unsigned int i = 0; i < arr_size; ++i) {
		if (arr[i].pid == pid){
			arr[i].pid = 0;
			return;
		}
	}
	return;
}

int same_host(char* host1_ip, unsigned short int host1_port, char* host2_ip, unsigned short int host2_port) {
	return (!strcmp(host1_ip, host2_ip)) && host1_port == host2_port;
}

void sighandler_t(int signum){	
	pid_t pid;
	while((pid = waitpid(-1, NULL, WNOHANG)) != -1) {	// when stop?
		remove_pid(arr, pid, arr_size);
	}	
}



// int handle_RRQ(FILE* fd, int dest_port,struct sockaddr* requesting_host, 
// 	socklen_t request_len, struct sockaddr* responding_host, socklen_t respond_len, PACKET* p) {
// }




int main(int argc, char* argv[]) {

	if( argc != 3 ) {
		printf("Expected 2 arguments: port range start and end.");
		exit(1); //program needs start and end ports to define port range.
	}


	unsigned short int port_range_start = atoi(argv[1]);
	unsigned short int port_range_end = atoi(argv[2]);


	arr_size = port_range_end - port_range_start + 1;
	arr = calloc(arr_size, sizeof(Port));
	
	for (unsigned int i = 0; i < arr_size; ++i) {
		arr[i].port = port_range_start + i;
		arr[i].pid = 0;
	}

	// start a server
	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in responding_host;
	responding_host.sin_family = AF_INET;
	responding_host.sin_addr.s_addr = htonl(INADDR_ANY);
	responding_host.sin_port = htons(port_range_start);

	if ( bind( sd, (struct sockaddr *) &responding_host, sizeof( responding_host ) ) < 0 ) {
        perror( "bind() failed" );
        return EXIT_FAILURE;
	}

	while(1) {
		Signal(SIGCHLD, sighandler_t);

		PACKET receive_p;
		struct sockaddr_in requesting_host;
		socklen_t request_len = sizeof(requesting_host);

		initial_listen:;
		ssize_t n = recvfrom(sd, &receive_p, sizeof(receive_p), 0, (struct sockaddr *)&requesting_host, &request_len); // cast
		if(n < 0 && errno == EINTR){
			goto initial_listen;
		}
		if (n < 0 && errno != EINTR){
			perror("Recvfrom failed.\n");
			exit(1);
		}
		if ( n == -1 ) {
      		perror( "recvfrom() failed\n" );
    	}
    	
    	// record the ip and port in the first recvfrom
    	char* request_ip = inet_ntoa(requesting_host.sin_addr);
		unsigned short int request_port = ntohs(requesting_host.sin_port);
    	
		// check opcode, filename, mode
		unsigned short int opcode = get_opcode(&receive_p);
		if(opcode != op_RRQ && opcode != op_WRQ) {
			send_ERROR(sd, 4, NULL, (struct sockaddr* )&requesting_host, request_len);	
			continue;
		}
		else if (opcode == op_RRQ){
			FILE* fd;
			if ((fd = checkFilenameMode(&receive_p)) == NULL){
				send_ERROR(sd, 1, NULL, (struct sockaddr* )&requesting_host, request_len);	// error num
			}else{
				// fork to handle RRQ
				int dest_port = get_dest_port(arr, port_range_start, port_range_end);
				if(dest_port == -1){
					perror("Cannot allocate a port, all ports in use");
					continue;
				}
				pid_t pid = fork();
				if (pid > 0){
					set_pid(arr, dest_port , arr_size, pid);
					fclose(fd);
				}

				if (pid == 0) {
					close(sd);
					// handle_RRQ(fd, dest_port, (struct sockaddr_in*)&requesting_host, request_len,(struct sockaddr_in*)&responding_host, sizeof( responding_host ),&receive_p);
					
					int sd_new = socket(AF_INET, SOCK_DGRAM, 0);
					if ( sd_new == -1 ) {
				    	perror( "socket() failed" );
				    	return EXIT_FAILURE;
				  	}
					responding_host.sin_port = htons(dest_port);
					if ( bind( sd_new, (struct sockaddr *) &responding_host, sizeof( responding_host )) < 0 ) {
				        perror( "bind() failed1" );
				        return EXIT_FAILURE;
					}
				    
					send_ACK(sd_new, 0, (struct sockaddr* )&requesting_host, request_len);

					int blockNum = 0;
					int finished = 0;

					while(finished == 0){
						char dataSend[512];	// fread will not append NULL at end
						size_t numBytes = fread(dataSend, 1, 512, fd);
						blockNum += 1;
						dataSend[numBytes] = '\0';
						if (numBytes < 512) {
							
							finished = 1;
						}		
						struct timeval timeout;
						timeout.tv_sec = 1;
						timeout.tv_usec = 0;

						int count = 0;
						while(1) {

							if (count == 10) { 
								close(sd_new); 
								//printf("timeout, disconnect\n");
								return EXIT_SUCCESS; 	// child process end, signal handler remove_pid
							}

							send_DATA(sd_new, blockNum, dataSend, (struct sockaddr* ) &requesting_host, request_len);
							fd_set readfds;
							FD_ZERO( &readfds );
							FD_SET(sd_new, &readfds);
							select( FD_SETSIZE, &readfds, NULL, NULL, &timeout);
							timeout.tv_sec = 1;
							timeout.tv_usec = 0;

							if (FD_ISSET(sd_new, &readfds)) {	
								
								memset (&receive_p, 0, sizeof(receive_p));	// will reuse the packet p,  zero out the memory
								rrq_initial_recv:;
								int bytes_received = recvfrom(sd_new, &receive_p, sizeof(receive_p), 0, (struct sockaddr *)&requesting_host, &request_len);
								if(bytes_received < 0 && errno == EINTR){
									goto rrq_initial_recv;
								}
								if (bytes_received < 0 && errno != EINTR){
									perror("Recvfrom failed.\n");
									exit(1);
								}

	    	   					// record the new ip and port
	    	   					char* request_ip_new = inet_ntoa(requesting_host.sin_addr);
								unsigned short int request_port_new = ntohs(requesting_host.sin_port);
							
								if (!same_host(request_ip, request_port, request_ip_new, request_port_new)) {
									send_ERROR(sd_new, 5, NULL, (struct sockaddr* )&requesting_host, request_len);
								}
								if (get_opcode(&receive_p) == op_ERROR) {

									//if after sending a packet, an error is returned, continue sending the packet?
									continue;
								}
								if(get_opcode(&receive_p) == op_RRQ){
									send_ACK(sd_new, 0, (struct sockaddr* )&requesting_host, request_len);

								}
								if (get_opcode(&receive_p) != op_ACK){
									//printf("opcode is not ack\n");
									send_ERROR(sd_new, 4, NULL, (struct sockaddr* )&requesting_host, request_len);	
								}
								
								if (get_opcode(&receive_p) == op_ACK && ntohs(receive_p.type.ack.blockNum) == blockNum) {
									break;		// go to send next block
								}
					
							}
							count = count + 1;

						} // while(1)
					} 	  // while(!finished)

					close(sd_new);
					fclose(fd);
					//printf("blocks all sent\n");
					return EXIT_SUCCESS;

				}	// child process
			}

		}
		else if (opcode == op_WRQ){
			
			//printf("Received WRQ request.\n");
			// fork to handle RRQ
			int dest_port = get_dest_port(arr, port_range_start, port_range_end);
			if(dest_port == -1){
				perror("Cannot allocate a port, all ports in use");
				continue;
			}

			char* filename_mode = receive_p.type.wrq.filenameMode;
			char* filename = calloc(1024, sizeof(char));
			strcpy(filename, filename_mode);
			//printf("%s\n", filename);

			FILE* f = fopen(filename, "w");
			free(filename);
			if (!f){
				printf("ERROR: fopen failed.\n");
				exit(1);
			}

			pid_t pid = fork();
			if (pid > 0){
				set_pid(arr, dest_port , arr_size, pid);
				fclose(f);
			}

			if (pid == 0) {
				close(sd);
				// handle_RRQ(fd, dest_port, (struct sockaddr_in*)&requesting_host, request_len,(struct sockaddr_in*)&responding_host, sizeof( responding_host ),&receive_p);
				
				int sd_new = socket(AF_INET, SOCK_DGRAM, 0);
				if ( sd_new == -1 ) {
			    	perror( "socket() failed" );
			    	return EXIT_FAILURE;
			  	}
				responding_host.sin_port = htons(dest_port);
				if ( bind( sd_new, (struct sockaddr *) &responding_host, sizeof( responding_host )) < 0 ) {
			        perror( "bind() failed1" );
			        return EXIT_FAILURE;
				}
			    
				send_ACK(sd_new, 0, (struct sockaddr* )&requesting_host, request_len);
				//printf("ACKing WRQ request\n");
				int blockNum = 0;
				int finished = 0;

				
				
				while(finished == 0){
					blockNum = blockNum + 1;

					struct timeval timeout;
					timeout.tv_sec = 1;
					timeout.tv_usec = 0;

					int count = 0;
					while(1) { //continue acking a packet until we get the next packet

						if (count == 10) { 
							close(sd_new); 
							//printf("timeout, disconnect\n");
							return EXIT_SUCCESS; 	// child process end, signal handler remove_pid
						}

						
						fd_set readfds;
						FD_ZERO( &readfds );
						FD_SET(sd_new, &readfds);
						select( FD_SETSIZE, &readfds, NULL, NULL, &timeout);
						timeout.tv_sec = 1;
						timeout.tv_usec = 0;

						if(count > 0){
							send_ACK(sd_new, blockNum, (struct sockaddr* )&requesting_host, request_len);
						}


						if (FD_ISSET(sd_new, &readfds)) {	
							
							memset (&receive_p, 0, sizeof(receive_p));	// will reuse the packet p,  zero out the memory
							wrq_initial_recv:;
							int bytes_received = recvfrom(sd_new, &receive_p, sizeof(receive_p), 0, (struct sockaddr *)&requesting_host, &request_len);
							if(bytes_received < 0 && errno == EINTR){
								printf("EINTR block\n");
								goto wrq_initial_recv;
							}
							if (bytes_received < 0 && errno != EINTR){
								perror("Recvfrom failed.\n");
								exit(1);
							}
							if (bytes_received < 512) {
								finished = 1;
							}
    	   					// record the new ip and port
    	   					char* request_ip_new = inet_ntoa(requesting_host.sin_addr);
							unsigned short int request_port_new = ntohs(requesting_host.sin_port);
						
							if (!same_host(request_ip, request_port, request_ip_new, request_port_new)) {
								send_ERROR(sd_new, 5, NULL, (struct sockaddr* )&requesting_host, request_len);
							}
							if (get_opcode(&receive_p) == op_ERROR) {
								//if after sending a packet, an error is returned, continue sending the packet?
								continue;
							}

							if(get_opcode(&receive_p) == op_WRQ){
								send_ACK(sd_new, 0, (struct sockaddr* )&requesting_host, request_len);
							}
							
							if (get_opcode(&receive_p) == op_DATA && ntohs(receive_p.type.data.blockNum) == blockNum){
								//printf("Sending ACK\n");
								//printf("%s\n, %d\n",  receive_p.type.data.data, bytes_received);
								int length = strlen(receive_p.type.data.data); 
								int result = fwrite(receive_p.type.data.data, 1, length, f);
								fflush(f);
								if(result < 0){
									printf("ERROR, failed to write string to file\n");
								}
								//printf("Received: %s\n", receive_p.type.data.data);
								send_ACK(sd_new, blockNum, (struct sockaddr* )&requesting_host, request_len);
								break;

							}
				
						}
						count = count + 1;

					} // while(1)

				} 	  // while(!finished)

				close(sd_new);
				fclose(f);
				//printf("blocks all received\n");
				return EXIT_SUCCESS;

			}

		}

	}

	// free arr
	return 0;
}
