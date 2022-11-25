#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#define PORT 25633 // the port client will be connecting to
#define MAXDATASIZE 50 // max number of bytes we can get at once

using namespace std;

int main() {
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct hostent *he;
	struct sockaddr_in their_addr; // connector’s address information
	if ((he=gethostbyname("localhost")) == NULL) { // get the host info
		herror("gethostbyname");
		exit(1);
	}
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	cout << "The client is up and running" << endl;

	string username, password;
	cout << "Please enter the username: ";
	getline(cin, username);
	cout << "Please enter the password: ";
	getline(cin, password);
	
	string temp = username;
	temp.append(50-temp.length(), '\0');
	temp = password;
	temp.append(50-temp.length(), '\0'); 

	their_addr.sin_family = AF_INET; // host byte order
	their_addr.sin_port = htons(PORT); // short, network byte order
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), '\0', 8); // zero the rest of the struct
	if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}
	string msg = username;
        msg.append(50-msg.length(), '\0');
	if (send(sockfd, msg.c_str() , 50, 0) == -1) {
                                perror("send");
                        }
	msg = password;
        msg.append(50-msg.length(), '\0');
	if (send(sockfd, msg.c_str() , 50, 0) == -1) {
                                perror("send");
                        }
	cout << username << " sent an authentication request to the main server."<< endl;
	if ((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv");
		exit(1);
	}
	buf[numbytes] = '\0';
	printf("Received: %s",buf);
	close(sockfd);
	return 0;
}

