//============================================================================
// Name        : CS118_PRJ2_SERVER.cpp
// Author      : Akhil Rangaraj, Justin Kiang
// Version     :
// Copyright   : Copyright 2009 White Castle Development Inc.
// Description : Hello World in C, Ansi-style
//============================================================================
/*
** listener.c -- a datagram sockets "server" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <string.h>

using namespace std;

#define MYPORT "6969"    // the port users will be connecting to
#define STARTPORT 10000 	// beginning of port pool we can use
#define ENDPORT 65535	// end of port pool we can use
#define MMS 1400

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
addrinfo* createOutgoingSocket(const char* host, const char* port, int& sockfd){
	struct addrinfo hints, *servinfo, *p;
	int rv;
	memset (&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0){
		cerr << "Failed while getaddrinfo()\n";
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}
	for(p = servinfo; p != NULL; p = p->ai_next) {
	if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
		perror("talker: socket");
		continue;
	}
	 break;
	}
	if (p == NULL) {
		cerr << "Failed to create socket";
	}
	return p;
}
addrinfo* createReceivingSocket(const char* port, int& sockfd){
	struct addrinfo hints, *servinfo, *p;
	int rv;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}
	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		cerr << "Failed to bind incoming socket\n";
	}
	freeaddrinfo(servinfo);
	return p;
}
int main(void)
{
    int recv_sockfd;
    struct addrinfo *p;
    struct sockaddr_storage remote_addr;
    size_t addr_len;
    char s[INET6_ADDRSTRLEN];
    p = createReceivingSocket(MYPORT,recv_sockfd);

    pid_t child;
    cout << "Listening on port " << MYPORT << "\nWaiting for incoming connections\n";
    char buf[MMS];
    while(1){
		// Listen on announcement socket for INIT requests
		addr_len = sizeof remote_addr;
		recvfrom(recv_sockfd, buf, MMS-1 , 0, (struct sockaddr *)&remote_addr, (socklen_t *)&addr_len);
		if(!(child=fork())){
			close(recv_sockfd);
			char command[5];
			for (size_t i=0; i<5; i++){
				command[i] = buf[i];
			}
			char port[6];
			char portBuf[6];
			struct addrinfo *out, *in;
			int requestSock;
			int incomingSock;
			command[4]='\0';
			string remoteAddr = inet_ntop(remote_addr.ss_family, get_in_addr((struct sockaddr *)&remote_addr), s, sizeof s);
			if(!strcmp(command,"INIT")){
				for (size_t i=5; i<10; i++){
					port[i-5] = buf[i];
				}
				port[5]='\0';
				srand ( time(NULL) );
				int incomingPort = rand() %(ENDPORT-STARTPORT) + STARTPORT + 1;
				sprintf(portBuf,"%d",incomingPort);
				out = createOutgoingSocket(remoteAddr.c_str(),port,requestSock); // Create outbound socket for sending files
				in = createReceivingSocket(portBuf,incomingSock); // Create inbound socket for receiving requests and acs
				string returnBuf = portBuf;
				returnBuf = "GAHD " + returnBuf;
				returnBuf += "\n\n";
				sendto(requestSock, returnBuf.c_str(), returnBuf.size()+2, 0, out->ai_addr, out->ai_addrlen);
			}
			else{
				exit(0);
			}
			printf("New session from %s\n",remoteAddr.c_str());
			printf("\tOutbound port: %s\n\tInbound(ACK) port: %s\n",port, portBuf);
			char buf2[MMS];
			recvfrom(incomingSock, buf2, MMS-1 , 0, NULL, 0);
			for (size_t i=0; i<5; i++){
				command[i] = buf2[i];
			}
			command[4]='\0';
			if(!strcmp(command,"QUIT")){
				printf("Session from %s closed\n",remoteAddr.c_str());
				close(incomingSock);
				close(requestSock);
			}
			else if(!strcmp(command,"RETR")){
				char filename[MMS];
				for(size_t i=5; i<strlen(buf2) && buf2[i]!='\n'; i++){
					filename[i-5] = buf2[i];
				}
				cout << "\t" << remoteAddr << " asks for " << filename << "\n";
				FILE* content = fopen(filename, "r");
				if(!content){
					cout << "File " << filename << " not found";
				}
				else{
					long size;
					fseek(content, 0, SEEK_END);
					size = ftell(content);
					rewind(content);
					char* filebuffer = (char*)malloc(sizeof(char*)*size);
					fread(filebuffer, 1, size, content);
					fclose(content);
					cout << "X" << size <<"X";
					sendto(requestSock, filebuffer, size, 0, out->ai_addr, out->ai_addrlen);
				}
				// REQUEST TO RETRIEVE FILE
			}


			exit(0);
		}
    }
    close(recv_sockfd);

    return 0;
}
