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

#define TCPPORT 25633 // serverM's TCP port
#define UDPPORT 24633 // serverM's UDP port
#define CPORT 21633 // server C's port
#define EEPORT 23633 // serverEE's port
#define CSPORT 22633 //serverCS's port
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 51 // max number of bytes we can get at once

using namespace std;

void SigchldHandler(int s) //reap dead processes, from Beej's guide 2005 ed
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

string EncryptCred(string input) { //encryption function, based on https://stackoverflow.com/a/67540651 
	string encrypted = input; //create a string to hold the output
	for(int i=0; i < input.length(); i++){
		char curr = input[i];
		if(isalpha(curr)){//check if alphabetical
			if(isupper(curr)){//check if uppercase, then offset by 4
				curr -= 'A';
				curr += 4;
				curr = curr % 26;
				curr += 'A';
			} else { //this means its lower case letter
				curr -= 'a';
				curr += 4;
				curr = curr % 26;
				curr += 'a';
			}
		} else if(isdigit(curr)) {//check if its a digit
			curr -= '0';
			curr +=4;
			curr = curr % 10;
			curr += '0'; 
		}
		//do nothing to spaces and special characters
		encrypted[i] = curr;//fill this with the encrypted results
	}
	return encrypted;
}

int main(void) {
	//many of these intializations from Beej's guide 2005
	int tcpSock, udpSock, client; // listen on sock_fd, new connection on client
	struct hostent *he;
	struct sockaddr_in tcpAddr; // TCP address info
	struct sockaddr_in udpAddr; // UDP address info
	struct sockaddr_in clientAddr; // client's address information
	struct sockaddr_in serverCAddr; //serverC info
	struct sockaddr_in serverEEAddr; //server EE info
	struct sockaddr_in serverCSAddr; //serverCS info
	socklen_t sinSize;
	socklen_t addrLen;
	struct sigaction sa;
	char buf[MAXDATASIZE];
	int yes=1;

	if ((he=gethostbyname("localhost")) == NULL) { // get the host info (Beej's Guide 2005)
		herror("gethostbyname");
		exit(1);
	}

	// set up the TCP socket and address info, based on Beej's guide 2005 ed
	if ((tcpSock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(tcpSock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	tcpAddr.sin_family = AF_INET; // host byte order
	tcpAddr.sin_port = htons(TCPPORT); // short, network byte order
	tcpAddr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(tcpAddr.sin_zero), '\0', 8); // zero the rest of the struct
	if (bind(tcpSock, (struct sockaddr *)&tcpAddr, sizeof(struct sockaddr))==-1){
		perror("bind");
		exit(1);
	}

	// set up the UDP socket and address info, based on Beej's guide 2005
	if ((udpSock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(udpSock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	udpAddr.sin_family = AF_INET; // host byte order
	udpAddr.sin_port = htons(UDPPORT); // short, network byte order
	udpAddr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(udpAddr.sin_zero), '\0', 8); // zero the rest of the struct
	if (bind(udpSock, (struct sockaddr *)&udpAddr, sizeof(struct sockaddr))==-1){
		perror("bind");
		exit(1);
	}

	//set up the Server C address info
	//this and below from Beej's guide 2005 ed
	serverCAddr.sin_family = AF_INET; // host byte order
	serverCAddr.sin_port = htons(CPORT); // short, network byte order
	serverCAddr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverCAddr.sin_zero), '\0', 8); // zero the rest of the struct

	//set up the serverEE address info
	serverEEAddr.sin_family = AF_INET; // host byte order
	serverEEAddr.sin_port = htons(EEPORT); // short, network byte order
	serverEEAddr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverEEAddr.sin_zero), '\0', 8); // zero the rest of the struct
	
	//set up the server CS address info
	serverCSAddr.sin_family = AF_INET; // host byte order
	serverCSAddr.sin_port = htons(CSPORT); // short, network byte order
	serverCSAddr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverCSAddr.sin_zero), '\0', 8); // zero the rest of the struct

	if (listen(tcpSock, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	cout << "The main server is up and running." << endl;

	sa.sa_handler = SigchldHandler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1) { // main accept() loop
		sinSize = sizeof(struct sockaddr_in);
		addrLen = sizeof(struct sockaddr);
		//connect to the client on TCP and fork a child process, from Beej's guide 2005
		if ((client = accept(tcpSock, (struct sockaddr *)&clientAddr, &sinSize)) == -1) {
			perror("accept");
			continue;
		}
		if (!fork()) { // this is the child process
			string username, password;
			int n = 2; //measuring authentication attemts
			while(1) { //authentication loop
				//read in username and password, from Beej's guide 2005
				if (recv(client, buf, MAXDATASIZE, 0) == -1) {
					perror("recv");
					exit(1);
				}
				username = buf;
				if (recv(client, buf, MAXDATASIZE, 0) == -1) {
					perror("recv");
					exit(1);
				}
				password = buf;

				cout << "The main server recieved the authentication for " << username;
				cout << " using TCP over port " << TCPPORT << "." << endl;

				//encrypt the username and password and null-pad to send to the cred server
				string encryptedUser = EncryptCred(username);
				encryptedUser.append(MAXDATASIZE-encryptedUser.length(), '\0');
				string encryptedPass = EncryptCred(password);
				encryptedPass.append(MAXDATASIZE-encryptedPass.length(), '\0');
				
				//sending to cred server over UDP, from Beej's guide 2005
				if (sendto(udpSock, encryptedUser.c_str(), MAXDATASIZE, 0, (struct sockaddr *)&serverCAddr, addrLen) == -1) {
					perror("sendto");
					exit(1);
				}
				if (sendto(udpSock, encryptedPass.c_str(), MAXDATASIZE, 0, (struct sockaddr *)&serverCAddr, addrLen) == -1) {
					perror("sendto");
					exit(1);
				}
				cout << "The main server sent an authentication request to serverC." << endl;

				//recieveing the result from cred server over UDP, from Beej's Guide 2005
				if (recvfrom(udpSock, buf, 2, 0, (struct sockaddr *)&serverCAddr, &addrLen) == -1) {
					perror("recvfrom");
					exit(1);
				}
				cout << "The main server received the result of the authentication request from ServerC using UDP over port "; 
				cout << UDPPORT << "." << endl; 
				
				if (send(client, buf, 1, 0) == -1) {
					perror("send");
				}
				cout << "The main server sent the authentication result to the client." << endl;
				
				if(buf[0] == '2') { break; } //if the authentication was successful, break out of the loop
				else { n--; }
				if(n == -1){ break; } //if the client's terminated from failure, break out of the loop
			}
			if(n == -1){ continue; } //if client terminated from failed auth, skip query phase

			while(1){//query loop
				//get course info and category from client over TCP
				//from beej's guide 2005
				if (recv(client, buf, 6, 0) == -1) {
					perror("recv");
					exit(1);
				}
				string course = buf;
				if (recv(client, buf, 11, 0) == -1) {
					perror("recv");
					exit(1);
				}
				string category = buf;
				cout << "The main server received from " << username << " to query course " << course;
				cout << " about " << category << " using TCP over port " << TCPPORT << "." << endl;

				//variables to hold the address and name of which of the two servers we will be asking
				struct sockaddr_in addr;
				string server;
				if(course.front() == 'E') { //if it starts with an E, send it to EE
					addr = serverEEAddr;
					server = "EE"; 
				}
				else {  //otherwise send it to CS
					addr = serverCSAddr;
					server = "CS"; 
				}

				//null pad and send the course and category over UDP, from Beej's guide
				course.append(6-course.length(), '\0');
				if (sendto(udpSock, course.c_str(), 6, 0, (struct sockaddr *)&addr, addrLen) == -1) {
					perror("sendto");
					exit(1);
				}
				category.append(11-category.length(), '\0');
				if (sendto(udpSock, category.c_str(), 11, 0, (struct sockaddr *)&addr, addrLen) == -1) {
					perror("sendto");
					exit(1);
				}
				cout << "The main server sent a request to server" << server << "." << endl;

				//receive the information requested
				if (recvfrom(udpSock, buf, MAXDATASIZE, 0, (struct sockaddr *)&addr, &addrLen) == -1) {
					perror("recvfrom");
					exit(1);
				}

				cout << "The main server received the response from Server" << server << " using UDP over port "; 
				cout << UDPPORT << "." << endl;

				//send the result to the client
				if (send(client, buf, MAXDATASIZE, 0) == -1) {
					perror("send");
				}

				cout << "The main server sent the query information to the client." << endl;
			}

			close(udpSock);
			close(tcpSock);
			close(client);
			exit(0);
		}
		close(client); // parent doesn’t need this

	}
	
	return 0;
}
