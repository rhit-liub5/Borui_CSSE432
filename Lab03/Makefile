CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread

all: proxy

proxy: proxy.cpp
	$(CXX) $(CXXFLAGS) proxy.cpp -o proxy

clean:
	rm -f proxy
