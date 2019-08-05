// client using winsock2 
#define _WIN32_WINNT 0x0601
#define _WIN32_IE 0x0900
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <limits>
#include <ctime>
#include "resources.h"

#define DEFAULT_PORT 2000
#define DEFAULT_BUFLEN 512

// for improving GUI appearance
#include <commctrl.h> 

// struct to hold socket info, gui window handler 
struct info {
	SOCKET socket;
	HWND hwnd;
};

// register window 
const char g_szClassName[] = "main GUI";
const char g_szClassName2[] = "connection page";
const char g_szClassName3[] = "chat page";

// handler variable for the window 
HWND hwnd;
HWND chatPage;
HWND connectionPage;

// variables for networking
WSADATA wsaData;
int iResult;
//const char* username = "user1";
SOCKET connectSocket;
info guiInfo;
std::string username;

// use Tahoma font for the text 
// this HFONT object needs to be deleted (via DeleteObject) when program ends 
HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
      OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
      DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma")
);

// for inet_pton - https://stackoverflow.com/questions/15660203/inet-pton-identifier-not-found/15660299
// https://stackoverflow.com/questions/1561469/is-there-an-alternative-to-inet-ntop-inetntop-in-windows-xp

// need 1 extra thread for sending messages. 
// https://stackoverflow.com/questions/27610316/winsock-single-client-server-send-and-receive-simultaneously
HANDLE receiveThread;


// to be executed in new thread 
// uhhh need to pass it a HANDLE TO THE GUI WINDOW!!!
DWORD WINAPI receiveMessagesProc(LPVOID lpParam){
	
	HWND window = ((info *)lpParam)->hwnd;
	HWND textBox = GetDlgItem(window, ID_TEXTAREA);
	
	// pass in the socket for this client to receive on  
	SOCKET clientSock = ((info *)lpParam)->socket; //*(SOCKET *)lpParam;
	char recvbuf[DEFAULT_BUFLEN] = {0};
	int recvbuflen = DEFAULT_BUFLEN;
	int rtnVal;
	
	while(1){
		// clear the recv buffer 
		ZeroMemory(recvbuf, recvbuflen);
		rtnVal = recv(clientSock, recvbuf, recvbuflen, 0);
		if(rtnVal > 0){

			//printf("%s\n", recvbuf);
			
			// get curr time
			std::time_t timeNow = std::time(NULL);
			std::tm* ptm = std::localtime(&timeNow);
	
			char buff[32];
			std::strftime(buff, 32, "%d-%m-%Y_%H%M%S", ptm);
			std::string currTime = std::string(buff);
			
			std::string s = currTime + ": " + std::string(recvbuf) + "\n";
			std::cout << "message received from server: " << s << std::endl;
			
			int textLen = GetWindowTextLength(textBox);
			std::cout << "curr text length in text box: " << textLen << std::endl;
			SendMessage(textBox, EM_SETSEL, textLen, textLen); // move pointer to where new text should go 
			SendMessage(textBox, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
			
		}else if(rtnVal == 0){
			printf("connection closed.\n");
			exit(1);
		}else{
			printf("recv failed: %d\n", WSAGetLastError());
			exit(1);
		}
	}
	
	return 0;
}

void createConnectionPage(HWND hwnd, HINSTANCE hInstance){
	// allows user to input IP address to connect to 
	// enter ip address label 
	HWND setIPAddressLabel = CreateWindow(
	    TEXT("STATIC"),
        TEXT("Enter IP address to connect to: "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 45,
        250, 20,
        hwnd,
        NULL,
        hInstance,
        NULL
	);
	SendMessage(setIPAddressLabel, WM_SETFONT, (WPARAM)hFont, true);
	
	// edit box to enter the IP address
	HWND setIPAddressBox = CreateWindow(
		TEXT("edit"),
		TEXT("127.0.0.1"),
		WS_VISIBLE | WS_CHILD | WS_BORDER, 
		200, 42,  /* x, y coords */
		150, 20, /* width, height */
		hwnd,
		(HMENU)ID_SET_IP_ADDRESS,
		hInstance,
		NULL
	);
	SendMessage(setIPAddressBox, WM_SETFONT, (WPARAM)hFont, true);
	
	// enter username label 
	HWND setUsernameLabel = CreateWindow(
	    TEXT("STATIC"),
        TEXT("Enter username: "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 75,
        250, 20,
        hwnd,
        NULL,
        hInstance,
        NULL
	);
	SendMessage(setUsernameLabel, WM_SETFONT, (WPARAM)hFont, true);
	
	// edit box to enter a username (max 15 chars)
	HWND setUsernameBox = CreateWindow(
		TEXT("edit"),
		TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_BORDER, 
		200, 75,  /* x, y coords */
		150, 20, /* width, height */
		hwnd,
		(HMENU)ID_SET_USERNAME,
		hInstance,
		NULL
	);
	SendMessage(setUsernameBox, WM_SETFONT, (WPARAM)hFont, true);
	
	
	// make a button to connect
	HWND connectButton = CreateWindow(
		TEXT("button"),
		TEXT("connect"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        150, 150, // x, y
        60, 20, // width, height
        hwnd,
        (HMENU)ID_CONNECT,
        hInstance,
        NULL
	);
	SendMessage(connectButton, WM_SETFONT, (WPARAM)hFont, true);
	
	// label to display an error connecting message 
	HWND errConnectLabel = CreateWindow(
	    TEXT("STATIC"),
        TEXT(""),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        120, 230,
        250, 20,
        hwnd,
        (HMENU)ID_ERR_CONNECT_LABEL,
        hInstance,
        NULL
	);
	SendMessage(errConnectLabel, WM_SETFONT, (WPARAM)hFont, true);
}

void createChatPage(HWND hwnd, HINSTANCE hInstance){
	
	// text edit area to enter text 
	HWND enterTextBox = CreateWindow(
		TEXT("edit"),
		TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_BORDER, 
		10, 25, 
		280, 20,
		hwnd,
		(HMENU)ID_ENTER_TEXT_BOX,
		hInstance,
		NULL
	);
	SendMessage(enterTextBox, WM_SETFONT, (WPARAM)hFont, true);
	
	//text area to display text
	HWND textArea = CreateWindow(
		TEXT("edit"),
		TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 
		10, 50, 
		280, 350,
		hwnd,
		(HMENU)ID_TEXTAREA,
		hInstance,
		NULL
	);
	SendMessage(textArea, WM_SETFONT, (WPARAM)hFont, true);
	
	// make a button to post text  
	HWND addTextButton = CreateWindow(
		TEXT("button"),
		TEXT("add text"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        300, 50, // x, y
        60, 20, // width, height
        hwnd,
        (HMENU)ID_ADD_TEXT,
        hInstance,
        NULL
	);
	SendMessage(addTextButton, WM_SETFONT, (WPARAM)hFont, true);
	
	// make a button to disconnect
	HWND disconnectButton = CreateWindow(
		TEXT("button"),
		TEXT("disconnect"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        300, 100, // x, y
        80, 20, // width, height
        hwnd,
        (HMENU)ID_DISCONNECT,
        hInstance,
        NULL
	);
	SendMessage(disconnectButton, WM_SETFONT, (WPARAM)hFont, true);
}

// for the chat page 
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    
    switch(msg){
		
        case WM_COMMAND:
		{
            /* LOWORD takes the lower 16 bits of wParam => the element clicked on */
            switch(LOWORD(wParam)){
				
                case ID_ADD_TEXT:
                {
					// get handle to text box 
					// https://stackoverflow.com/questions/23545216/win32-edit-box-displaying-in-new-lines
					//HWND textBox = GetDlgItem(hwnd, ID_TEXTAREA);

					//int textLen = GetWindowTextLength(textBox); //SendMessage(textBox, WM_GETTEXTLENGTH, 0, 0);
					//std::cout << "length: " << textLen << std::endl;
					
					// note that LPCWSTR won't work and will only yield the first character
					// why? does it have to do with casting with L?
					//LPCWSTR newText = L"this is new text\n";
					
					// grab the text in the 'enter text' box
					// post to server
					HWND enterTextBox = GetDlgItem(hwnd, ID_ENTER_TEXT_BOX);
					int textLen = GetWindowTextLength(enterTextBox);
					TCHAR text[textLen + 1]; // +1 for null term 
					GetWindowText(enterTextBox, text, textLen + 1);
					std::string theText = std::string(text);
					std::cout << "text entered: " << theText << std::endl;
					
					// send the text to the server to post to all clients 
					const char *msgBuf = (const char *)theText.c_str();
					send(connectSocket, msgBuf, (int)strlen(msgBuf), 0);
					
					// clear the textbox after sending the msg 
					SetDlgItemText(hwnd, ID_ENTER_TEXT_BOX, "");
                }
                break;
				
				case ID_CONNECT:
				{
					// try to connect to server
					SetDlgItemText(hwnd, ID_ERR_CONNECT_LABEL, "connecting...");
					
					// get the ip addr (need to validate it!)
					HWND ipAddrBox = GetDlgItem(hwnd, ID_SET_IP_ADDRESS);
					TCHAR ipAddr[16];
					GetWindowText(ipAddrBox, ipAddr, sizeof(ipAddr));		
					std::string ip = std::string(ipAddr);
					const char* theIp = ip.c_str();
					//std::cout << "the ip address: " << ip << std::endl;
					
					// also get the username!
					HWND usernameBox = GetDlgItem(hwnd, ID_SET_USERNAME);
					TCHAR user[16];
					GetWindowText(usernameBox, user, ID_SET_USERNAME);
					username = std::string(user);
				
					iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
					if(iResult != 0){
						printf("WSAStartup failed: %d\n", iResult);
						return 1;
					}
					
					// start socket stuff
					connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					if(connectSocket == INVALID_SOCKET){
						printf("socket() failed\n");
						WSACleanup();
						return 1;
					}
					
					struct sockaddr_in servAddr;
					ZeroMemory(&servAddr, sizeof(servAddr));
					servAddr.sin_family = AF_INET;
					
					int size = sizeof(servAddr);
					iResult = WSAStringToAddressA((LPSTR)theIp, AF_INET, NULL, (struct sockaddr *)&servAddr, &size);
					if(iResult != 0){
						printf("getaddrinfo failed: %d\n", iResult);
						WSACleanup();
						
						SetDlgItemText(hwnd, ID_ERR_CONNECT_LABEL, "failed to connect! :(");
					}else{
						servAddr.sin_port = htons(DEFAULT_PORT);
						
						// CONNECT TO THE SERVER
						iResult = connect(connectSocket, (struct sockaddr *)&servAddr, sizeof(servAddr));
						if(iResult == SOCKET_ERROR){
							closesocket(connectSocket);
							connectSocket = INVALID_SOCKET;
							SetDlgItemText(hwnd, ID_ERR_CONNECT_LABEL, "invalid socket error! :(");
						}else{
							printf("was able to connect to server!\n");
							printf("------------------------------\n");
							
							// create thread to receive messages 
							guiInfo.socket = connectSocket;
							
							// refer to the window handler of the chat page (the current hwnd is the connection page)
							guiInfo.hwnd = chatPage;

							receiveThread = CreateThread(NULL, 0, receiveMessagesProc, &guiInfo, 0, 0);
							
							// move on to chat page
							ShowWindow(connectionPage, SW_HIDE);
							ShowWindow(chatPage, SW_SHOW);
							UpdateWindow(hwnd);
							
							// send server initial message (client identifies self)
							std::string helloMsg = "hello:" + username;
							const char *helloMsgBuf = (const char *)helloMsg.c_str();
							send(connectSocket, helloMsgBuf, (int)strlen(helloMsgBuf), 0);	
						}
					}
				}
				break;
				
				case ID_DISCONNECT:
				{
					// close socket connection 
					// terminate receiving thread 
					DWORD exitCode;
					if(GetExitCodeThread(receiveThread, &exitCode) != 0){
						TerminateThread(receiveThread, exitCode);
					}
			
					iResult = shutdown(connectSocket, SD_SEND);
					if(iResult == SOCKET_ERROR){
						printf("shutdown failed: %d\n", WSAGetLastError());
						closesocket(connectSocket);
						WSACleanup();
						PostQuitMessage(0);
					}
					closesocket(connectSocket);
					
					ShowWindow(chatPage, SW_HIDE);
					ShowWindow(connectionPage, SW_SHOW);
					UpdateWindow(hwnd);
					
					SetDlgItemText(connectionPage, ID_ERR_CONNECT_LABEL, "");
				}
				break;
            }
		}
		break;
			
		case WM_CLOSE:
			{
				// gotta close socket here 
				iResult = shutdown(connectSocket, SD_SEND);
				if(iResult == SOCKET_ERROR){
					printf("shutdown failed: %d\n", WSAGetLastError());
					closesocket(connectSocket);
					WSACleanup();
					PostQuitMessage(0);
				}
				closesocket(connectSocket);
				WSACleanup();
				DestroyWindow(hwnd);
			}
			break;
			
		case WM_DESTROY:
			{
				// gotta close socket here 
				iResult = shutdown(connectSocket, SD_SEND);
				if(iResult == SOCKET_ERROR){
					printf("shutdown failed: %d\n", WSAGetLastError());
					closesocket(connectSocket);
					WSACleanup();
					return 1;
				}
				closesocket(connectSocket);
				WSACleanup();
				PostQuitMessage(0);
			}
			break;
		
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){

    /* console attached for debugging */
    //AllocConsole();
    //freopen( "CON", "w", stdout );
	    
    // for improving the gui appearance (buttons, that is. the font needs to be changed separately) 
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);
    
    WNDCLASSEX wc; // this is the main GUI window  
    MSG Msg;
	
	/* make a main window */ 
	wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszMenuName = NULL; 
    wc.lpszClassName = g_szClassName;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	
	// register connection window
	WNDCLASSEX wc2;
    wc2.cbSize = sizeof(WNDCLASSEX);
    wc2.style = 0;
    wc2.lpfnWndProc = WndProc;
    wc2.cbClsExtra = 0;
    wc2.cbWndExtra = 0;
    wc2.hInstance = hInstance;
    wc2.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc2.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc2.lpszMenuName = NULL;
    wc2.lpszClassName = g_szClassName2;
	wc2.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc2.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	
	// register chat window
	WNDCLASSEX wc3;
    wc3.cbSize = sizeof(WNDCLASSEX);
    wc3.style = 0;
    wc3.lpfnWndProc = WndProc;
    wc3.cbClsExtra = 0;
    wc3.cbWndExtra = 0;
    wc3.hInstance = hInstance;
    wc3.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc3.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc3.lpszMenuName = NULL;
    wc3.lpszClassName = g_szClassName3;
	wc3.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc3.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	
    if(!RegisterClassEx(&wc)){
        std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window registration failed for the main GUI!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	
	if(!RegisterClassEx(&wc2)){
        std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window registration failed for the connection page!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	   
	if(!RegisterClassEx(&wc3)){
        std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window registration failed for the chat page!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	
	// create the main window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "chatbox",
        (WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX) & ~WS_MAXIMIZEBOX, // this combo disables maximizing and resizing the window
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 450,
        NULL, NULL, hInstance, NULL
    );
	
	// create the connection page 
	connectionPage = CreateWindowEx(
        WS_EX_WINDOWEDGE,
        g_szClassName2,
        NULL,
        WS_CHILD,
        0, 0, 400, 450,
        hwnd, // parent window
		NULL, 
		hInstance, NULL
    );
	
	// create the chat page 
	chatPage = CreateWindowEx(
        WS_EX_WINDOWEDGE,
        g_szClassName3,
        NULL,
        WS_CHILD,
        0, 0, 400, 450,
        hwnd, // parent window
		NULL, 
		hInstance, NULL
    );
	
    if(hwnd == NULL){
		//std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window creation failed for the main GUI!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	
	if(connectionPage == NULL){
        MessageBox(NULL, "connection page creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	
	if(chatPage == NULL){
        MessageBox(NULL, "chat page failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
	// make and show main GUI window
    ShowWindow(hwnd, nCmdShow); // show the GUI 
    UpdateWindow(hwnd);
	
	createConnectionPage(connectionPage, hInstance);
	createChatPage(chatPage, hInstance);
	
	ShowWindow(connectionPage, SW_SHOW);
	UpdateWindow(hwnd);

    /* message loop */
    while(GetMessage(&Msg, NULL, 0, 0) > 0){
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}













/*
// have a protocol:
// when sending messages, append the user's name to the front of the message?
int main(int argc, char** argv){
	
	// validate args!
	if(argc != 3){
		return 1;
	}
	
	// argv[1] = ip address to connect with 
	// argv[2] = user name 
	
	// validate ip address?
	
	// set some limits on user name 
	// let's make it up to 10 chars 
	std::string nameCheck(argv[2]);
	if(nameCheck.size() > 10){
		printf("sorry, your username is too long.\n");
		return 1;
	}
	
	WSADATA wsaData;
	
	int iResult;
	
	//std::string test = "this is a test";
	//const char *sendbuf = test.c_str();
	const char* username = argv[2];
	
	// intialize winsock 
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if(iResult != 0){
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
	
	
	// start socket stuff
	SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(connectSocket == INVALID_SOCKET){
		printf("socket() failed\n");
		WSACleanup();
		return 1;
	}
	
	struct sockaddr_in servAddr;
	ZeroMemory(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	
	int size = sizeof(servAddr);
	iResult = WSAStringToAddressA(argv[1], AF_INET, NULL, (struct sockaddr *)&servAddr, &size);
	if(iResult != 0){
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	servAddr.sin_port = htons(DEFAULT_PORT);
	
	// CONNECT TO THE SERVER
	iResult = connect(connectSocket, (struct sockaddr *)&servAddr, sizeof(servAddr));
	if(iResult == SOCKET_ERROR){
		closesocket(connectSocket);
		connectSocket = INVALID_SOCKET;
		return 1;
	}else{
		printf("was able to connect to server!\n");
		printf("------------------------------\n");
	}
	
	// create thread to receive messages 
	receiveThread = CreateThread(NULL, 0, receiveMessagesProc, &connectSocket, 0, 0);
	
	// receive data/send data loop 
	// send initial message to server identifying the client 
	std::string helloMsg = "hello:" + std::string(username);
	const char *helloMsgBuf = (const char *)helloMsg.c_str();
	send(connectSocket, helloMsgBuf, (int)strlen(helloMsgBuf), 0);
	
	// should we expect a confirmation???

	// the format for all messages is like this:
	// [IDENTIFIER]:[message]
	do{	
	
		std::string newInputHead = username + std::string(": ");
		std::string newInput;
		std::getline(std::cin, newInput);
		std::cin.clear();
	
		// I would like the user's input to be removed after pressing enter
		// this is complicated though - what if you're typing and a message comes up? the message 
		// will also interfere with what you're typing I think.
		// http://www.cplusplus.com/forum/beginner/147204/ -> probably more helpful than below
		// https://stackoverflow.com/questions/1508490/erase-the-current-printed-console-line
		// better solution: make a win32 gui for this?

		// catch ctrl+c for quit?
		// need to come up with protocol to separate termination messages
		if(newInput == "quit"){
			
			// terminate receiving thread 
			DWORD exitCode;
			if(GetExitCodeThread(receiveThread, &exitCode) != 0){
				TerminateThread(receiveThread, exitCode);
			}
			
			shutdown(connectSocket, SD_SEND);
			closesocket(connectSocket);
			WSACleanup();
			return 0;
		}
		
		newInput = newInputHead + newInput; 
		const char *msg = (const char *)newInput.c_str();
		iResult = send(connectSocket, msg, (int)strlen(msg), 0);
		
	}while(iResult > 0);
	
	// at this point, connection is closed so disconnect
	iResult = shutdown(connectSocket, SD_SEND);
	if(iResult == SOCKET_ERROR){
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}
	
	closesocket(connectSocket);
	WSACleanup();
	
	return 0;
}
*/