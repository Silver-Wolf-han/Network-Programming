CC	= gcc
CXX	= g++
CFLAGS	= -Wall -g
MAKE	= make
DIRS	= bin/cmd

PROGS	= npshell

all: $(PROGS)
	for d in $(DIRS); do $(MAKE) -C $$d || exit 1; done
	cp ./bin/cmd/noop ./bin/noop
	cp ./bin/cmd/number ./bin/number
	cp ./bin/cmd/removetag ./bin/removetag
	cp ./bin/cmd/removetag0 ./bin/removetag0
	cp ./bin/cmd/manyblessings ./bin/manyblessings
	cp /usr/bin/ls ./bin/ls 
	cp /usr/bin/cat ./bin/cat
	cp /usr/bin/wc ./bin/wc

%: %.c
	$(CC) -o $@ $(CFLAGS) $<

%: %.cpp
	$(CXX) -o $@ $(CFLAGS) $<

clean:
	rm -f *~ $(PROGS)
	for d in $(DIRS); do $(MAKE) -C $$d clean; done
	rm -f  ./bin/wc ./bin/ls ./bin/cat  ./bin/noop ./bin/number ./bin/removetag ./bin/removetag0 ./bin/manyblessings

