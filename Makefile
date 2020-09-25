CFLAGS= -Wall
all: hw01.c
	gcc $(CFLAGS) hw01.c -o tftp.out libunp.a