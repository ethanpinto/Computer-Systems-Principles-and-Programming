CC = gcc
CFLAGS = -Wall -pedantic -g -std=gnu99 -I/local/courses/csse2310/include
CARGS = -L/local/courses/csse2310/lib -lcsse2310a3
.PHONY: clean

jobrunner: main.o parse.o running.o
	$(CC) $(CFLAGS) $(CARGS) $^ -o $@

main.o: main.c parse.h running.h

parse.o: parse.c parse.h

running.o: running.c parse.h running.h

clean:
	rm -f *.o
