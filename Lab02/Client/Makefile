CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
TARGET = client
SRC = client.cc

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)