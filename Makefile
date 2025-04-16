CXX	= g++
CFLAGS	= -Wall -g
MAKE	= make
DIRS	= cmd

PROGS	= np_simple np_multi_proc

all:
	$(CXX) -c npshell.cpp $(CFLAGS) -o npshell.o
	$(CXX) np_multi_proc.cpp npshell.o $(CFLAGS) -o np_multi_proc

test: all
	mkdir working_directory
	mkdir working_directory/bin
	mkdir working_directory/user_pipe
	for d in $(DIRS); do $(MAKE) -C $$d || exit 1; done
	cp ./cmd/noop ./working_directory/bin/noop
	cp ./cmd/number ./working_directory/bin/number
	cp ./cmd/removetag ./working_directory/bin/removetag
	cp ./cmd/removetag0 ./working_directory/bin/removetag0
	cp ./cmd/manyblessings ./working_directory/bin/manyblessings
	cp ./test.html ./working_directory/test.html
	cp /usr/bin/ls ./working_directory/bin/ls 
	cp /usr/bin/cat ./working_directory/bin/cat
	cp /usr/bin/wc ./working_directory/bin/wc

clean:
	rm -f *~ $(PROGS)
	rm -f *.o

clean_test: clean
	rm -rf working_directory

