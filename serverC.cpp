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
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <cstring>

#define SERVERCPORT 21633 
#define SERVERMPORT 24633
#define MAXDATASIZE 51 // max number of bytes we can get at once

using namespace std;

struct Cred {
	string user;
	string pass;
};

void SigchldHandler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


int main(void) {
	int serverCSock;
	struct hostent *he;
	struct sockaddr_in serverCAddr; // my address information
	struct sockaddr_in serverMAddr; // connector’s address information
	socklen_t addrLen;
	struct sigaction sa;
	char buf[MAXDATASIZE];
	int numbytes;
	int yes=1;

	ifstream file("cred.txt");
	string line; 
	int n = 0;

	while(getline(file,line)){
		n++;
	}
	Cred entries[n];
	file.close();
	file.open("cred.txt");
	n = 0;
	char c;
	while (getline(file,line)){
	    entries[n].user = line.substr(0,line.find(','));
	    line.erase(0, line.find(',') + 1);
	    entries[n].pass = line;
		n++;
	}
	
	if ((he=gethostbyname("localhost")) == NULL) { // get the host info
		herror("gethostbyname");
		exit(1);
	}

	if ((serverCSock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(serverCSock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	serverCAddr.sin_family = AF_INET; // host byte order
	serverCAddr.sin_port = htons(SERVERCPORT); // short, network byte order
	serverCAddr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(serverCAddr.sin_zero), '\0', 8); // zero the rest of the struct
	if (bind(serverCSock, (struct sockaddr *)&serverCAddr, sizeof(struct sockaddr))==-1){
		perror("bind");
		exit(1);
	}

	serverMAddr.sin_family = AF_INET; // host byte order
	serverMAddr.sin_port = htons(SERVERMPORT); // short, network byte order
	serverMAddr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverMAddr.sin_zero), '\0', 8); // zero the rest of the struct

	cout << "The ServerC is up and running using UDP on port " << SERVERCPORT << "." << endl;
	sa.sa_handler = SigchldHandler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1) { // main accept() loop
		addrLen = sizeof(struct sockaddr);
		if ((numbytes=recvfrom(serverCSock, buf, MAXDATASIZE, 0, (struct sockaddr *)&serverMAddr, &addrLen)) == -1) {
			perror("recvfrom");
			exit(1);
		}

		buf[numbytes-1] = '\0';
		string username = buf;

		if ((numbytes=recvfrom(serverCSock, buf, MAXDATASIZE, 0, (struct sockaddr *)&serverMAddr, &addrLen)) == -1) {
			buf[numbytes] = '\0';
			perror("recvfrom");
			exit(1);
		}
		buf[numbytes-1] = '\0';
		string password = buf;

		cout << "The ServerC received an authentication request from the Main Server." << endl;
		
		string auth = "0"; //0 means no username match, 1 means wrong password, 2 means correct credentials
		for(int i = 0; i < n; i++){
			if(!username.compare(entries[i].user)){
				if(!password.compare(entries[i].pass)){ auth = "2"; }
				else{ auth = "1"; }
				break;
			}
		}
		if ((numbytes = sendto(serverCSock, auth.c_str(), 2, 0, (struct sockaddr *)&serverMAddr, addrLen)) == -1) {
			perror("sendto");
			exit(1);
		}
		cout << "The ServerC finished sending the response to the Main Server." << endl;

	}
	
	return 0;
}
