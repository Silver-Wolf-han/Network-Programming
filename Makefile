CXX=g++
CXXFLAGS=-std=c++14 -Wall -g -pedantic -pthread -lboost_system

all:
	$(CXX) socks_server.cpp -o socks_server $(CXXFLAGS)
	$(CXX) console.cpp -o console.cgi $(CXXFLAGS)

project4:
	$(CXX) http_server.cpp -o http_server $(CXXFLAGS)
	$(CXX) console.cpp -o console.cgi $(CXXFLAGS)

clean:
	rm -rf socks_server console.cgi http_server
