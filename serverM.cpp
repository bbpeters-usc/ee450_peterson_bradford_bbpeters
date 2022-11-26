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
#include <cctype>
#include <cstring>

#define SERVERM_TCP_PORT 25633 // the port the client will be connecting to
#define SERVERM_UDP_PORT 24633 // the port servers will be connecting to
#define SERVERC_PORT 21633
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 51 // max number of bytes we can get at once

using namespace std;

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

char* encryptCred(char* input) {
	char* encrypted = (char*) malloc(strlen(input));
	for(int i=0; i < strlen(input); i++){
		char curr = input[i];
		if(isalpha(curr)){
			if(isupper(curr)){
				curr -= 'A';
				curr += 4;
	                        curr = curr % 26;
				curr += 'A';
			} else {
				curr -= 'a';
				curr += 4;
                                curr = curr % 26;
                                curr += 'a';
			}
		} else if(isdigit(curr)) {
			curr -= '0';
			curr +=4;
			curr = curr % 10;
			curr += '0'; 
		}
		encrypted[i] = curr;
	}
	return encrypted;
}

int main(void) {
	int serverM_TCP_fd, serverM_UDP_fd, client_fd, numbytes; // listen on sock_fd, new connection on client_fd
	struct sockaddr_in serverM_TCP_addr; // my address information
	struct sockaddr_in serverM_UDP_addr;
	struct sockaddr_in client_addr; // connector’s address information
	struct sockaddr_in serverC_addr;
	socklen_t sin_size;
	struct sigaction sa;
	char buf[MAXDATASIZE];
	int yes=1;

	if ((serverM_TCP_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(serverM_TCP_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	serverM_TCP_addr.sin_family = AF_INET; // host byte order
	serverM_TCP_addr.sin_port = htons(SERVERM_TCP_PORT); // short, network byte order
	serverM_TCP_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(serverM_TCP_addr.sin_zero), '\0', 8); // zero the rest of the struct
	if (bind(serverM_TCP_fd, (struct sockaddr *)&serverM_TCP_addr, sizeof(struct sockaddr))==-1){
		perror("bind");
		exit(1);
	}

	if ((serverM_UDP_fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(serverM_UDP_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	serverM_UDP_addr.sin_family = AF_INET; // host byte order
	serverM_UDP_addr.sin_port = htons(SERVERM_UDP_PORT); // short, network byte order
	serverM_UDP_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(serverM_UDP_addr.sin_zero), '\0', 8); // zero the rest of the struct
	if (bind(serverM_UDP_fd, (struct sockaddr *)&serverM_UDP_addr, sizeof(struct sockaddr))==-1){
		perror("bind");
		exit(1);
	}

	serverC_addr.sin_family = AF_INET; // host byte order
	serverC_addr.sin_port = htons(SERVERC_PORT); // short, network byte order
	serverC_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverC_addr.sin_zero), ’\0’, 8); // zero the rest of the struct
	
	if (listen(serverM_TCP_fd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	cout << "The main server is up and running." << endl;

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1) { // main accept() loop
		sin_size = sizeof(struct sockaddr_in);
		if ((client_fd = accept(serverM_TCP_fd, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		if (!fork()) { // this is the child process
			if ((numbytes=recv(client_fd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv");
				exit(1);
			}
			buf[numbytes] = '\0';
			char username[strlen(buf)];
			strcpy(username,buf);

			if ((numbytes=recv(client_fd, buf, MAXDATASIZE-1, 0)) == -1) {
				    perror("recv");
				    exit(1);
			    }
			buf[numbytes] = '\0';
			char password[strlen(buf)];
			strcpy(password,buf);

			cout << "The main server recieved the authentication for " << username << " using TCP over port " << SERVERM_TCP_PORT << "." << endl;
			strcpy(username,encryptCred(username));
			strcpy(password,encryptCred(password));
			
			if ((numbytes = sendto(serverM_UDP_fd, username, 50, 0, (struct sockaddr *)&serverC_addr, sizeof(struct sockaddr))) == -1) {
				perror("sendto");
				exit(1);
			}
			if ((numbytes = sendto(serverM_UDP_fd, password, 50, 0, (struct sockaddr *)&serverC_addr, sizeof(struct sockaddr))) == -1) {
				perror("sendto");
				exit(1);
			}
			cout << "The main server sent an authentication request to serverC." << endl;

			if ((numbytes=recvfrom(serverM_UDP_fd, buf, 1 , 0, (struct sockaddr *)&serverC_addr, &addr_len)) == -1) {
				perror("recvfrom");
				exit(1);
			}
			cout << "The main server received the result of the authentication request from ServerC using UDP over port " << SERVERM_UDP_PORT << "." << endl; 
			
			if (send(client_fd, buf[0], 1, 0) == -1) {
				perror("send");
			}
			cout << "The main server sent the authentication result to the client." << endl;

			close(serverM_UDP_fd);
			close(serverM_TCP_fd);
			close(client_fd);
			exit(0);
		}
		close(client_fd); // parent doesn’t need this

	}
	
	return 0;
}
