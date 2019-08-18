#include "headers/client_helper.hh"

// register window 
const char g_szClassName[] = "main GUI";
const char g_szClassName2[] = "connection page";
const char g_szClassName3[] = "chat page";

// handler variable for the main window 
HWND hwnd;

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
