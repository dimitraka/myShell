SHELL := /bin/bash

# ==================================================
# COMMANDS

CC = gcc
RM = rm -f

# ==================================================
# TARGETS

all: myshell

# final link for executable
myshell: myshell.o
	$(CC) $^ -o $@

# generate objects
%.o: %.c
	$(CC) -c $<

# clean temporary files
clean:
	$(RM) *.o *~

# remove executable
purge: clean
	$(RM) myshell
