## server and client using WinSock    
    
The server program allows multiple clients to chat with each other by using select(). Any message a client sends to the server gets broadcasted to all connected clients.    
    
The client program takes 2 arguments - the first being an IP address of the server to connect to, and the second being a username of the client. There is a 10-character limit for the username, and 'quit' should be typed and entered to disconnect from the server.    
    
![screenshot of the server and 2 clients](screenshots/screenshot.png)    