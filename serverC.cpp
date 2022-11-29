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

#define CPORT 21633 //serverC port
#define MPORT 24633 //serverM port
#define MAXDATASIZE 51 // max number of bytes we can get at once

using namespace std;

struct Cred {//struct to hold the username and password
	string user;
	string pass;
};

void SigchldHandler(int s) //from Beej's Guide 2005, to reap dead processes
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


int main(void) {
	int serverCSock; //serverC socket
	struct hostent *he;
	struct sockaddr_in serverCAddr; // serverC address information
	struct sockaddr_in serverMAddr; // main server's address information
	socklen_t addrLen;
	struct sigaction sa;
	char buf[MAXDATASIZE];
	int yes=1;

	ifstream file("cred.txt");//read in the cred file
	string line; 
	int m = 0;

	while(getline(file,line)){//to find the number of lines in the cred file
		m++;
	}

	Cred entries[m];
	file.close();
	//reopen the file and now read in the values
	file.open("cred.txt");
	int n = 0;
	while (getline(file,line)){ //code to split strings from https://stackoverflow.com/a/14266139 
		entries[n].user = line.substr(0,line.find(',')); //read until comma
		line.erase(0, line.find(',') + 1); //delete the already read portion
		if(n<(m-1)){ //delete the last invisible character on all lines except the last
			line.erase(line.length()-1);
		}
		entries[n].pass = line;
		n++;
	}
	
	if ((he=gethostbyname("localhost")) == NULL) { // get the host info
		herror("gethostbyname");
		exit(1);
	}

	//set up the serverC socket and address info
	//from Beej's Guide 2005 edition
	if ((serverCSock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(serverCSock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	serverCAddr.sin_family = AF_INET; // host byte order
	serverCAddr.sin_port = htons(CPORT); // short, network byte order
	serverCAddr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(serverCAddr.sin_zero), '\0', 8); // zero the rest of the struct
	if (bind(serverCSock, (struct sockaddr *)&serverCAddr, sizeof(struct sockaddr))==-1){
		perror("bind");
		exit(1);
	}

	//set up the ServerM address info
	//from beej's guide 2005 edition
	serverMAddr.sin_family = AF_INET; // host byte order
	serverMAddr.sin_port = htons(MPORT); // short, network byte order
	serverMAddr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverMAddr.sin_zero), '\0', 8); // zero the rest of the struct

	cout << "The ServerC is up and running using UDP on port " << CPORT << "." << endl;
	sa.sa_handler = SigchldHandler; // reap all dead processes, from Beej's guide 2005 edition
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1) { // main loop to perform authentication
		addrLen = sizeof(struct sockaddr);
		//receive the username and password, from Beej's guide 2005 ed
		if (recvfrom(serverCSock, buf, MAXDATASIZE, 0, (struct sockaddr *)&serverMAddr, &addrLen) == -1) {
			perror("recvfrom");
			exit(1);
		}
		string username = buf;
		if (recvfrom(serverCSock, buf, MAXDATASIZE, 0, (struct sockaddr *)&serverMAddr, &addrLen) == -1) {
			perror("recvfrom");
			exit(1);
		}
		string password = buf;

		cout << "The ServerC received an authentication request from the Main Server." << endl;
		
		string auth = "0"; //0 means no username match, 1 means wrong password, 2 means correct credentials
		for(int i = 0; i < n; i++){
			if(!username.compare(entries[i].user)){//check for username match
				if(!password.compare(entries[i].pass)){ auth = "2"; } //check for password match
				else{ auth = "1"; }
				break;
			}
		}
		//send result to main server, code based on Beej's guide 2005 ed
		if (sendto(serverCSock, auth.c_str(), 2, 0, (struct sockaddr *)&serverMAddr, addrLen) == -1) {
			perror("sendto");
			exit(1);
		}
		cout << "The ServerC finished sending the response to the Main Server." << endl;

	}
	
	return 0;
}
