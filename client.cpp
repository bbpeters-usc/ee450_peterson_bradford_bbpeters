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

#define SERVERMPORT 25633 // the port client will be connecting to
#define MAXDATASIZE 51 // max number of bytes we can get at once

using namespace std;

int main() {
	int serverMSock, numBytes;
	char buf[MAXDATASIZE];
	struct hostent *he;
	struct sockaddr_in serverMAddr; 
	struct sockaddr_in clientAddr;
	unsigned int clientPort;

	if ((he=gethostbyname("localhost")) == NULL) { // get the host info
		herror("gethostbyname");
		exit(1);
	}
	if ((serverMSock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	serverMAddr.sin_family = AF_INET; // host byte order
	serverMAddr.sin_port = htons(SERVERMPORT); // short, network byte order
	serverMAddr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverMAddr.sin_zero), '\0', 8); // zero the rest of the struct
	if (connect(serverMSock, (struct sockaddr *)&serverMAddr, sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}
	//Retrieve the locally-bound name of the specified socket and store it in the sockaddr structure
	bzero(&clientAddr, sizeof(clientAddr));
	socklen_t len = sizeof(clientAddr);
	getsockname(serverMSock, (struct sockaddr *) &clientAddr, &len);
	clientPort = ntohs(clientAddr.sin_port);

	cout << "The client is up and running" << endl;

	string username, password;
	int n = 2;
	while(1){ //Authentication loop
		cout << "Please enter the username: ";
		getline(cin, username);
		cout << "Please enter the password: ";
		getline(cin, password);

		string msg = username;
		msg.append(MAXDATASIZE-msg.length(), '\0');
		if (send(serverMSock, msg.c_str() , MAXDATASIZE, 0) == -1) {
			perror("send");
		}
		msg = password;
		msg.append(MAXDATASIZE-msg.length(), '\0');
		if (send(serverMSock, msg.c_str() , MAXDATASIZE, 0) == -1) {
			perror("send");
		}
		cout << username << " sent an authentication request to the main server." << endl;
		
		if ((numBytes=recv(serverMSock, buf, 1, 0)) == -1) {
			perror("recv");
			exit(1);
		}
		buf[numBytes] = '\0';

		if(buf[0] == '2'){ 
			cout << username << " received the result of authentication using TCP over port " << clientPort << ". "; 
			cout << "Authentication is successful" << endl;
			break; 
		} else if(n == -1){
			cout << "Authentication Failed for 3 attempts. Client will shut down." << endl;
			exit(0);
		} else if(buf[0] == '0'){
			cout << username << " received the result of authentication using TCP over port " << clientPort << ". "; 
			cout << "Authentication failed: Username Does not exist" << endl;
			cout << "Attempts remaining: " << n << endl;
			n--
		} else {
			cout << username << " received the result of authentication using TCP over port " << clientPort << ". "; 
			cout << "Authentication failed: Password does not match" << endl;
			cout << "Attempts remaining: " << n << endl;
			n--
		}
	}
	
	string course, category;
	while(1){ //query loop
		cout << "Please enter the course code to query: ";
		getline(cin, course);
		cout << "Please enter the category (Credit / Professor / Days / CourseName): ";
		getline(cin, category);
		while((category != "Credit") || (category != "Professor") || (category != "Days") || (category != "CourseName")){
			cout << "Error. " << category << " is not a valid category." << endl;
			cout << "Please enter the category (Credit / Professor / Days / CourseName): ";
			getline(cin, category);
		}


		if (send(serverMSock, course.c_str() , 5, 0) == -1) {
			perror("send");
		}

		category.erase(2, category.length()-1);
		if (send(serverMSock, category.c_str() , 2, 0) == -1) {
			perror("send");
		}
		cout << username << " sent a request to the main server." << endl;

	}
	
	close(serverMSock);
	return 0;
}
