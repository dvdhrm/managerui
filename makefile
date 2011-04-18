#
# managerui - makefile
# Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
# Dedicated to the Public Domain
#

.PHONY: all clean install uninstall

all:
	gcc -Wall -O2 -o managerui src/main.c

clean:
	rm -f managerui

install:
	install -m 755 managerui /usr/bin

uninstall:
	rm /usr/bin/managerui
