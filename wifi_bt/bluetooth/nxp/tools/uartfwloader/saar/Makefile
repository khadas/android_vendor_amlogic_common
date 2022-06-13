#
# Tools makefile for Arm-Linux SAAR Platform
.SUFFIXES:
.SUFFIXES: .C .c .s .o

CC=arm-linux-gcc
CFLAGS=-g -O2 -I.
.c.o:
	$(CC) $(CFLAGS) -c $<

default: makelinks fw_loader


sources=`ls ../src/*.[Cchyls]`
makelinks:
	if [ ! -f ./stamp_links ]; then \
		for i in $(sources) ; do \
			ln -s $$i; \
	        done; \
		touch stamp_links; \
	fi;

OBJS=fw_loader_uart.o fw_loader_io_linux.o
fw_loader: makelinks $(OBJS) 
	$(CC) -o fw_loader $(OBJS)

clean:
	rm -f *.o fw_loader

cleanall: 
	find . -type l -exec rm {} \;
	rm -f stamp_links
