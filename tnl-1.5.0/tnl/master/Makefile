# TNL Makefile
# (c) 2003 GarageGames
#
# This makefile is for Linux atm.


# 
# Configuration
#
CC=g++ -g -I../tnl #-O2

OBJECTS_MASTER=\
	main.o\
	masterInterface.o\
	config.o

CFLAGS=

.cpp.o : 
	$(CC) -c $(CFLAGS) $<

default: $(OBJECTS_MASTER)
	$(CC) -o ../exe/master $(OBJECTS_MASTER) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm

clean:
	rm -f $(OBJECTS_MASTER) ../exe/master

