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
#define CPORT 21633
#define EEPORT 23633
#define CSPORT 22633
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 51 // max number of bytes we can get at once

using namespace std;

void SigchldHandler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

string EncryptCred(string input) {
	string encrypted = input;
	for(int i=0; i < input.length(); i++){
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
	int tcpSock, udpSock, client; // listen on sock_fd, new connection on client
	struct hostent *he;
	struct sockaddr_in tcpAddr; // my address information
	struct sockaddr_in udpAddr;
	struct sockaddr_in clientAddr; // connector’s address information
	struct sockaddr_in serverCAddr;
	struct sockaddr_in serverEEAddr;
	struct sockaddr_in serverCSAddr;
	socklen_t sinSize;
	socklen_t addrLen;
	struct sigaction sa;
	char buf[MAXDATASIZE];
	int yes=1;

	if ((he=gethostbyname("localhost")) == NULL) { // get the host info
		herror("gethostbyname");
		exit(1);
	}

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

	serverCAddr.sin_family = AF_INET; // host byte order
	serverCAddr.sin_port = htons(CPORT); // short, network byte order
	serverCAddr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverCAddr.sin_zero), '\0', 8); // zero the rest of the struct

	serverEEAddr.sin_family = AF_INET; // host byte order
	serverEEAddr.sin_port = htons(EEPORT); // short, network byte order
	serverEEAddr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverEEAddr.sin_zero), '\0', 8); // zero the rest of the struct
	
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
		if ((client = accept(tcpSock, (struct sockaddr *)&clientAddr, &sinSize)) == -1) {
			perror("accept");
			continue;
		}
		if (!fork()) { // this is the child process
			string username, password;
			int n = 2;
			while(1) { //authentication loop
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

				string encryptedUser = EncryptCred(username);
				encryptedUser.append(MAXDATASIZE-encryptedUser.length(), '\0');
				string encryptedPass = EncryptCred(password);
				encryptedPass.append(MAXDATASIZE-encryptedPass.length(), '\0');
				
				if (sendto(udpSock, encryptedUser.c_str(), MAXDATASIZE, 0, (struct sockaddr *)&serverCAddr, addrLen) == -1) {
					perror("sendto");
					exit(1);
				}
				if (sendto(udpSock, encryptedPass.c_str(), MAXDATASIZE, 0, (struct sockaddr *)&serverCAddr, addrLen) == -1) {
					perror("sendto");
					exit(1);
				}
				cout << "The main server sent an authentication request to serverC." << endl;

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
				
				if(buf[0] == '2') { break; }
				else { n--; }
				/*if(n == 2){

				}*/
			}

			while(1){//query loop
				if ((client, buf, 6, 0) == -1) {
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

				struct sockaddr_in addr;
				string server;
				if(course[0] == 'E') { 
					addr = serverEEAddr;
					server = "EE"; 
				}
				else { 
					addr = serverCSAddr;
					server = "CS"; 
				}
				if (sendto(udpSock, course.c_str(), 6, 0, (struct sockaddr *)&addr, addrLen) == -1) {
					perror("sendto");
					exit(1);
				}
				category.append(11-category.length(), '\0');
				if (sendto(udpSock, category.c_str(), 11, 0, (struct sockaddr *)&addr, addrLen) == -1) {
					perror("sendto");
					exit(1);
				}
				cout << "The main server sent an authentication request to server" << server << "." << endl;

				if (recvfrom(udpSock, buf, MAXDATASIZE, 0, (struct sockaddr *)&addr, &addrLen) == -1) {
					perror("recvfrom");
					exit(1);
				}

				cout << "The main server received the response from Server" << server << " using UDP over port "; 
				cout << UDPPORT << "." << endl;

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
