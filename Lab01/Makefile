CXX = g++
CXXFLAGS = -Wall -std=c++17 -g

all: server client

server: server.o
	$(CXX) $(CXXFLAGS) server.o -o server

client: client.o
	$(CXX) $(CXXFLAGS) client.o -o client

server.o: server.cc
	$(CXX) $(CXXFLAGS) -c server.cc

client.o: client.cc
	$(CXX) $(CXXFLAGS) -c client.cc

clean:
	rm -f server client server.o client.o