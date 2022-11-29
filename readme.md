Name: William Bradford Peterson  
ID: 9308415633  
I have completed phases 1 through 4 of the original assignment, but not the extra credit.  

My files:  
Makefile: Generates the executables.  
serverM.cpp: The main server. Serves as the bridge between the client and the end servers, authenticating the login and getting the course queries. Also contains the encyrption algorithm.   
serverC.cpp: The credentials server. Reads cred.txt for the valid username and password combos and stores them, checking against the credentials provided by serverM.  
serverEE.cpp: On of the course query servers, containing information on the EE classes as read from EE.txt.   
serverCS.cpp: The other query server, but this time containing the information on the CS classes.   
client.cpp: The client, where the user interacts with the program. Here login information is entered to be validated and course and category are entered and the relevent information retrieved from the servers.  

Message format: Given that the max username and password could each individually be 50 characters, I set the max data size to be 51 characters, to include a terminating null value assuming a max input of 50 characters. For the user name and password, to ensure consistency, all messages sent are null-padded to be 51 characters. I also pad the course query results to 51 characters, to ensure consistency between sending back a 1 character credits value and a multi character CourseName value.   
For returning authentication results I use a code where 0 means failed because no username, 1 means failed beacause wrong password, and 2 means authentication successful. For the course queries I simply transmit the class code and desired category as a whole between the servers.  
For the course queries, my code makes the follwing assumptions: all course codes are 5 characters, since that is true of the two departments we are working with. There will be no more than 9 units in and class, and no less than 0. No classes will have a name that is more than 50 characters. Categories and class names must be typed exactly correct, being case sensitive.   
I assume that for the input files cred.txt, cs.txt, and ee.txt, that there will be no newline after the final entry, as that may end up compromising my code for reading in those values.  
Except for the case of the client disconnecting due to 3 failed authentication attempts, manually closing the client with Ctrl+C may require a restart of the main server for completely accurate output, as it may lead to junk queries being sent to the CS server. I decided that if a course code was not EE, it would go to the CS server and then be rejected their as not found. In the case where the client closes because of 3 failed authentications, a new client can be reconnected without restarting servers.  
 
Reused code:  
My Makefile code is based on: https://stackoverflow.com/a/9789115   
Much of the networking code in all of my servers and my client is based on the simple stream server, simple stream client, and datagram talker and listener from Beej’s Guide to Network Programming Using Internet Sockets, 2005 edition.  
In serverM.cpp, my EncryptCred function from lines 31 to 57 is based on: https://stackoverflow.com/a/67540651  
In serverC.cpp (lines 54-59), serverEE.cpp (lines 57-68), and serverCS.cpp (lines 57-68), my code to splitting strings based on where is commas are is from: https://stackoverflow.com/a/14266139     
The code to get the dynamic port for client (lines 42-45) in is from Piazza: https://piazza.com/class/l7dlij7ko7v4bv/post/188  
