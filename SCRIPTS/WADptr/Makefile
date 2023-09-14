# **********************************************************************
# *                        The WADPTR Project                          *
# *                                                                    *
# *                     MAKEFILE for the project                       *
# *                                                                    *
# **********************************************************************

# install prefix:
PREFIX          = /usr/local

# library for RISC OS
WIMPLIBPATH	= scsi::5.$.vice1_1.src.arch.riscos.wimplib


# Objects for Unix / DOS
OBJECTSO	= main.o waddir.o errors.o wadmerge.o palette.o lumps.o

# default
OBJECTS		= $(OBJECTSO)

DOSFLAGS	= CC=gcc EXECUTABLE=wadptr.exe DELETE=del CFLAGS=-O3

LINUXFLAGS      = CC=gcc EXECUTABLE=wadptr DELETE=rm CFLAGS='-O3 -DANSILIBS -DNORMALUNIX'


all : $(EXECUTABLE)

dos:
	make $(DOSFLAGS)

dos_clean:
	make $(DOSFLAGS) clean

linux:
	make $(LINUXFLAGS)

linux_clean:
	make $(LINUXFLAGS) clean


$(EXECUTABLE) : $(OBJECTS)
	$(PURIFY) $(CC) -o $(EXECUTABLE) $(OBJECTS) $(LDFLAGS)

######### C source files ##########

main.o : main.c wadptr.h errors.h waddir.h wadmerge.h lumps.h
	$(CC) $(CFLAGS) -c main.c

waddir.o : waddir.c waddir.h errors.h
	$(CC) $(CFLAGS) -c waddir.c

errors.o : errors.c errors.h
	$(CC) $(CFLAGS) -c errors.c

lumps.o : lumps.c lumps.h waddir.h errors.h
	$(CC) $(CFLAGS) -c lumps.c

wadmerge.o : wadmerge.c wadmerge.h waddir.h errors.h
	$(CC) $(CFLAGS) -c wadmerge.c

palette.o : palette.c palette.h waddir.h errors.h
	$(CC) $(CFLAGS) -c palette.c

########## Functions ############

install :
	install -D wadptr $(DESTDIR)$(PREFIX)/bin/wadptr
	install -D wadptr.txt $(DESTDIR)$(PREFIX)/share/doc/wadptr/wadptr.txt

clean : 
	-$(DELETE) $(EXECUTABLE)
	-$(DELETE) *.o

