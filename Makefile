CXX=g++
CXXFLAGS=-std=c++14 -Wall -g -pedantic -pthread -lboost_system

all: part1 part2

part1:
	$(CXX) http_server.cpp -o http_server $(CXXFLAGS)
	$(CXX) console.cpp -o console.cgi $(CXXFLAGS)

part2:
	$(CXX) cgi_server.cpp -o cgi_server.exe -lws2_32 -lwsock32 $(CXXFLAGS)

run_part1: part1
	mkdir my_working_directory
	cp http_server my_working_directory/http_server
	cp console.cgi my_working_directory/console.cgi
	cp extra_files/cgi/panel.cgi my_working_directory/panel.cgi
	cp -r extra_files/test_case my_working_directory/test_case
	./my_working_directory/http_server 7001

clean:
	rm -f *~ http_server console.cgi cgi_server.exe

clean_part1: clean
	rm -rf my_working_directory