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

#ifndef _GAMENETINTERFACE_H_
#define _GAMENETINTERFACE_H_

#include "../tnl/tnlNetInterface.h"

using namespace TNL;

namespace Zap
{

class Game;

class GameNetInterface : public NetInterface
{
   typedef NetInterface Parent;
   Game *mGame;

   struct BannedHost {
      Address theAddress;
      U32 banDuration;
   };
   Vector<BannedHost> mBanList;
public:
   enum PacketType
   {
      Ping = FirstValidInfoPacketId,
      PingResponse,
      Query,
      QueryResponse,
   };
   GameNetInterface(const Address &bindAddress, Game *theGame);
   void handleInfoPacket(const Address &remoteAddress, U8 packetType, BitStream *stream);
   void sendPing(const Address &theAddress, const Nonce &clientNonce);
   void sendQuery(const Address &theAddress, const Nonce &clientNonce, U32 identityToken);
   void processPacket(const Address &sourceAddress, BitStream *pStream);
   void banHost(const Address &bannedAddress, U32 bannedMilliseconds);
   void checkBanlistTimeouts(U32 timeElapsed);
   bool isHostBanned(const Address &theAddress);
};

};

#endif
