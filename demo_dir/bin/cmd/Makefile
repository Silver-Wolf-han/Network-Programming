CC	= gcc
CXX	= g++
CFLAGS	= -Wall -g

PROGS	= noop number removetag removetag0

all: $(PROGS)

%: %.c
	$(CC) -o $@ $(CFLAGS) $<

%: %.cpp
	$(CXX) -o $@ $(CFLAGS) $<

clean:
	rm -f *~ $(PROGS)

