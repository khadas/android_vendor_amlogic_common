#
# Tools makefile for Linux
.SUFFIXES:
.SUFFIXES: .C .c .s .o

OSTYPE=linux
CC=gcc -Wall
CONFIG_TEST_CODE=0
ifeq ($(CONFIG_TEST_CODE),1)
	CFLAGS=-g -O2 -I. -DTEST_CODE
else
	CFLAGS=-g -O2 -Wcomment -Wunused-variable -Wpointer-sign -I.
endif

help:
	@echo "make <cleanmake|make|cleanall|clean>\n"

.c.o:
	$(CC) $(CFLAGS) -c $<

cleanmake: clean make

sources=`ls ../src/*.[Cchyls]`
makelinks:
	if [ ! -f ./stamp_links ]; then \
		for i in $(sources) ; do \
			ln -s $$i; \
	        done; \
		touch stamp_links; \
	fi;

fw_loader_uart.o: fw_loader_uart.c fw_loader_uart.h
fw_loader_io_linux.o: fw_loader_io_linux.c fw_loader_io_linux.h
OBJS=fw_loader_uart.o fw_loader_io_linux.o  
fw_loader: makelinks $(OBJS)  
	$(CC) -o fw_loader $(OBJS)

make: fw_loader

clean:
	rm -f *.o fw_loader

cleanall: 
	find . -type l -exec rm {} \;
	rm -f stamp_links
