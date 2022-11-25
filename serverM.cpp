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

#define TCPPORT 25633 // the port the client will be connecting to
#define UDPPORT 24633 // the port servers will be connecting to
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
	int sockfd, new_fd, numbytes; // listen on sock_fd, new connection on new_fd
	struct sockaddr_in my_addr; // my address information
	struct sockaddr_in their_addr; // connector’s address information
	socklen_t sin_size;
	struct sigaction sa;
	char buf[MAXDATASIZE];
	int yes=1;
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	my_addr.sin_family = AF_INET; // host byte order
	my_addr.sin_port = htons(TCPPORT); // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))==-1){
		perror("bind");
		exit(1);
	}
	if (listen(sockfd, BACKLOG) == -1) {
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
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
	
		if ((numbytes=recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
            		perror("recv");
            		exit(1);
		}
		buf[numbytes] = '\0';
		char username[strlen(buf)];
		strcpy(username,buf);

		if ((numbytes=recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                            perror("recv");
                            exit(1);
                    }
		buf[numbytes] = '\0';
        	char password[strlen(buf)];
		strcpy(password,buf);

		cout << "The main server recieved the authentication for " << username << " using TCP over port " << TCPPORT << "." << endl;
		strcpy(username,encryptCred(username));
		strcpy(password,encryptCred(password));

		if (send(new_fd, "Hello, world!\n", 14, 0) == -1) {
			perror("send");
		}
		close(sockfd);
		close(new_fd); // parent doesn’t need this

	}
	
	return 0;
}
