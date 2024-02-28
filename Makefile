#$@ is string to left side of the in scope ':'
#$^ is string to right side of in scope ':'

#Makefile for xinutest

CC=gcc
CFLAGS=-Wall -Werror -std=gnu99
DEPS = confirm_output.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) 

xinutest: xinutest.o confirm_output.o
	$(CC) -o $@ $^