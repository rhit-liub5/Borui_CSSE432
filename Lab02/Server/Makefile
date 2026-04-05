CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
TARGET = server
SRC = server.cc

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)