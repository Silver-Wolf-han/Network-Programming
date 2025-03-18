CC	= gcc
CXX	= g++
CFLAGS	= -Wall -g

PROGS	= shell npshell

all: $(PROGS)

%: %.c
	$(CC) -o $@ $(CFLAGS) $<

%: %.cpp
	$(CXX) -o $@ $(CFLAGS) $<

clean:
	rm -f *~ $(PROGS)

