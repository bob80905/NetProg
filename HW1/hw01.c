#include <stdint.h>
#include <sys/types.h> 
#include <sys/socket.h>

#include "unp.h"

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
}Port; //each port has a unique pid associated with it


typedef enum {
     op_RRQ = 1,
     op_WRQ = 2,
     op_DATA = 3,
     op_ACK = 4,
     op_ERROR = 5
}opcode; //5 separate opcodes


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
}PACKET; //packet joins the previous structs into one

Port* arr;
int arr_size;

//send an ack with a specific blocknum to a host with size host_len
ssize_t send_ACK(int sd, int blockNum, struct sockaddr* host, socklen_t host_len) {
	PACKET p;
	p.type.ack.opcode = htons(op_ACK);
	p.type.ack.blockNum = htons(blockNum);
	ssize_t n;

	//send the message, check for errors
	if ((n = sendto(sd, &p, 4, 0, host, host_len)) < 0) {		//https://stackoverflow.com/questions/22415077/sendto-invalid-argument
		perror("send ack failed\n");
	}
	return n;
}

//send an error with a specific error code and message to a host with a specific size.
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

	//send the message
	if ((n = sendto(sd, &p, 4 + len + 1, 0, host, host_len)) < 0) {
		perror("send error failed\n");		
	}
	return n;
}

//send a data packet with a block number and size to a specific host, containing data in buffer.
ssize_t send_DATA(int sd, int blockNum, char* buffer, struct sockaddr* host, socklen_t host_len) {
	PACKET p;
	p.type.data.opcode = htons(op_DATA);
	p.type.data.blockNum = htons(blockNum);
	strcpy(p.type.data.data, buffer);	// strcpy include the null pointer by default
	ssize_t n;

	//send to client, check for errors
	if ((n = sendto(sd, &p, strlen(p.type.data.data) + 4, 0, host, host_len)) < 0){
		perror("send data failed\n");
	}
	return n;
}

//return the opcode contained in the packet given a packet.
unsigned short int get_opcode(PACKET* p) {	//https://beginnersbook.com/2014/01/c-passing-array-to-function-example/V
	unsigned short int opcode;
	unsigned short int* opcode_ptr;
	opcode_ptr = (unsigned short int* )p;
	opcode = ntohs(*opcode_ptr);
	return opcode;
}

//return a file descriptor to a file that the client requests to read from.
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


//return the next valid port the server can listen on.
int get_dest_port(Port* arr, unsigned short int port_range_start, unsigned short int port_range_end) {

	for (int i = port_range_start; i <= port_range_end; i++) {	 		
	 	if (arr[i].pid == 0) {
			arr[i].pid = -1;	// -1 means it is reserved for a later pid assignment
	 		return arr[i].port;
		}
	}
	return -1; //if all ports are in use, return an error
}

//reserve a slot in the pid table for the server
void set_pid(Port* arr, unsigned short int port , int arr_size, pid_t pid) {
	for (unsigned int i = 0; i < arr_size; ++i) {
		if (arr[i].port == port){
			arr[i].pid = pid;
			return;
		}
	}
	return;
}

//release a slot in the pid table for the server.
void remove_pid(Port* arr, pid_t pid, int arr_size) {
	for (unsigned int i = 0; i < arr_size; ++i) {
		if (arr[i].pid == pid){
			arr[i].pid = 0;
			return;
		}
	}
	return;
}

//test to see if 2 hosts are the same or not
int same_host(char* host1_ip, unsigned short int host1_port, char* host2_ip, unsigned short int host2_port) {
	return (!strcmp(host1_ip, host2_ip)) && host1_port == host2_port;
}

//function for the parent to clean up the children.
void sighandler_t(int signum){	
	pid_t pid;
	while((pid = waitpid(-1, NULL, WNOHANG)) != -1) {
		remove_pid(arr, pid, arr_size);
	}	
}


int main(int argc, char* argv[]) {

	//need start and end range for port numbers
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

	// start a server, make listener socket
	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in responding_host;
	responding_host.sin_family = AF_INET;
	responding_host.sin_addr.s_addr = htonl(INADDR_ANY);
	responding_host.sin_port = htons(port_range_start); //set to listening port.

	//bind a socket to the server
	if ( bind( sd, (struct sockaddr *) &responding_host, sizeof( responding_host ) ) < 0 ) {
        perror( "bind() failed" );
        return EXIT_FAILURE;
	}

	while(1) {
		Signal(SIGCHLD, sighandler_t); //call signal handler when SIGCHLD is received

		PACKET receive_p;
		struct sockaddr_in requesting_host;
		socklen_t request_len = sizeof(requesting_host);

		initial_listen:;
		ssize_t n = recvfrom(sd, &receive_p, sizeof(receive_p), 0,
		 (struct sockaddr *)&requesting_host, &request_len); // cast
		//if recvfrom is interupted, recvfrom again
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
    	//data is now received
    	// record the ip and port in the first recvfrom
    	char* request_ip = inet_ntoa(requesting_host.sin_addr);
		unsigned short int request_port = ntohs(requesting_host.sin_port);
    	
		// check opcode, filename, mode
		unsigned short int opcode = get_opcode(&receive_p);
		if(opcode != op_RRQ && opcode != op_WRQ) {
			send_ERROR(sd, 4, NULL, (struct sockaddr* )&requesting_host, request_len);	
			continue;
		}
		//if client requests to read a file from the server
		else if (opcode == op_RRQ){
			FILE* fd;
			if ((fd = checkFilenameMode(&receive_p)) == NULL){
				send_ERROR(sd, 1, NULL, (struct sockaddr* )&requesting_host, request_len);	// error num
				//server may not have the file client wants to read
			}

			else{
				// fork to handle RRQ
				int dest_port = get_dest_port(arr, port_range_start, port_range_end);
				if(dest_port == -1){
					perror("Cannot allocate a port, all ports in use");
					continue;
				}
				//parent handles server resources, child actually serves client.
				pid_t pid = fork();
				if (pid > 0){
					set_pid(arr, dest_port , arr_size, pid);
					fclose(fd);
				}

				if (pid == 0) {
					close(sd);
					// handle_RRQ(fd, dest_port, (struct sockaddr_in*)&requesting_host, request_len,(struct sockaddr_in*)&responding_host, sizeof( responding_host ),&receive_p);
					
					//make new socket to handle communication to client, other than listening socket.
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
				    
					int blockNum = 0;
					int finished = 0;

					//while this RRQ request is still going.
					while(finished == 0){
						char dataSend[512];	// fread will not append NULL at end
						size_t numBytes = fread(dataSend, 1, 512, fd); //read data from file
						blockNum += 1;
						dataSend[numBytes] = '\0';
						if (numBytes < 512) {
							
							finished = 1;
						}		
						struct timeval timeout;
						timeout.tv_sec = 1;
						timeout.tv_usec = 0;

						int count = 0;
						//while an ack has not been received for the data packet that was sent out.
						while(1) { 

							if (count == 10) { 
								close(sd_new); 
								return EXIT_SUCCESS; 	// child process end, signal handler remove_pid
							}

							send_DATA(sd_new, blockNum, dataSend, (struct sockaddr* ) &requesting_host, request_len);
							fd_set readfds;
							FD_ZERO( &readfds );
							FD_SET(sd_new, &readfds);
							select( FD_SETSIZE, &readfds, NULL, NULL, &timeout);
							timeout.tv_sec = 1;
							timeout.tv_usec = 0;

							//if a packet was received from the client
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
								//send errors if unexpected packet is received, if error packet is received,
								// keep waiting for ack
								if (!same_host(request_ip, request_port, request_ip_new, request_port_new)) {
									send_ERROR(sd_new, 5, NULL, (struct sockaddr* )&requesting_host, request_len);
								}

								if (get_opcode(&receive_p) == op_WRQ){
									send_ERROR(sd_new, 4, NULL, (struct sockaddr* )&requesting_host, request_len);
								}

								if (get_opcode(&receive_p) == op_ERROR) {
									count = count+1;
									continue;
								}
								
								if (get_opcode(&receive_p) != op_ACK){
									send_ERROR(sd_new, 4, NULL, (struct sockaddr* )&requesting_host, request_len);	
								}
								//if we get the ack for the data packet with correct blocknum, send next packet.
								if (get_opcode(&receive_p) == op_ACK && ntohs(receive_p.type.ack.blockNum) == blockNum) {
									break;		// go to send next block
								}
					
							}
							count = count + 1;

						} 
					} 	  

					close(sd_new);
					fclose(fd);
					return EXIT_SUCCESS;

				}	// child process
			}

		}
		//if server received write request.
		else if (opcode == op_WRQ){
			
			//Received WRQ request
			// fork to handle RRQ
			int dest_port = get_dest_port(arr, port_range_start, port_range_end);
			if(dest_port == -1){
				perror("Cannot allocate a port, all ports in use");
				continue;
			}

			char* filename_mode = receive_p.type.wrq.filenameMode;
			char* filename = calloc(1024, sizeof(char));
			strcpy(filename, filename_mode);
			

			FILE* f = fopen(filename, "w");
			free(filename);
			if (!f){
				printf("ERROR: fopen failed.\n");
				exit(1);
			}

			pid_t pid = fork();
			//parent handles server resources, child actually serves client.
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
				//bind new socket dedicated to client to server
				if ( bind( sd_new, (struct sockaddr *) &responding_host, sizeof( responding_host )) < 0 ) {
			        perror( "bind() failed1" );
			        return EXIT_FAILURE;
				}
			    
			    //continue to send an ack packet for the initial request
			    // to client until the next packet is received 
			    fd_set readfds;
			    for(int i = 0; i < 10; i++){
			    	struct timeval timeout;
			    	timeout.tv_sec = 1;
					timeout.tv_usec = 0;
					send_ACK(sd_new, 0, (struct sockaddr* )&requesting_host, request_len);
					FD_ZERO( &readfds );
					FD_SET(sd_new, &readfds);
					select( FD_SETSIZE, &readfds, NULL, NULL, &timeout);
					if(FD_ISSET(sd_new, &readfds)) {	
						break;
					}			
				}
				
				int blockNum = 0;
				int finished = 0;

				
				//while the wrq operation is taking place (sending multiple acks)
				while(finished == 0){
					blockNum = blockNum + 1;

					struct timeval timeout;
					timeout.tv_sec = 1;
					timeout.tv_usec = 0;

					int count = 0;
					//while we are waiting on the next data packet
					while(1) { //continue acking a packet until we get the next packet

						if (count == 10) { 
							close(sd_new); 
							
							return EXIT_SUCCESS; 	// child process end, signal handler remove_pid
						}

						
						FD_ZERO( &readfds );
						FD_SET(sd_new, &readfds);
						select( FD_SETSIZE, &readfds, NULL, NULL, &timeout); //see if client sent anything
						timeout.tv_sec = 1;
						timeout.tv_usec = 0;

						if(count > 0){ //resend ack
							send_ACK(sd_new, blockNum, (struct sockaddr* )&requesting_host, request_len);
						}


						if (FD_ISSET(sd_new, &readfds)) {	 //if server received something from client
							
							memset (&receive_p, 0, sizeof(receive_p));	// will reuse the packet p,  zero out the memory
							wrq_initial_recv:;
							int bytes_received = recvfrom(sd_new, &receive_p, sizeof(receive_p), 0, (struct sockaddr *)&requesting_host, &request_len);
							if(bytes_received < 0 && errno == EINTR){
								//printf("EINTR block\n");
								goto wrq_initial_recv;
							}
							if (bytes_received < 0 && errno != EINTR){
								perror("Recvfrom failed.\n");
								exit(1);
							}
							if (bytes_received < 516) { //if server received < 516B, this is the last data packet
								finished = 1;
							}
							if (bytes_received > 516) {
								send_ERROR(sd_new, 5, NULL, (struct sockaddr* )&requesting_host, request_len);
							}
    	   					// record the new ip and port
    	   					char* request_ip_new = inet_ntoa(requesting_host.sin_addr);
							unsigned short int request_port_new = ntohs(requesting_host.sin_port);
						
							if (!same_host(request_ip, request_port, request_ip_new, request_port_new)) {
								send_ERROR(sd_new, 5, NULL, (struct sockaddr* )&requesting_host, request_len);
							}
							if (get_opcode(&receive_p) == op_ERROR) {
								count = count + 1;
								//if after sending a packet, an error is returned, continue sending the packet?
								continue;
							}

							if(get_opcode(&receive_p) == op_WRQ){ //if client still sending an initial request
								//ack it.
								send_ACK(sd_new, 0, (struct sockaddr* )&requesting_host, request_len);
							}

							if (get_opcode(&receive_p) == op_RRQ){
								send_ERROR(sd_new, 4, NULL, (struct sockaddr* )&requesting_host, request_len);
							}
							
							if (get_opcode(&receive_p) == op_DATA && ntohs(receive_p.type.data.blockNum) == blockNum){
								
								int length = strlen(receive_p.type.data.data); 
								int result = fwrite(receive_p.type.data.data, 1, length, f);
								fflush(f);
								if(result < 0){
									perror("ERROR, failed to write string to file\n");
									exit(1);
								}

								//send ack if correct data packet is received
								send_ACK(sd_new, blockNum, (struct sockaddr* )&requesting_host, request_len);
								break;

							}
				
						}
						count = count + 1;

					} 

				} 	  

				close(sd_new);
				fclose(f);
				
				return EXIT_SUCCESS;

			}

		}

	}

	return 0;
}