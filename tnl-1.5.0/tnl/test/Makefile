# TNL Makefile
# (c) 2003 GarageGames
#
# This makefile is for Linux atm.


# 
# Configuration
#
CC=g++ -g #-O2

OBJECTS_SERVER=\
	testGame.o\
	server.o

OBJECTS_TNLTEST=\
	testGame.o\
	testWindow.o \
	testGameRender.o \

CFLAGS=

.cpp.o : 
	$(CC) -c $(CFLAGS) $<

default: $(OBJECTS_SERVER) $(OBJECTS_CLIENT)
	@echo Building linux dedicated server...
	$(CC) -o server $(OBJECTS_SERVER) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm

tnltest: $(OBJECTS_TNLTEST)
	@echo Building TNLTest gui...
	$(CC) -o tnltest $(OBJECTS_TNLTEST) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lGL -lGLU -lglut -lm
clean:
	rm -f $(OBJECTS_SERVER) $(OBJECTS_CLIENT) $(OBJECTS_TNLTEST) server client tnltest
