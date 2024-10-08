/*
** client.c -- a stream socket client demo using TCP
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, determine whether to use IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    // numbytes: To store the number of bytes received from the server.
	int sockfd, numbytes; 
    // buf: Buffer to hold the incoming data from the server
	char buf[MAXDATASIZE];
    // hints: A structure to specify the type of socket we want (initially zeroed out).
    // servinfo: Pointer to the linked list of address information returned by getaddrinfo.
    // p: Pointer used to traverse the linked list of address info.
	struct addrinfo hints, *servinfo, *p;
	int rv;
    // s: Buffer to hold the string representation of the IP address.
	char s[INET6_ADDRSTRLEN];

    // only one argument expected
	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

    // initializing addrinfo structure
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM; // this indicates that we want a TCP socket

    // calls getaddrinfo to convert the hostname into a list of address structures
	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
        // Attempts to connect to the server using connect(). If this fails, 
        // it closes the socket and continues to the next address
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

    // check for successful connection
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

    // Converts the connected address to a human-readable string using inet_ntop 
    // and prints it to the console.
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

    // waits to receive data from the server
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n",buf);

	close(sockfd);

	return 0;
}
