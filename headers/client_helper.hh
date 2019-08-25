// client using winsock2 

#ifndef CLIENT_HELPER_H
#define CLIENT_HELPER_H

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

extern HWND chatPage;
extern HWND connectionPage;

// functions 
DWORD WINAPI receiveMessagesProc(LPVOID lpParam);

void createConnectionPage(HWND hwnd, HINSTANCE hInstance);
void createChatPage(HWND hwnd, HINSTANCE hInstance);

std::wstring getWideStringFromString(std::string string);
std::string getStringFromWideString(std::wstring wstring);

LRESULT CALLBACK msgEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


#endif