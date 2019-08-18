# make file for winsock client

CXX = g++

HEADER = headers/

GUI_LIBS = -lmingw32 -lcomctl32 -mwindows 

LIB = -lws2_32 -lmswsock

FLAGS = -Wall -g -c -std=c++14

LINK_FLAGS = $(LIB) -static-libstdc++ -static-libgcc

EXE = server.exe client.exe

SERVER_OBJ = server.o 

CLIENT_OBJ =  client.o client_helper.o

all: $(EXE)

server.exe: $(SERVER_OBJ)
	$(CXX) $(SERVER_OBJ) $(LINK_FLAGS) -o $@

client.exe: $(CLIENT_OBJ)
	$(CXX) $(CLIENT_OBJ) $(GUI_LIBS) $(LINK_FLAGS) -o $@
	
server.o: server.cpp 
	$(CXX) $(FLAGS) $< -o $@
	
client_helper.o: client_helper.cpp $(HEADER)client_helper.hh 
	$(CXX) $(FLAGS) $< -o $@
	
client.o: client.cpp $(HEADER)client_helper.hh
	$(CXX) $(FLAGS) $< -o $@
	
clean:
	rm *.o