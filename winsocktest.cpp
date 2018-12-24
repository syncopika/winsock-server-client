#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

// if using visual studio 
// #pragma comment(lib, "Ws2_32.lib")

// compiling...
// http://www.cplusplus.com/forum/beginner/240684/
// https://ubuntuforums.org/showthread.php?t=441397

// tutorial 
// https://docs.microsoft.com/en-us/windows/desktop/winsock/initializing-winsock

int main(){
	
	WSADATA wsaData;
	
	int iResult;
	
	// intialize winsock 
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if(iResult != 0){
		printf("WSASTartup failed: %d\n", iResult);
		return 1;
	}
	
	printf("initialized socket.\n");
	
	return 0;
}