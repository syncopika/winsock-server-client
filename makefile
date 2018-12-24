# make file for winsock test

CXX = g++

LIB = -lws2_32 -lmswsock

FLAGS = -Wall -g -c -std=c++14

LINK_FLAGS = -static-libstdc++ -static-libgcc $(LIB)

EXE = server.exe client.exe

SERVER_OBJ = server.o 

CLIENT_OBJ = client.o

all: $(EXE)

server.exe: $(SERVER_OBJ)
	$(CXX) $(SERVER_OBJ) $(LINK_FLAGS) -o $@

client.exe: $(CLIENT_OBJ)
	$(CXX) $(CLIENT_OBJ) $(LINK_FLAGS) -o $@
	
server.o: server.cpp 
	$(CXX) $(FLAGS) $< -o $@
	
client.o: client.cpp
	$(CXX) $(FLAGS) $< -o $@
	
clean:
	rm *.exe && rm *.o