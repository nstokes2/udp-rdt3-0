//============================================================================
// Name        : wcclient.cpp
// Author      : Akhil Rangaraj, Justin Kiang
// Version     :
// Copyright   : Copyright 2009 White Castle Development Inc.
// Description : Hello World in C, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fstream>
#include <netdb.h>
#include <iostream>
#include <string.h>
#include <ctype.h>
#include "config.h"

using namespace std;

#define STARTPORT 10000 	// beginning of port pool we can use
#define ENDPORT 65535	// end of port pool we can use
#define MMS 1500


/*
 * WCPACKET
 * fields:
 * wc_seqnum: sequence number
 * size: data size
 * md5sum: packet md5
 * data: file data;
 */

#define MAXPACKETDATA	1024 //packet can hold 1024 bytes.

struct wcpacket_t
{
	int seqnum;
	int flag;
	//some kind of shit
	char data[MAXPACKETDATA];
	int size;
};

/*
 * wcpacket_t* recvpacket(int sockfd)
 * Get a packet from the server
 * sockfd - sock that we're listening on
 * Return the filled out packet, or  NULL if there is an error
 */
wcpacket_t* recvpacket(int sockfd)
{
	if(!sockfd)
		return NULL;
	
	wcpacket_t* recvd = new wcpacket_t;
	if(recvfrom(sockfd, recvd, sizeof(wcpacket_t), 0,NULL, 0) <= 0)
	{
		perror("recv failure");
		delete recvd;
		return NULL;
	}
	return recvd;
	
}

/*
 * void send_packet(int sockfd, int sqn)
 * Creates an ACK packet to send
 * sockfd, socket that we're sending to
 * sqn - sequence number
 * returns filled out packet
 */
void send_packet(int sockfd, int sqn, addrinfo* out)
{
	wcpacket_t* new_packet = new wcpacket_t;
	new_packet->seqnum = sqn;
	sendto(sockfd, new_packet, sizeof(wcpacket_t), 0, out->ai_addr, out->ai_addrlen);
}

void usage(){
	ifstream readme ("README");
	string line = "";
	if(readme.is_open()){
		while (!readme.eof()){
			getline (readme,line);
			cout << line << endl;
		}
		readme.close();
	}
	else{
		//WTF WHY HAVE A SEPARATE FX FOR USAGE
		cout << "usage: wcftp server_host server_port [filename] [Pl] [Pc]\ntype: wcftp help for more information\n";
	}
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
	 if ((sockfd = socket(p->ai_family, p->ai_socktype,
			 p->ai_protocol)) == -1) {
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
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
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
		fprintf(stderr, "listener: failed to bind socket\n");
	}
	freeaddrinfo(servinfo);
	return p;
}
int main(int argc, char * const argv[]) {
	if(argc>1 &&!strcmp(argv[1],"help")){
		usage();
		return EXIT_SUCCESS;
	}
	if(argc<3 || argc > 6){
		usage();
		return EXIT_SUCCESS;
	}
	string requestMessage = "";
	string host = argv[1];
	string port = argv[2];
	for(size_t i=0; i<port.size(); i++){
		if(!isdigit(argv[2][i])){
			usage();
			return EXIT_SUCCESS;
		}
	}
	srand ( time(NULL) );
	int incomingPort = rand() %(ENDPORT-STARTPORT) + STARTPORT;
	char requestBuff[50];
	char portBuf[6];
	sprintf(portBuf,"%d",incomingPort);
	sprintf(requestBuff,"INIT %d",incomingPort);
	requestMessage = requestBuff;
	requestMessage += "\n\n\0";
	int requestSock;
	int incomingSock;
	struct addrinfo *out, *in;
	out = createOutgoingSocket(host.c_str(),port.c_str(),requestSock);
	in = createReceivingSocket(portBuf,incomingSock);
	cout << requestMessage;
	if (sendto(requestSock, requestMessage.c_str(), requestMessage.size()+2, 0, out->ai_addr, out->ai_addrlen) == -1) {
		perror("talker: sendto");
		return EXIT_FAILURE;
	}
    char buf[MMS];

	if (recvfrom(incomingSock, buf, MMS-1 , 0,
		NULL, 0) == -1) {
		perror("recvfrom");
		exit(1);
	}
	cout << "SERVER SAYS: " << buf;
	char command[5];
	for (size_t i=0; i<5; i++){
		command[i] = buf[i];
	}
	char ackport[6];
	command[4]='\0';
	if(!strcmp(command,"GAHD")){
		for (size_t i=5; i<10; i++){
			ackport[i-5] = buf[i];
		}
		ackport[5]='\0';
	}
	out = createOutgoingSocket(host.c_str(),ackport,requestSock);


	requestMessage = "";
	if(argc>3){
		string filename = argv[3];
		requestMessage = "RETR " + filename + "\n\n";
		cout << requestMessage;
	}
	else{
		string requestBuffer;
		do{
			getline(cin,requestBuffer);
			if(requestBuffer!=""){
				requestMessage = requestMessage + requestBuffer + "\n";
			}
		}while(requestBuffer!="");
		requestMessage += "\n\0";
	}

	if (sendto(requestSock, requestMessage.c_str(), requestMessage.size()+1, 0,
			 out->ai_addr, out->ai_addrlen) == -1) {
		perror("talker: sendto");
		return EXIT_FAILURE;
	}
	wcpacket_t* incpacket;
	while((incpacket = recvpacket(incomingSock)) != NULL) {
		
		cout << "SERVER SAYS: packet #" << incpacket->seqnum << "\n\n";//<<incpacket->data <<"\n\n";
		//immediately send ack for the packet
		send_packet(requestSock, incpacket->seqnum, out);
		delete incpacket;
	}
	return EXIT_SUCCESS;
}
