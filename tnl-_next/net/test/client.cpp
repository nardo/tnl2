// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "testGame.h"
#include <stdio.h>
#include <string.h>

class DedicatedServerLogConsumer : public LogConsumer
{
public:
   void logString(const char *string)
   {
      printf("%s\n", string);
   }
} gDedicatedServerLogConsumer;

int main(int argc, const char **argv)
{
   const char *localBroadcastAddress = "IP:broadcast:28999";

   S32 port = 28999;
   if(argc == 2)
      port = atoi(argv[1]);
   TestGame* theGame = new TestGame(false, Address(IPProtocol, Address::Any, port),Address(localBroadcastAddress));
   for(;;)
      theGame->tick();
}
