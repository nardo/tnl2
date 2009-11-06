# TNL Makefile
# (c) 2003 GarageGames

# 
# Configuration
#
CC=g++ -g -I../tnl -I../glut -I../openal -DTNL_DEBUG -DTNL_ENABLE_LOGGING #-O2

OBJECTS_ZAP=\
   CTFGame.o\
   HTFGame.o\
   LoadoutZone.o\
   SweptEllipsoid.o\
   UI.o\
   UICredits.o\
   UIEditor.o\
   UIGame.o\
   UIInstructions.o\
   UIMenus.o\
   UINameEntry.o\
   UIQueryServers.o\
   barrier.o\
   controlObjectConnection.o\
   engineeredObjects.o\
   flagItem.o\
   footballGame.o\
   game.o\
   gameConnection.o\
   gameItems.o\
   gameLoader.o\
   gameNetInterface.o\
   gameObject.o\
   gameObjectRender.o\
   gameType.o\
   gameWeapons.o\
   goalZone.o\
   gridDB.o\
   huntersGame.o\
   input.o\
   item.o\
   linuxInput.o\
   loadoutSelect.o\
   main.o\
   masterConnection.o\
   moveObject.o\
   projectile.o\
   quickChat.o\
   rabbitGame.o\
   retrieveGame.o\
   sfx.o\
   ship.o\
   soccerGame.o\
   sparkManager.o\
   teleporter.o\
   voiceCodec.o\
   gsm_encode.o\
   gsm_decode.o\
   gsm_state.o\
   lpc10enc.o\
   lpc10dec.o\
   ../master/masterInterface.o

CFLAGS=

.c.o:
	$(CC) -c $(CFLAGS) $<

.cpp.o : 
	$(CC) -c $(CFLAGS) $<

default: ../exe/zap

zap: ../exe/zap

zapded: ../exe/zapded

../exe/zap: $(OBJECTS_ZAP)
	$(CC) -o ../exe/zap $(OBJECTS_ZAP) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a ../openal/linux/libopenal.a -lpthread -lstdc++ -lGL -lGLU -lglut -lm

../exe/zapded: CFLAGS=-DZAP_DEDICATED
../exe/zapded: $(OBJECTS_ZAP)
	$(CC) -o ../exe/zapded $(OBJECTS_ZAP) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lpthread -lstdc++ -lm

../master/masterInterface.o:
	make -C ../master

clean:
	rm -f $(OBJECTS_ZAP) ../exe/zap ../exe/zapded

cleano:
	rm -f $(OBJECTS_ZAP)
