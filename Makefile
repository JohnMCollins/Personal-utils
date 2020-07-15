#
# Copyright (c) Xi Software Ltd. 2007.
#
# Makefile: created by John M Collins on Thu Jul 12 2007.
#----------------------------------------------------------------------
# $Header$
# $Log$
#----------------------------------------------------------------------
#

XMLHDRS		=	-I/usr/include/libxml2
CFLAGS		=	-O2
CXXFLAGS	=	-O2 $(XMLHDRS)
LDFLAGS		=	-s

all:	ftppasswd what notrail untabify tabify fdate

ftppasswd:	ftppasswd.o
	$(CXX) $(LDFLAGS) -o ftppasswd ftppasswd.o -lxml2 -lcrypt

what:	       what.o
	$(CC) $(LDFLAGS) -o what what.o

fdate:			fdate.o
	$(CC) $(LDFLAGS) -o fdate fdate.o

notrail:	notrail.o
	$(CC) $(LDFLAGS) -o notrail notrail.o -lfl

untabify:	untabify.o
	$(CC) $(LDFLAGS) -o untabify untabify.o -lfl

tabify:		tabify.o
	$(CC) $(LDFLAGS) -o tabify tabify.o -lfl

clean:
	rm -f *.o what ftppasswd untabify tabify fdate notrail


