CXX=g++
CXXFLAGS=-std=c++14 -Wall -g -pedantic -pthread -lboost_system

all: part1 part2

part1:
	$(CXX) http_server.cpp -o http_server $(CXXFLAGS)
	$(CXX) console.cpp -o console.cgi $(CXXFLAGS)

part2:
	$(CXX) cgi_server.cpp -o cgi_server.exe -lws2_32 -lwsock32 $(CXXFLAGS)

create_part1_dir: part1
	mkdir my_working_directory
	cp http_server my_working_directory/http_server
	cp console.cgi my_working_directory/console.cgi
	cp extra_files/cgi/panel.cgi my_working_directory/panel.cgi
	cp extra_files/cgi/printenv.cgi my_working_directory/printenv.cgi
	cp extra_files/np_single_golden my_working_directory/np_single_golden
	cp -r extra_files/test_case my_working_directory/test_case

	mkdir my_working_directory/bin
	$(CXX) extra_files/command/delayedremovetag.cpp -o my_working_directory/bin/delayedremovetag
	$(CXX) extra_files/command/noop.cpp -o my_working_directory/bin/noop
	$(CXX) extra_files/command/number.cpp -o my_working_directory/bin/number
	$(CXX) extra_files/command/removetag.cpp -o my_working_directory/bin/removetag
	$(CXX) extra_files/command/removetag0.cpp -o my_working_directory/bin/removetag0

run_part1_demo: clean_part1_demo
	mkdir ./project-4-demo-sample-Silver-Wolf-han-main/src/312552056
	cp Makefile ./project-4-demo-sample-Silver-Wolf-han-main/src/312552056/Makefile
	cp http_server.cpp ./project-4-demo-sample-Silver-Wolf-han-main/src/312552056/http_server.cpp
	cp console.cpp ./project-4-demo-sample-Silver-Wolf-han-main/src/312552056/console.cpp

clean:
	rm -f *~ http_server console.cgi cgi_server.exe

clean_part1: clean
	rm -rf my_working_directory

clean_part1_demo:
	rm -rf /project-4-demo-sample-Silver-Wolf-han-main/src/312552056