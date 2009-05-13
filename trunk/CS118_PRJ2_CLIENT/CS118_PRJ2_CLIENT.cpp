//============================================================================
// Name        : CS118_PRJ2_CLIENT.cpp
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

void usage(){
	string usage_message = "usage: wcftp server_host server_port [filename] [Pl] [Pc]\ntype: wcftp help for more information\n";
	cout << usage_message;
}
void dumpReadme(){
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
		usage();
	}
}
int main(int argc, char * const argv[]) {
	if(argc>1 &&!strcmp(argv[1],"help")){
		dumpReadme();
		return EXIT_SUCCESS;
	}
	if(argc<3 || argc > 6){
		usage();
		return EXIT_SUCCESS;
	}
	string port = argv[2];
	string host = argv[1];
	for(size_t i=0; i<port.size(); i++){
		if(!isdigit(argv[2][i])){
			usage();
			return EXIT_SUCCESS;
		}
	}

	string requestMessage = "";
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
		requestMessage += "\n";
	}

	 int sockfd;
	 struct addrinfo hints, *servinfo, *p;
	 int rv;
	 int numbytes;

	 memset (&hints, 0, sizeof hints);
	 hints.ai_family = AF_UNSPEC;
	 hints.ai_socktype = SOCK_DGRAM;

	 if ((rv = getaddrinfo(host.c_str(), port.c_str(), &hints, &servinfo)) != 0){
		 cerr << "Failed while getaddrinfo()";
		 return EXIT_FAILURE;
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
		return EXIT_FAILURE;
	}
	if ((numbytes = sendto(sockfd, requestMessage.c_str(), requestMessage.size(), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
