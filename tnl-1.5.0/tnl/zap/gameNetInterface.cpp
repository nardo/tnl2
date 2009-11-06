//-----------------------------------------------------------------------------------
//
//   Torque Network Library - ZAP example multiplayer vector graphics space game
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "gameNetInterface.h"
#include "UIQueryServers.h"
#include "game.h"

namespace Zap
{

extern const char *gServerPassword;
extern bool gDedicatedServer;

GameNetInterface::GameNetInterface(const Address &bindAddress, Game *theGame)
   : NetInterface(bindAddress)
{
   mGame = theGame;
};

void GameNetInterface::banHost(const Address &bannedAddress, U32 bannedMilliseconds)
{
   BannedHost h;
   h.theAddress = bannedAddress;
   h.banDuration = bannedMilliseconds;
   mBanList.push_back(h);
}

bool GameNetInterface::isHostBanned(const Address &theAddress)
{
   for(S32 i = 0; i < mBanList.size(); i++)
      if(theAddress.isEqualAddress(mBanList[i].theAddress))
         return true;
   return false;
}

void GameNetInterface::processPacket(const Address &sourceAddress, BitStream *pStream)
{
   for(S32 i = 0; i < mBanList.size(); i++)
      if(sourceAddress.isEqualAddress(mBanList[i].theAddress))
         return;
   Parent::processPacket(sourceAddress, pStream);
}

void GameNetInterface::checkBanlistTimeouts(U32 timeElapsed)
{
   for(S32 i = 0; i < mBanList.size(); )
   {
      if(mBanList[i].banDuration < timeElapsed)
         mBanList.erase_fast(i);
      else
      {
         mBanList[i].banDuration -= timeElapsed;
         i++;
      }
   }
}

void GameNetInterface::handleInfoPacket(const Address &remoteAddress, U8 packetType, BitStream *stream)
{
   switch(packetType)
   {
      case Ping:
         logprintf("Got ping packet from %s", remoteAddress.toString());
         if(mGame->isServer())
         {
            Nonce clientNonce;
            clientNonce.read(stream);
            char string[256];
            stream->readString(string);
            if(strcmp(string, ZAP_GAME_STRING))
               break;

            U32 token = computeClientIdentityToken(remoteAddress, clientNonce);
            PacketStream pingResponse;
            pingResponse.write(U8(PingResponse));
            clientNonce.write(&pingResponse);
            pingResponse.write(token);
            pingResponse.sendto(mSocket, remoteAddress);
         }
         break;
      case PingResponse:
         {
            logprintf("Got ping response from %s", remoteAddress.toString());
            Nonce theNonce;
            U32 clientIdentityToken;
            theNonce.read(stream);
            stream->read(&clientIdentityToken);
            gQueryServersUserInterface.gotPingResponse(remoteAddress, theNonce, clientIdentityToken);
         }
         break;
      case Query:
         {
            logprintf("Got query from %s", remoteAddress.toString());
            Nonce theNonce;
            U32 clientIdentityToken;
            theNonce.read(stream);
            stream->read(&clientIdentityToken);
            if(clientIdentityToken == computeClientIdentityToken(remoteAddress, theNonce))
            {
               PacketStream queryResponse;
               queryResponse.write(U8(QueryResponse));
               theNonce.write(&queryResponse);
               queryResponse.writeString(gServerGame->getHostName(), QueryServersUserInterface::MaxServerNameLen);
               queryResponse.write(gServerGame->getPlayerCount());
               queryResponse.write(gServerGame->getMaxPlayers());
               queryResponse.writeFlag(gDedicatedServer);
               queryResponse.writeFlag(gServerPassword != NULL);

               queryResponse.sendto(mSocket, remoteAddress);
            }
         }
         break;
      case QueryResponse:
         {
            logprintf("Got query response from %s", remoteAddress.toString());
            Nonce theNonce;
            char nameString[256];
            U32 playerCount, maxPlayers;
            bool dedicated, passwordRequired;
            theNonce.read(stream);
            stream->readString(nameString);
            stream->read(&playerCount);
            stream->read(&maxPlayers);
            dedicated = stream->readFlag();
            passwordRequired = stream->readFlag();
            gQueryServersUserInterface.gotQueryResponse(remoteAddress, theNonce, nameString, playerCount, maxPlayers, dedicated, passwordRequired);
         }
         break;
   }
}

void GameNetInterface::sendPing(const Address &theAddress, const Nonce &clientNonce)
{
   logprintf("pinging server %s...", theAddress.toString());

   PacketStream packet;
   packet.write(U8(Ping));
   clientNonce.write(&packet);
   packet.writeString(ZAP_GAME_STRING);
   packet.sendto(mSocket, theAddress);
}

void GameNetInterface::sendQuery(const Address &theAddress, const Nonce &clientNonce, U32 identityToken)
{
   logprintf("querying server %s...", theAddress.toString());

   PacketStream packet;
   packet.write(U8(Query));
   clientNonce.write(&packet);
   packet.write(identityToken);
   packet.sendto(mSocket, theAddress);
}

};

