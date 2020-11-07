/*
* lab7.c
* CSCI 4220 Lab 7
* RPI Fall 2020
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h> // for inet_ntoa

int main(int argc, char ** argv) {
    if (argc != 2) {
        perror("Invalid arguments");
        return EXIT_FAILURE;
    }

    struct addrinfo hints, *res, *res0;
    int error;
    int s;
    const char *cause = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo(argv[1], "http", &hints, &res0);
    if (error) {
        perror("getaddrinfo failed");
        return EXIT_FAILURE;
    }
    s = -1;
    for (res = res0; res; res = res->ai_next) {
            s = socket(res->ai_family, res->ai_socktype,
                res->ai_protocol);
            if (s < 0) {
                    cause = "socket";
                    continue;
            }

            if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
                    cause = "connect";
                    // close(s);
                    s = -1;
                    continue;
            }
            struct sockaddr_in * addr = (struct sockaddr_in *)res->ai_addr;
            printf("%s\n", inet_ntoa((struct in_addr)addr->sin_addr));

            // break;  /* okay we got one */
    }
    if (s < 0) {
            // err(1, "%s", cause);
            /*NOTREACHED*/
    }
    freeaddrinfo(res0);

}