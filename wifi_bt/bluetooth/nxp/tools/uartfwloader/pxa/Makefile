#
# Tools makefile for Arm-Android PXA Platform
#   (this builds statically linked version)
#
.SUFFIXES: .C .c .s .o

CROSS_COMPILE=/usr/local/arm-marvell-linux-gnueabi/bin/arm-marvell-linux-gnueabi-
CC=$(CROSS_COMPILE)gcc
CFLAGS=-g -O2 -I.
.o:
	$(CC) $(CFLAGS) -c $<

default: makelinks fw_loader_static


sources=`ls ../src/*.[Cchyls]`
makelinks:
	if [ ! -f ./stamp_links ]; then \
		for i in $(sources) ; do \
			ln -s $$i; \
	        done; \
		touch stamp_links; \
	fi;

OBJS=fw_loader_uart.o fw_loader_io_linux.o
fw_loader_static: makelinks $(OBJS) 
	$(CC) -static -o fw_loader_static $(OBJS)

clean:
	rm -f *.o fw_loader_static

cleanall: 
	find . -type l -exec rm {} \;
	rm -f stamp_links
