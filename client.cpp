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

#define SERVERM_PORT 25633 // the port client will be connecting to
#define MAXDATASIZE 51 // max number of bytes we can get at once

using namespace std;

int main() {
	int serverM_fd, numbytes;
	char buf[MAXDATASIZE];
	struct hostent *he;
	struct sockaddr_in serverM_addr; 
	struct sockaddr_in client_addr;
	unsigned int client_port;

	if ((he=gethostbyname("localhost")) == NULL) { // get the host info
		herror("gethostbyname");
		exit(1);
	}
	if ((serverM_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	serverM_addr.sin_family = AF_INET; // host byte order
	serverM_addr.sin_port = htons(SERVERM_PORT); // short, network byte order
	serverM_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverM_addr.sin_zero), '\0', 8); // zero the rest of the struct
	if (connect(serverM_fd, (struct sockaddr *)&serverM_addr, sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}
	//Retrieve the locally-bound name of the specified socket and store it in the sockaddr structure
	bzero(&client_addr, sizeof(client_addr));
	socklen_t len = sizeof(client_addr);
	getsockname(client_port, (struct sockaddr *) &client_addr, &len);
	client_port = ntohs(client_addr.sin_port);

	cout << "The client is up and running" << endl;

	string username, password;
	int n = 2;
	while(1){ //Authentication loop
		cout << "Please enter the username: ";
		getline(cin, username);
		cout << "Please enter the password: ";
		getline(cin, password);

		string msg = username;
		msg.append(50-msg.length(), '\0');
		if (send(serverM_fd, msg.c_str() , 50, 0) == -1) {
			perror("send");
		}
		msg = password;
		msg.append(50-msg.length(), '\0');
		if (send(serverM_fd, msg.c_str() , 50, 0) == -1) {
			perror("send");
		}
		cout << username << " sent an authentication request to the main server." << endl;
		
		if ((numbytes=recv(serverM_fd, buf, 1, 0)) == -1) {
			perror("recv");
			exit(1);
		}
		buf[numbytes] = '\0';

		if(buf[0] == '2'){ 
			cout << username << " received the result of authentication using TCP over port " << client_port << ". "; 
			cout << "Authentication is successful" << endl;
			break; 
		} else if(n == -1){
			cout << "Authentication Failed for 3 attempts. Client will shut down." << endl;
			exit(0);
		} else if(buf[0] == '0'){
			cout << username << " received the result of authentication using TCP over port " << client_port << ". "; 
			cout << "Authentication failed: Username Does not exist" << endl;
			cout << "Attempts remaining: " << n << endl;
			n--
		} else {
			cout << username << " received the result of authentication using TCP over port " << client_port << ". "; 
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


		if (send(serverM_fd, course.c_str() , 5, 0) == -1) {
			perror("send");
		}

		category.erase(2, category.length()-1);
		if (send(serverM_fd, category.c_str() , 2, 0) == -1) {
			perror("send");
		}
		cout << username << " sent a request to the main server." << endl;

	}
	
	close(serverM_fd);
	return 0;
}
