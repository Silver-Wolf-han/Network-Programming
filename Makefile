CXX	= g++
CFLAGS	= -Wall -g
MAKE	= make
DIRS	= cmd

PROGS	= np_sample np_sample_proc

all:
	$(CXX) -c npshell.cpp $(CFLAGS) -o npshell.o
	$(CXX) np_sample.cpp npshell.o $(CFLAGS) -o np_sample
	$(CXX) np_sample_proc.cpp $(CFLAGS) -o np_sample_proc

run_test: all
	mkdir working_directory
	mkdir working_directory/bin
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
	for d in $(DIRS); do $(MAKE) -C $$d clean; done
	rm -rf working_directory

