CC=gcc
CFLAGS=-O3 -g -Wall -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -Wpointer-arith -Wpointer-arith -Wsign-compare -Wchar-subscripts -Wstrict-prototypes -Wshadow -Wformat=2 -Wtype-limits
LIBS=-lasound -lm

all: loop

loop: loop.c
	$(CC) $(CFLAGS) -o alsa-loop loop.c $(LIBS)
