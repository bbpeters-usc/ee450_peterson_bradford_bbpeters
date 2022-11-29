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

#define CSPORT 22633 //my port
#define MPORT 24633 //main server port
#define MAXDATASIZE 51 //max characters sent or recieved

using namespace std;

struct Course { //struct to hold all of the course information
	string code;
	string cred;
	string prof;
	string days;
	string name;
};

void SigchldHandler(int s) //reap dead processes, from beej's guide
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


int main(void) {
	int serverCSSock; //my socket
	struct hostent *he;
	struct sockaddr_in serverCSAddr; // my address information
	struct sockaddr_in serverMAddr; // main servers address information
	socklen_t addrLen;
	struct sigaction sa;
	char buf[MAXDATASIZE];
	int yes=1;

	ifstream file("cs.txt"); //read in course info
	string line; 
	int m = 0;

	while(getline(file,line)){//see how many lines there are
		m++;
	}
	Course courses[m];
	file.close(); //close and reopen to now read in the info
	file.open("cs.txt");
	int n = 0;
	while (getline(file,line)){ //seperate the strings based on where the commas are
		courses[n].code = line.substr(0,line.find(',')); //from https://stackoverflow.com/a/67540651 
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
	file.close();
	
	if ((he=gethostbyname("localhost")) == NULL) { // get the host info
		herror("gethostbyname");
		exit(1);
	}

	//set up my info and my socket, from Beej's guide 2005
	if ((serverCSSock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(serverCSSock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	serverCSAddr.sin_family = AF_INET; // host byte order
	serverCSAddr.sin_port = htons(CSPORT); // short, network byte order
	serverCSAddr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(&(serverCSAddr.sin_zero), '\0', 8); // zero the rest of the struct
	if (bind(serverCSSock, (struct sockaddr *)&serverCSAddr, sizeof(struct sockaddr))==-1){
		perror("bind");
		exit(1);
	}

	//server M's address info, from beej's guide 2005
	serverMAddr.sin_family = AF_INET; // host byte order
	serverMAddr.sin_port = htons(MPORT); // short, network byte order
	serverMAddr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(serverMAddr.sin_zero), '\0', 8); // zero the rest of the struct

	cout << "The ServerCS is up and running using UDP on port " << CSPORT << "." << endl;
	sa.sa_handler = SigchldHandler; // reap all dead processes, from beej's guide 2005
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1) { // main accept() loop
		addrLen = sizeof(struct sockaddr);
		//recieve course and category info, from beej's guide 2005
		if (recvfrom(serverCSSock, buf, 6, 0, (struct sockaddr *)&serverMAddr, &addrLen) == -1) {
			perror("recvfrom");
			exit(1);
		}
		string course = buf;
		if (recvfrom(serverCSSock, buf, 11, 0, (struct sockaddr *)&serverMAddr, &addrLen) == -1) {
			perror("recvfrom");
			exit(1);
		}
		string category = buf;
		
		cout << "The ServerCS received a request from the Main Server about the " << category << " of ";
		cout << course << "." << endl;

		string result = "-1"; //-1 means no course found, otherwise this is the information
		for(int i = 0; i < n; i++){
			if(!course.compare(courses[i].code)){//look for course code
				//then check which category was requested
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
		if(result.compare("-1")){//if the course was found, output the results
			cout << "The course information has been found: The " << category << " of " << course;
			cout << " is " << result << "." << endl;
		} else { cout << "Didn’t find the course: " << course << "." << endl;}//if the course was not found

		//null pad the result and send to the main server
		result.append(MAXDATASIZE-result.length(), '\0');
		if (sendto(serverCSSock, result.c_str(), MAXDATASIZE, 0, (struct sockaddr *)&serverMAddr, addrLen) == -1) {
			perror("sendto");
			exit(1);
		}

		cout << "The ServerCS finished sending the response to the Main Server." << endl;

	}

	close(serverCSSock);
	return 0;
}
