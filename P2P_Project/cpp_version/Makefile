CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

COMMON = common.cpp
TRACKER_SRC = tracker.cpp tracker_state.cpp $(COMMON)
PEER_SRC = peer.cpp file_manager.cpp $(COMMON)

all: tracker peer

tracker: $(TRACKER_SRC)
	$(CXX) $(CXXFLAGS) -o tracker $(TRACKER_SRC)

peer: $(PEER_SRC)
	$(CXX) $(CXXFLAGS) -o peer $(PEER_SRC)

clean:
	rm -f tracker peer
