CXX=g++
CXXFLAGS=-std=c++14 -Wall -g -pedantic -pthread -lboost_system

all: part1 part2

part1:
	$(CXX) http_server.cpp -o http_server $(CXXFLAGS)
	$(CXX) console.cpp -o console.cgi $(CXXFLAGS)

part2:
	$(CXX) cgi_server.cpp -o cgi_server.exe -lws2_32 -lwsock32 $(CXXFLAGS)

clean:
	rm -f *~ http_server console.cgi cgi_server.exe
