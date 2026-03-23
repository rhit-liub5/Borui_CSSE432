/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 1025 // max number of bytes we can get at once 

bool readLine(char** line, size_t* size, size_t* length);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 3) {
	    fprintf(stderr,"usage: client hostname port\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(sockfd == -1) {
			perror("client: socket");
			continue;
		}

		rv = connect(sockfd, p->ai_addr, p->ai_addrlen);

		if(rv == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if(p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	char* line = NULL;
	size_t size = 0;
	size_t len;

	while(readLine(&line, &size, &len))
	{
		if(send(sockfd, line, len, 0) == -1)
		{
			perror("send");
			exit(1);
		}
	}

	close(sockfd);

	return 0;
}

bool readLine(char** line, size_t* size, size_t* length)
{
	while (1)
	{
		printf("Enter a line: ");
		size_t len = getline(line, size, stdin);

		if (len == -1){
			return false;
		}

		if(*(line)[len-1] == '\n')
		{
			(*line)[--len] = '\0';
		}
		*length = len;

		if(len == 0){
			continue;
		}

		return len > 1 || **line != '.';

	}
}
