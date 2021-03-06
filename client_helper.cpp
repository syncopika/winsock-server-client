#include "headers/client_helper.hh"

// variables for networking
WSADATA wsaData;
int iResult;
info guiInfo;
std::wstring username;
SOCKET connectSocket;
HANDLE receiveThread;
//char helloBuf[DEFAULT_BUFLEN] = {0}; 

// window handlers for the pages
HWND chatPage;
HWND connectionPage;

// wndproc variable for subclassing 
WNDPROC prevProc;

// use Tahoma font for the text 
// this HFONT object needs to be deleted (via DeleteObject) when program ends 
HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
      OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
      DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma")
);


std::wstring getWideStringFromString(std::string string){	
	int sz = MultiByteToWideChar(CP_UTF8, 0, &string[0], -1, NULL, 0);
	std::wstring res(sz, 0);
	MultiByteToWideChar(CP_UTF8, 0, &string[0], -1, &res[0], sz);
	return res;
}

std::string getStringFromWideString(std::wstring wstring){
	int sz = WideCharToMultiByte(CP_UTF8, 0, &wstring[0], -1, 0, 0, 0, 0);
	std::string res(sz, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstring[0], -1, &res[0], sz, 0, 0);
	return res;
}


// to be executed in new thread
DWORD WINAPI receiveMessagesProc(LPVOID lpParam){
	
	HWND window = ((info *)lpParam)->hwnd; // the main gui window to post msgs to 
	HWND textBox = GetDlgItem(window, ID_TEXTAREA);
	
	// pass in the socket for this client to receive on  
	SOCKET clientSock = ((info *)lpParam)->socket;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	int rtnVal;
	
	while(1){
		// clear the recv buffer 
		ZeroMemory(recvbuf, recvbuflen);
		rtnVal = recv(clientSock, (char *)recvbuf, recvbuflen, 0);
		if(rtnVal > 0){	
			// get curr time
			std::time_t timeNow = std::time(NULL);
			std::tm* ptm = std::localtime(&timeNow);
	
			WCHAR buff[64];
			std::wcsftime(buff, 64, L"%m-%d-%Y_%H:%M:%S", ptm);
			std::wstring currTime = std::wstring(buff);
			
			std::string multiByteMsg = std::string(recvbuf);
			std::wstring res = getWideStringFromString(multiByteMsg);
			//std::wcout << L"msg from server: " << res << std::endl;
			printf("Message received from server: %s\n", recvbuf);
			
			std::wstring s = L"\r\n" + currTime + L" " + res + L"\r\n";
			//std::wcout << L"message received from server: " << s << std::endl;
			
			int textLen = GetWindowTextLengthW(textBox);
			std::wcout << L"curr text length in text box: " << textLen << "\n" << std::endl;
			
			SendMessageW(textBox, EM_SETSEL, textLen, textLen); // move pointer to where new text should go 
			SendMessageW(textBox, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
			
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
	HWND setUsernameBox = CreateWindowW(
		TEXT(L"edit"),
		TEXT(L""),
		WS_VISIBLE | WS_CHILD | WS_BORDER, 
		200, 75,  /* x, y coords */
		150, 20, /* width, height */
		hwnd,
		(HMENU)ID_SET_USERNAME,
		hInstance,
		NULL
	);
	SendMessageW(setUsernameBox, WM_SETFONT, (WPARAM)hFont, true);
	
	// make a button to connect
	HWND connectButton = CreateWindow(
		TEXT("button"),
		TEXT("connect"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        160, 150, // x, y
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

/*
	function to allow submitting message with enter key 
*/
LRESULT CALLBACK msgEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	switch(msg){
		case WM_CHAR:
		{
			// have to do this to suppress beep after enter key press
			switch(wParam){
				case VK_RETURN:
				{
					return 0;
				}
			}
		}
		break; 
				
		case WM_KEYDOWN:
		{
			switch(wParam){
				case VK_RETURN:
				{
					// get this edit box's parent window handle (so we can pass it a message)
					HWND parentWindow = (HWND)GetWindowLong(hwnd, GWL_HWNDPARENT);
					SendMessage(parentWindow, WM_COMMAND, (WPARAM)MAKELPARAM(ID_ADD_TEXT, 0), true);
					return 0;
				}
				break;
			}
		}
		break;
	}
	// note that this return relies on the global variable prevProc
	return CallWindowProc(prevProc, hwnd, msg, wParam, lParam);
}
	
void createChatPage(HWND hwnd, HINSTANCE hInstance){
	
	// text edit area to enter text 
	HWND enterTextBox = CreateWindowW(
		TEXT(L"edit"),
		TEXT(L""),
		WS_VISIBLE | WS_CHILD | WS_BORDER, 
		10, 25, 
		280, 20,
		hwnd,
		(HMENU)ID_ENTER_TEXT_BOX,
		hInstance,
		NULL
	);
	SendMessageW(enterTextBox, WM_SETFONT, (WPARAM)hFont, true);
	
	// subclass proc for msg edit box to allow msg sending via enter key press 
	// note the GetWindowLongW and SetWindowLongW (for unicode support)
	prevProc = (WNDPROC)GetWindowLongW(enterTextBox, GWL_WNDPROC);
	SetWindowLongW(enterTextBox, GWL_WNDPROC, (LONG_PTR)msgEditProc);

	//text area to display text
	HWND textArea = CreateWindowW(
		TEXT(L"edit"),
		TEXT(L""),
		WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 
		10, 50, 
		280, 350,
		hwnd,
		(HMENU)ID_TEXTAREA,
		hInstance,
		NULL
	);
	SendMessageW(textArea, WM_SETFONT, (WPARAM)hFont, true);
	
	// make a button to post text  
	HWND addTextButton = CreateWindow(
		TEXT("button"),
		TEXT("send"),
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
					// grab the text in the 'enter text' box and post to server
					HWND enterTextBox = GetDlgItem(hwnd, ID_ENTER_TEXT_BOX);
					int textLen = GetWindowTextLengthW(enterTextBox);
					WCHAR text[textLen+1] = {0};	// +1 for null term 
					GetWindowTextW(enterTextBox, text, sizeof(text));
					printf("text: %d\n", (int)text[0]);
					
					std::wstring theText = std::wstring(text);
					printf("text entered: %ls\n", theText.c_str());
					printf("num bytes entered: %d\n", textLen);
					
					// send the text to the server to post to all clients 
					std::string msgBuf = getStringFromWideString(theText);
					
					send(connectSocket, msgBuf.c_str(), strlen(msgBuf.c_str()), 0);
						
					// clear the textbox after sending the msg 
					SetDlgItemTextW(hwnd, ID_ENTER_TEXT_BOX, L"");
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
					WCHAR user[24];
					GetWindowTextW(usernameBox, user, sizeof(user));
					username = std::wstring(user);
					std::wcout << "size of username: " << username.size() << std::endl;
				
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
							std::wstring helloMsg = L"hello:" + username;
							std::string res = getStringFromWideString(helloMsg);

							send(connectSocket, res.c_str(), strlen(res.c_str()), 0);	
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
						return 1;
						//PostQuitMessage(0);
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
				DWORD exitCode;
				if(GetExitCodeThread(receiveThread, &exitCode) != 0){
					TerminateThread(receiveThread, exitCode);
				}
					
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
				DWORD exitCode;
				if(GetExitCodeThread(receiveThread, &exitCode) != 0){
					TerminateThread(receiveThread, exitCode);
				}
				
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