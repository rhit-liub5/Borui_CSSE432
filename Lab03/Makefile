CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic -pthread
TARGET = proxy

all: $(TARGET)

$(TARGET): proxy.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) proxy.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean
