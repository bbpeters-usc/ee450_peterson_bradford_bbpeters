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

#define SERVERMPORT 25633 // the main server port
#define MAXDATASIZE 51 // max number of bytes we can send or receive at once

using namespace std;

int main() {
	int serverMSock;
	char buf[MAXDATASIZE];
	struct hostent *he;
	struct sockaddr_in serverMAddr; 
	struct sockaddr_in clientAddr;
	unsigned int clientPort;

	if ((he=gethostbyname("localhost")) == NULL) { // get the host info
		herror("gethostbyname");
		exit(1);
	}

	//setting up the infromation to connect to the main server's socket
	//from Beej's guide 2005
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

	//From Piazza post 188
	//Retrieve the locally-bound name of the specified socket and store it in the sockaddr structure
	bzero(&clientAddr, sizeof(clientAddr));
	socklen_t len = sizeof(clientAddr);
	getsockname(serverMSock, (struct sockaddr *) &clientAddr, &len);
	clientPort = ntohs(clientAddr.sin_port);

	cout << "The client is up and running." << endl;

	string username, password, msg;
	int n = 2;//number of authentication tries after the current one
	while(1){ //Authentication loop
		//reading in the username and password
		cout << "Please enter the username: ";
		getline(cin, username);
		cout << "Please enter the password: ";
		getline(cin, password);

		//null-padding the username and password to 51 characters before sending them
		//send()code from Beej's Guide 2005.
		msg = username;
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
		
		//Recieving the information on how the authentication went, from Beej's Guide 2005
		if (recv(serverMSock, buf, 2, 0) == -1) {
			perror("recv");
			exit(1);
		}

		//Interpreting the authentication reuslt
		//2 means successful, 1 means wrong password, 0 means username not found
		if(buf[0] == '2'){ 
			cout << username << " received the result of authentication using TCP over port " << clientPort << ". "; 
			cout << "Authentication is successful" << endl;
			break; 
		} else if(buf[0] == '0'){
			cout << username << " received the result of authentication using TCP over port " << clientPort << ". "; 
			cout << "Authentication failed: Username Does not exist" << endl;
			cout << "Attempts remaining: " << n << endl;
			n--;//decrementing number of remaining attempts
		} else if(buf[0] == '1'){
			cout << username << " received the result of authentication using TCP over port " << clientPort << ". "; 
			cout << "Authentication failed: Password does not match" << endl;
			cout << "Attempts remaining: " << n << endl;
			n--;//decrementing after failed attept
		}
		if(n == -1){//once 3 attempts have been made, client shuts down
			cout << "Authentication Failed for 3 attempts. Client will shut down." << endl;
			close(serverMSock);
			exit(0);
		}
	}
	
	string course, category;
	while(1){ //course query loop
		//reading in course code and which category with user wants info on
		cout << "Please enter the course code to query: ";
		getline(cin, course);
		cout << "Please enter the category (Credit / Professor / Days / CourseName): ";
		getline(cin, category);
		//if the category is not typed exactly correct, prompts the user to try again until it is correct
		while((category.compare("Credit")) && (category.compare("Professor")) && (category.compare("Days")) && (category.compare("CourseName"))){
			cout << "Error. " << category << " is not a valid category." << endl;
			cout << "Please enter the category (Credit / Professor / Days / CourseName): ";
			getline(cin, category);
		}
		
		//null padding and sending the course and categorys wanted
		//send() code from Beej's Guide 2005 ed
		msg = course;
		msg.append(6-msg.length(), '\0');
		if (send(serverMSock, course.c_str() , 6, 0) == -1) {
			perror("send");
		}
		msg = category;						//longest category, CourseName, is 10 characters
		msg.append(11-msg.length(), '\0');	//so I null pad to 11
		if (send(serverMSock, category.c_str() , 11, 0) == -1) {
			perror("send");
		}
		cout << username << " sent a request to the main server." << endl;

		//recieving the query response
		//recv() code from Beej's Guide 2005 ed
		if (recv(serverMSock, buf, MAXDATASIZE, 0) == -1) {
			perror("recv");
			exit(1);
		}
		string result = buf;
		cout << "The client received the response from the Main server using TCP over port " << clientPort << ". " << endl;

		//displaying the result
		if(!result.compare("-1")){ //a result of -1 means that the course was not found
			cout << "Didn’t find the course: " << course << "." << endl;
		} else{
			cout << "The " << category << " of " << course << " is " << result << "." << endl;
		}
		cout << endl;
		cout << "-----Start a new request-----" << endl;
	}
	
	close(serverMSock);
	return 0;
}
