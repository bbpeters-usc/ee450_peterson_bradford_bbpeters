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

#define SERVERC_PORT 21633; 
#define SERVERM_PORT 24633;
#define MAXDATASIZE 51 // max number of bytes we can get at once

using namespace std;

struct cred {
	string user;
	string pass;
};

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


int main(void) {
	int serverC_fd;
	struct sockaddr_in serverC_addr; // my address information
	struct sockaddr_in serverM_addr; // connector’s address information
	socklen_t sin_size;
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
	cred entries[n];
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

	if ((serverC_fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(serverC_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	serverC_addr.sin_family = AF_INET; // host byte order
	serverC_addr.sin_port = htons(SERVERM_PORT); // short, network byte order
	serverC_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(serverC_addr.sin_zero), '\0', 8); // zero the rest of the struct
	if (bind(serverC_fd, (struct sockaddr *)&serverC_addr, sizeof(struct sockaddr))==-1){
		perror("bind");
		exit(1);
	}

	serverM_addr.sin_family = AF_INET; // host byte order
	serverM_addr.sin_port = htons(SERVERC_PORT); // short, network byte order
	serverM_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverM_addr.sin_zero), ’\0’, 8); // zero the rest of the struct

	cout << "The ServerC is up and running using UDP on port " << SERVERC_PORT << ". << endl;
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	addr_len = sizeof(struct sockaddr);

	while(1) { // main accept() loop
		if ((numbytes=recvfrom(serverC_fd, buf, MAXDATASIZE-1 , 0, (struct sockaddr *)&serverM_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		cout << "The ServerC received an authentication request from the Main Server." << endl;
		buf[numbytes] = '\0';
		char username[strlen(buf)];
		strcpy(username,buf);

		if ((numbytes=recvfrom(serverC_fd, buf, MAXDATASIZE-1 , 0, (struct sockaddr *)&serverM_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		buf[numbytes] = '\0';
		char password[strlen(buf)];
		strcpy(password,buf);

		char auth = '0'; //0 means no username match, 1 means wrong password, 2 means correct credentials
		for(int i = 0; i < n; i++){
			if(username == entries[i].user.c_str()){
				if(password == entries[i].pass.c_str()){ auth = '2'; }
				else{ auth = '1'; }
				break;
			}
		}
		if ((numbytes = sendto(serverC_fd, auth, 1, 0, (struct sockaddr *)&serverM_addr, sizeof(struct sockaddr))) == -1) {
			perror("sendto");
			exit(1);
		}
		cout << "The ServerC finished sending the response to the Main Server." << endl;

	}
	
	return 0;
}
