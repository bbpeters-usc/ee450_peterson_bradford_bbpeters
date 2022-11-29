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

/*
For comments on lines and sections, please check serverCS.cpp as it is functionally identical to this one	
*/
#define EEPORT 23633 
#define MPORT 24633
#define MAXDATASIZE 51

using namespace std;

struct Course {
	string code;
	string cred;
	string prof;
	string days;
	string name;
};

void SigchldHandler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


int main(void) {
	int serverEESock;
	struct hostent *he;
	struct sockaddr_in serverEEAddr; // my address information
	struct sockaddr_in serverMAddr; // connector’s address information
	socklen_t addrLen;
	struct sigaction sa;
	char buf[MAXDATASIZE];
	int yes=1;

	ifstream file("ee.txt");
	string line; 
	int m = 0;

	while(getline(file,line)){
		m++;
	}
	Course courses[m];
	file.close();
	file.open("ee.txt");

	int n = 0;
	while (getline(file,line)){//from https://stackoverflow.com/a/67540651
		courses[n].code = line.substr(0,line.find(','));
		line.erase(0, line.find(',') + 1);
		courses[n].cred = line.substr(0,line.find(','));
		line.erase(0, line.find(',') + 1);
		courses[n].prof = line.substr(0,line.find(','));
		line.erase(0, line.find(',') + 1);
		courses[n].days = line.substr(0,line.find(','));
		line.erase(0, line.find(',') + 1);
		if(n<(m-1)){
                        line.erase(line.length()-1);
                }
		courses[n].name = line;
		n++;
	}
	
	if ((he=gethostbyname("localhost")) == NULL) { // get the host info
		herror("gethostbyname");
		exit(1);
	}

	if ((serverEESock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(serverEESock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	serverEEAddr.sin_family = AF_INET; // host byte order
	serverEEAddr.sin_port = htons(EEPORT); // short, network byte order
	serverEEAddr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(serverEEAddr.sin_zero), '\0', 8); // zero the rest of the struct
	if (bind(serverEESock, (struct sockaddr *)&serverEEAddr, sizeof(struct sockaddr))==-1){
		perror("bind");
		exit(1);
	}

	serverMAddr.sin_family = AF_INET; // host byte order
	serverMAddr.sin_port = htons(MPORT); // short, network byte order
	serverMAddr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverMAddr.sin_zero), '\0', 8); // zero the rest of the struct

	cout << "The ServerEE is up and running using UDP on port " << EEPORT << "." << endl;
	sa.sa_handler = SigchldHandler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1) { // main accept() loop
		addrLen = sizeof(struct sockaddr);
		if (recvfrom(serverEESock, buf, 6, 0, (struct sockaddr *)&serverMAddr, &addrLen) == -1) {
			perror("recvfrom");
			exit(1);
		}
		string course = buf;

		if (recvfrom(serverEESock, buf, 11, 0, (struct sockaddr *)&serverMAddr, &addrLen) == -1) {
			perror("recvfrom");
			exit(1);
		}
		string category = buf;

		cout << "The ServerEE received a request from the Main Server about the " << category << " of ";
		cout << course << "." << endl;

		string result = "-1"; //-1 means no course found, otherwise this is the information
		for(int i = 0; i < n; i++){
			if(!course.compare(courses[i].code)){
				if(!category.compare("Credit")) {
					result = courses[i].cred;
				} else if(!category.compare("Professor")){
					result = courses[i].prof;
				} else if(!category.compare("Days")){
					result = courses[i].days;
				} else {
					result = courses[i].name;
				}
				break;
			}
		}
		if(result.compare("-1")){
			cout << "The course information has been found: The " << category << " of " << course;
			cout << " is " << result <<  "." << endl;
		} else { cout << "Didn’t find the course: " << course << "." << endl;}

		result.append(MAXDATASIZE-result.length(), '\0');
		if (sendto(serverEESock, result.c_str(), MAXDATASIZE, 0, (struct sockaddr *)&serverMAddr, addrLen) == -1) {
			perror("sendto");
			exit(1);
		}

		cout << "The ServerEE finished sending the response to the Main Server." << endl;

	}
	
	close(serverEESock);
	return 0;
}
