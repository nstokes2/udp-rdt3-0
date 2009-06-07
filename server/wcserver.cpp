//============================================================================
// Name        : wcserver.cpp
// Author      : Akhil Rangaraj, Justin Kiang
// Version     :
// Copyright   : Copyright 2009 White Castle Development Inc.
// Description : Hello World in C, Ansi-style
//============================================================================


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
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <queue>
#include  <fcntl.h>
#include <iostream>
#include <fstream>
using namespace std;

#define MYPORT "6969"    // the port users will be connecting to
#define STARTPORT 10000 	// beginning of port pool we can use
#define ENDPORT 65535	// end of port pool we can use
#define MMS 1400

#define WINDOWSIZE 5 //size of dong window
pid_t child;
/*
 * WCPACKET
 * fields:
 * wc_seqnum: sequence number
 * flag: TBD
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
 * packet_t* create_packet(ifstream* fp, int sequence)
 * Create a wcpacket out of file data with sequence number sequence
 * fp - pointer to file
 * sequence - sequence number
 * returns pointer to packet, or NULL on failure
 */

wcpacket_t* create_packet(void* is, int sequence)
{
	
	iostream* fp = (iostream*)is;
	if(!fp)
	{
		cout << "FILE IS DEAD" <<endl;
		return NULL;
	}
	wcpacket_t* new_packet = new wcpacket_t;
	memset(new_packet->data, 0, MAXPACKETDATA);
	fp->read(new_packet->data, MAXPACKETDATA);
	//cout << "packet: "<< new_packet->seqnum << "\n data: \n";
	//printf("%s\n", new_packet->data);
	new_packet->seqnum = sequence;
	return new_packet;
}

wcpacket_t* recv_packet(int isockfd)
{
	if(!isockfd)
		return NULL;
	
	wcpacket_t* recvd = new wcpacket_t;
	if(recvfrom(isockfd, recvd, sizeof(wcpacket_t), 0,NULL, 0) <= 0)
	{
		perror("recv failure");
		delete recvd;
		return NULL;
	}
	return recvd;
	
}
								  
/*
 * PTHREAD MAGICKS
 *
 */

/*
* void handle_sigchld(int signal)
* signal handler for SIGCHLD (child has exited)
* waits on child process and cleans up
*/
void handle_sigchld(int signal)
{
	
		int status;
		waitpid(child, &status, WNOHANG);
		cout << "Child process " << child <<" exited with exit status " << WEXITSTATUS(status)<< endl;
}

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
    
    /* wait for our children*/
    struct sigaction sigchld_action; 
	memset (&sigchld_action, 0, sizeof (sigchld_action)); 
	sigchld_action.sa_handler = &handle_sigchld; 
	sigchld_action.sa_flags = SA_RESTART;
	sigaction (SIGCHLD, &sigchld_action, NULL);  //handler for SIGCHLD
	
	
	
    p = createReceivingSocket(MYPORT,recv_sockfd);

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
				ifstream content;
				content.open(filename);
				if(!content){
					cout << "File " << filename << " not found";
				}
				else{
					cout << "I AM PREPARING TO SEND"<<endl;
					int i=0;
					int active_packets = 0;
					long sentdata = 0;
					//get file size so things don't explode
					//things still explode anyways.
				//	fcntl(incomingSock, F_SETFL, O_NONBLOCK); // set our recv sock to not block
					queue < wcpacket_t* > active_window;
					while(content.good() || active_window.size() > 0){
						while(active_window.size() < WINDOWSIZE && content.good())
						{
							wcpacket_t* send = create_packet(&content, i++); //create a packet out of our file
							active_window.push(send);
							sentdata+= send->size;
							if(active_window.size() == WINDOWSIZE || !content.good()){
								queue < wcpacket_t* > temp_window;
								for(wcpacket_t* curr=active_window.front(); !active_window.empty(); curr=active_window.front()){
									sendto(requestSock, curr, sizeof(wcpacket_t), 0, out->ai_addr, out->ai_addrlen);
									temp_window.push(curr);
									active_window.pop();
								}
								for(wcpacket_t* curr=temp_window.front(); !temp_window.empty(); curr=temp_window.front()){
									active_window.push(curr);
									temp_window.pop();
								}
								i=0;
							}
							
						}
						//ok, recieve ack
						wcpacket_t* recv;
						recv = recv_packet(incomingSock);
						if(recv)
						{
							cout << "ACK # "<< recv->seqnum<<endl;
							
							if(recv->seqnum != active_window.back()->seqnum)
							{
								cout << active_window.size();
								cout << "Wrong ack";
								queue < wcpacket_t* > temp_window;
								for(wcpacket_t* curr=active_window.front(); !active_window.empty(); curr=active_window.front()){
									sendto(requestSock, curr, sizeof(wcpacket_t), 0, out->ai_addr, out->ai_addrlen);
									temp_window.push(curr);
									active_window.pop();
								}
								for(wcpacket_t* curr=temp_window.front(); !temp_window.empty(); curr=temp_window.front()){
									active_window.push(curr);
									temp_window.pop();
								}		
							}
// 							if(recv->seqnum != active_window.front()->seqnum)
// 							{
// 								cout << "resending packet: "<<active_window.front()->seqnum <<endl;
// 								//we recieved an out of order ack, assume shit sux and resend
// 								sendto(requestSock, active_window.front(), sizeof(wcpacket_t), 0, out->ai_addr, out->ai_addrlen);
// 							}
							else
							{
								cout << active_window.size();
								cout << "Correct ack";
								while(!active_window.empty())
									active_window.pop();
							}
							delete recv;
						}
					}
					
				}
				// REQUEST TO RETRIEVE FILE
			}

			close(incomingSock);
			close(requestSock);
			exit(9001);
		}
    }
    close(recv_sockfd);

    return 0;
}
