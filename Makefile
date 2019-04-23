# OPERATING SYSTEMS DESING - 16/17
# Makefile for OSD file system

INCLUDEDIR=./include
CC=gcc
CFLAGS=-g -Wall -Werror -I$(INCLUDEDIR)
AR=ar
MAKE=make

OBJS_DEV= blocks_cache.o filesystem.o
LIB=libfs.a


all: create_disk test

test: $(LIB)
	$(CC) $(CFLAGS) -o test test.c libfs.a

filesystem.o: $(INCLUDEDIR)/filesystem.h
blocks_cache.o: $(INCLUDEDIR)/blocks_cache.h

$(LIB): $(OBJS_DEV)
	$(AR) rcv $@ $^

create_disk: create_disk.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(LIB) $(OBJS_DEV) test create_disk create_disk.o
