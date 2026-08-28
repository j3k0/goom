PROG=maindatabloc

CXXFLAGS=-I./ -O2 -Wall -g -D_DEBUG=1
CXX=g++

CFLAGS=-I./ -O2 -Wall -g -D_DEBUG=1
CC=gcc

MDB_OBJ=maindatabloc.o pacharbuffer.o pasequentialdatabloc.o paexception.o

ALL_OBJ=$(MDB_OBJ)

all:$(PROG)

maindatabloc:$(MDB_OBJ)
	g++ -o maindatabloc $(MDB_OBJ)

clean:
	rm -f $(ALL_OBJ) core $(PROG) *~
