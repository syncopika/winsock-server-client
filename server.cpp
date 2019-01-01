// server using winsock2

#define _WIN32_WINNT 0x501 // https://stackoverflow.com/questions/36420044/winsock-server-and-client-c-code-getaddrinfo-was-not-declared-in-this-scope

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unordered_map>
#include <iostream>

#define DEFAULT_PORT 2000 // for some reason 27015 doesn't work!? (even though it's used in the microsoft tutorial)
#define DEFAULT_BUFLEN 512

// if using visual studio 
// #pragma comment(lib, "Ws2_32.lib")

// winsock 
// https://tangentsoft.net/wskfaq/articles/othersys.html

// on compiling...
// http://www.cplusplus.com/forum/beginner/240684/
// https://ubuntuforums.org/showthread.php?t=441397

// tutorial 
// https://docs.microsoft.com/en-us/windows/desktop/winsock/initializing-winsock
// https://www.winsocketdotnetworkprogramming.com/winsock2programming/winsock2advancediomethod5a.html
// https://tangentsoft.net/wskfaq/articles/passing-sockets.html

// allow this server to accept multiple connections using select
// https://stackoverflow.com/questions/36470645/c-socket-recv-and-send-simultaneously
// https://stackoverflow.com/questions/8674491/server-with-multiple-clients-writing-with-select
// https://www.gnu.org/software/libc/manual/html_node/Server-Example.html#Server-Example

SOCKET constructSocket(){
	SOCKET sockRtnVal = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sockRtnVal == INVALID_SOCKET){
		printf("socket() failed\n");
		WSACleanup();
		exit(1);
	}
	return sockRtnVal;
}

// checks for SOCKET_ERROR 
// this error can come up when trying to bind, listen, send 
void socketErrorCheck(int returnValue, SOCKET socketToClose, const char* action){
	const char *actionAttempted = action;
	if(returnValue == SOCKET_ERROR){
		printf("socket error. %s failed with error: %d\n", actionAttempted, WSAGetLastError());
	}else{
		// everything's fine, probably...
		return;
	}
	closesocket(socketToClose);
	WSACleanup();
	exit(1);
}


int main(void){
	
	WSADATA wsaData;
	
	int iResult;
	int sendResult;
	char recvbuf[DEFAULT_BUFLEN] = {0};
	int recvbuflen = DEFAULT_BUFLEN;
	
	SOCKET serverSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;
	
	struct sockaddr_in servAddr;
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen;
	
	// for select() 
	fd_set activeFdSet;
	fd_set readFdSet;
	
	// keep array of sockets 
	std::vector<SOCKET> socketsArray;
	
	// map socket fds to usernames
	std::unordered_map<SOCKET, std::string> socketToUserMap;
	
	// intialize winsock 
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if(iResult != 0){
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
	
	// create socket for server
	serverSocket = constructSocket();
	//assert(serverSocket != 1);
	
	// set up server address 
	ZeroMemory(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(DEFAULT_PORT);
	
	// bind the server socket to the address 
	iResult = bind(serverSocket, (struct sockaddr *)&servAddr, sizeof(servAddr));
	socketErrorCheck(iResult, serverSocket, "bind");

	// listen on the socket for connections
	iResult = listen(serverSocket, SOMAXCONN);
	socketErrorCheck(iResult, serverSocket, "listen");

	// initialize active socket set (and add the server socket to the set)
	FD_ZERO(&activeFdSet);
	FD_SET(serverSocket, &activeFdSet);
	socketsArray.push_back(serverSocket);
	
	// accept client connections 
	printf("waiting for connections...\n");
	
	// keep new empty vector in case we need to update socketsArray (i.e. client disconnects)
	std::vector<SOCKET> newSocketArray(socketsArray);
	
	while(1){

		// block until input comes in 
		readFdSet = activeFdSet;
		iResult = select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL);
		socketErrorCheck(iResult, serverSocket, "select");
		
		// service all the sockets that have input! 
		for(int i = 0; i < (int)socketsArray.size(); i++){
		
			// grab the socket fd at this index 
			SOCKET currSocketFd = socketsArray.at(i);
			
			if(FD_ISSET(currSocketFd, &readFdSet)){
				// if active socket, check if it's a new connection or one with data 	
				if(currSocketFd == serverSocket){
					// this is a new connection 
					clientAddrLen = sizeof(clientAddr);
					clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
					if(clientSocket == INVALID_SOCKET){
						printf("accept failed with error: %d\n", WSAGetLastError());
						closesocket(clientSocket);
						WSACleanup();
						return 1;
					}else{
						printf("got a connection!\n");
					}
					// add them to the set
					FD_SET(clientSocket, &activeFdSet);
					
					// update the sockets array 
					newSocketArray.push_back(clientSocket);
				}else{
					// handle communication between already established client
					// clear the recv buffer first 
					ZeroMemory(recvbuf, recvbuflen);
					
					// get the new data 
					iResult = recv(currSocketFd, recvbuf, recvbuflen, 0);
					
					if(iResult > 0){
						// https://stackoverflow.com/questions/53094542/socket-programming-in-c-when-client-exit-server-the-server-is-crashed
						printf("Bytes received by server: %d\n", iResult);
						printf("Message received by server: %s\n", recvbuf);
						
						// check for initial message sent by client, which should identify themself via a username 
						// then add the name to the socketToUserMap
						// then broadcast to all users the new user 
						std::string msg(recvbuf);
						
						// the format for all messages is like this:
						// [IDENTIFIER]:[message]
						// where IDENTIFIER can be either "hello" (initial message from client to server) or "msg" (any messages from client after the initial)
						// so we should find the index of the first colon, take the substring from 0 to that index, and check the string 
						std::string identifier = msg.substr(0, msg.find(":"));
						std::string restOfMsg = msg.substr(msg.find(":") + 1, strlen(msg.c_str()));
						
						if(identifier == "hello"){
							std::string newClientJoinedMsg = restOfMsg + " has joined the server!";
							
							// add new user to map (restOfMSg in this case is the username)
							socketToUserMap.insert( std::pair<SOCKET, std::string>(currSocketFd, restOfMsg) );
							
							const char *msgToSend = (const char *)newClientJoinedMsg.c_str();
							for(int j = 1; j < (int)newSocketArray.size(); j++){
								if(clientSocket != (SOCKET)newSocketArray.at(j)){
									SOCKET sockFd = (SOCKET)newSocketArray.at(j);
									sendResult = send(sockFd, msgToSend, (int)strlen(msgToSend), 0);
									socketErrorCheck(sendResult, sockFd, "send");
								}
							}
						}else{
							// send message back to all connected clients 
							// skip first index since that's the server socket (used only for accepting new clients)
							// https://stackoverflow.com/questions/11532311/winsock-send-always-returns-error-10057-in-server
							for(int j = 1; j < (int)newSocketArray.size(); j++){
								SOCKET sockFd = (SOCKET)newSocketArray.at(j);
								sendResult = send(sockFd, recvbuf, iResult, 0);
								socketErrorCheck(sendResult, sockFd, "send");
							}
						}
						
						ZeroMemory(recvbuf, recvbuflen);
			
					}else if(iResult == 0){
						// note that iResult needs to be 0 in order for a connection to close. 
						printf("connection with socket %d closing (user: %s)...\n", currSocketFd, (const char *)socketToUserMap[currSocketFd].c_str());
								
						// if you reach this point, connection needs to be shutdown 
						iResult = shutdown(currSocketFd, SD_SEND);
						socketErrorCheck(iResult, currSocketFd, "shutdown");
						
						closesocket(currSocketFd);
						FD_CLR(currSocketFd, &activeFdSet);
						
						// remove socket from map  
						socketToUserMap.erase(currSocketFd);
						
						// need to remove the socket from the sockets array!
						// use the new socket array to record changes. 
						// TODO: also tell everyone that this user left the server 
						std::vector<SOCKET> tempArr; 
						for(auto sock: newSocketArray){
							if((SOCKET)sock != currSocketFd){
								tempArr.push_back((SOCKET)sock);
							}
						}
						newSocketArray.assign(tempArr.begin(), tempArr.end());
					}else{
						printf("recv failed: %d\n", WSAGetLastError());
						closesocket(currSocketFd);
						//FD_CLR(currSocketFd, &activeFdSet);
						WSACleanup();
						return 1;
					}
					printf("\n");
				}	
			}
		}
		
		// update sockets array so that the next loop will use this updated array 
		socketsArray.assign(newSocketArray.begin(), newSocketArray.end());
		
	}
	WSACleanup();
	return 0;
}