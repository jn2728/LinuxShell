BINS = parser
C = gcc
CFLAGS = -std=gnu99 -Wall -pedantic -g

all: $(BINS)

parser: parser.c
        $(C) $(CFLAGS) -o shell parser.c

clean:
        rm $(BINS)
