# Desc: makefile for mmn-13 my_format FAT12 formatter program
# Author: Roman Smirnov

CC = gcc

BIN = my_format
FLAGS = -Wall -Wextra -pedantic -L./-m32

all: clean my_format

my_format: my_format.c
	${CC} my_format.c -o ${BIN} -lm ${FLAGS}

clean:
	rm -f ${BIN}

