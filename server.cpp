// server using winsock2

#define _WIN32_WINNT 0x501 // https://stackoverflow.com/questions/36420044/winsock-server-and-client-c-code-getaddrinfo-was-not-declared-in-this-scope

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cassert>
#include <stdio.h>
#include <vector>

#define DEFAULT_PORT 2000 // for some reason 27015 doesn't work!? (even though it's used in the microsoft tutorial)
#define DEFAULT_BUFLEN 512

// if using visual studio 
// #pragma comment(lib, "Ws2_32.lib")

// on compiling...
// http://www.cplusplus.com/forum/beginner/240684/
// https://ubuntuforums.org/showthread.php?t=441397

// tutorial 
// https://docs.microsoft.com/en-us/windows/desktop/winsock/initializing-winsock

// allow this server to accept multiple connections using select
// https://stackoverflow.com/questions/36470645/c-socket-recv-and-send-simultaneously
// https://stackoverflow.com/questions/8674491/server-with-multiple-clients-writing-with-select
// https://www.gnu.org/software/libc/manual/html_node/Server-Example.html#Server-Example

// keep array of sockets 
std::vector<SOCKET> socketsArray;

SOCKET constructSocket(){
	SOCKET sockRtnVal = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sockRtnVal == INVALID_SOCKET){
		printf("socket() failed\n");
		WSACleanup();
		return 1;
	}
	return sockRtnVal;
}


int main(void){
	
	WSADATA wsaData;
	
	int iResult;
	int iSendResult;
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
	
	// intialize winsock 
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if(iResult != 0){
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
	
	// create socket for server
	serverSocket = constructSocket();
	assert(serverSocket != 1);
	
	// set up server address 
	ZeroMemory(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(DEFAULT_PORT);
	
	// bind the server socket to the address 
	iResult = bind(serverSocket, (struct sockaddr *)&servAddr, sizeof(servAddr));
	if(iResult == SOCKET_ERROR){
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	// listen on the socket for connections
	if(listen(serverSocket, SOMAXCONN) == SOCKET_ERROR){
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}
	
	// initialize active socket set (and add the server socket to the set)
	FD_ZERO(&activeFdSet);
	FD_SET(serverSocket, &activeFdSet);
	socketsArray.push_back(serverSocket);
	
	// accept client connections 
	printf("waiting for connections...\n");
	
	while(1){

		// block until input comes in 
		readFdSet = activeFdSet;
		int returnval = select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL);
		if(returnval == SOCKET_ERROR){
			printf("error with select\n: %d", WSAGetLastError());
			return 1;
		}
		
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
					socketsArray.push_back(clientSocket);
				}else{
					// handle communication between already established client
					// clear the recv buffer first 
					ZeroMemory(recvbuf, sizeof(recvbuflen));
					
					// get the new data 
					iResult = recv(currSocketFd, recvbuf, recvbuflen, 0);

					if(iResult > 0){
						printf("Bytes received by server: %d\n", iResult);
						printf("Message received by server: %s\n", recvbuf);
						
						// send message back to all connected clients 
						// skip first index since that's the server socket (used only for accepting new clients)
						// https://stackoverflow.com/questions/11532311/winsock-send-always-returns-error-10057-in-server
						for(int i = 1; i < (int)socketsArray.size(); i++){
							SOCKET sockFd = (SOCKET)socketsArray.at(i);
							iSendResult = send(sockFd, recvbuf, iResult, 0);
							if(iSendResult == SOCKET_ERROR){
								printf("send failed: %d\n", WSAGetLastError());
								closesocket(sockFd);
								WSACleanup();
								return 1;
							}
						}
						
						//printf("Bytes sent back to client: %d\n", iSendResult);	
					}else if(iResult == 0){
						printf("connection with socket %d closing...\n", currSocketFd);
								
						// if you reach this point, connection needs to be shutdown 
						iResult = shutdown(currSocketFd, SD_SEND);
						if(iResult == SOCKET_ERROR){
							printf("shutdown failed with error: %d\n", WSAGetLastError());
							closesocket(currSocketFd);
							WSACleanup();
							return 1;
						}
						closesocket(currSocketFd);
						FD_CLR(currSocketFd, &activeFdSet);
						// need to remove the socket from the sockets array!
						std::vector<SOCKET> newSocketArray;
						for(auto sock: socketsArray){
							if((SOCKET)sock != currSocketFd){
								newSocketArray.push_back((SOCKET)sock);
							}
						}
						socketsArray = newSocketArray;
					}else{
						printf("recv failed: %d\n", WSAGetLastError());
						closesocket(currSocketFd);
						FD_CLR(currSocketFd, &activeFdSet);
						WSACleanup();
						return 1;
					}
					printf("\n");
				}	
			}
		}
		
	}
	WSACleanup();
	return 0;
}