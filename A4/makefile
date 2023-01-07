CC = gcc
CFLAGS = -Wall -pedantic -g -std=gnu99 -pthread -I/local/courses/csse2310/include
CARGS = -L/local/courses/csse2310/lib -lcsse2310a3 -lcsse2310a4 -L/local/courses/csse2310/lib -ltinyexpr -lm
.PHONY: clean
.DEFAULT_GOAL := all

all: intclient intserver

intclient: intclient.o common.o
	$(CC) $(CFLAGS) $(CARGS) $^ -o $@

intserver: intserver.o common.o
	$(CC) $(CFLAGS) $(CARGS) $^ -o $@

intclient.o: intclient.c intclient.h common.h

intserver.o: intserver.c intserver.h intclient.h common.h

common.o: common.c common.h

clean:
	rm -f *.o
