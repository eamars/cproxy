# Author: 		Ran Bao
# Date:			6/Jan/2015
# Description:	Makefile for c-proxy

# Define compilers and other flags
CC = cc
CFLAGS = -Os -Wall -Wextra -g -std=c99
LIBS =
DEL = rm

all: cproxy.out

proxy_main.o: proxy_main.c proxy.h config.h
	$(CC) -c $(CFLAGS) $(LIBS) $< -o $@

config.o: config.c config.h
	$(CC) -c $(CFLAGS) $(LIBS) $< -o $@

cproxy.out: proxy_main.o config.o
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

.PHONY: clean
clean:
	-$(DEL) *.o *.out