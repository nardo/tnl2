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

#ifndef _UIQUERYSERVERS_H_
#define _UIQUERYSERVERS_H_

#include "UI.h"
#include "tnlNonce.h"

namespace Zap
{

class QueryServersUserInterface : public UserInterface
{
public:
   U32 selectedId;
   S32 sortColumn;
   S32 lastSortColumn;
   bool sortAscending;
   bool shouldSort;
   Nonce mNonce;
   U32 pendingPings;
   U32 pendingQueries;
   U32 broadcastPingSendTime;
   U32 lastUsedServerId;

   enum {
      MaxServerNameLen = 20,
      ServersPerScreen = 21,
      ServersAbove = 9,
      ServersBelow = 9,
      MaxPendingPings = 15,
      MaxPendingQueries = 10,
      PingQueryTimeout = 1500,
      PingQueryRetryCount = 3,
   };
   struct ServerRef
   {
      enum State
      {
         Start,
         SentPing,
         ReceivedPing,
         SentQuery,
         ReceivedQuery,
      };
      U32 state;
      U32 id;
      U32 pingTime;
      U32 identityToken;
      U32 lastSendTime;
      U32 sendCount;
      bool isFromMaster;
      bool dedicated;
      bool passwordRequired;
      Nonce sendNonce;
      char serverName[MaxServerNameLen+1];
      Address serverAddress;
      S32 playerCount, maxPlayers;
   };
   struct ColumnInfo
   {
      const char *name;
      U32 xStart;
      ColumnInfo(const char *nm = NULL, U32 xs = 0) { name = nm; xStart = xs; }
   };
   Vector<ServerRef> servers;
   Vector<ColumnInfo> columns;

   QueryServersUserInterface();

   S32 findSelectedIndex();

   void onKeyDown(U32 key);
   void onSpecialKeyDown(U32 key);
   void onControllerButtonDown(U32 buttonIndex);

   void onActivate();
   void idle(U32 t);
   void render();

   void addPingServers(const Vector<IPAddress> &ipList);

   void sort();

   void gotPingResponse(const Address &theAddress, const Nonce &clientNonce, U32 clientIdentityToken);
   void gotQueryResponse(const Address &theAddress, const Nonce &clientNonce, const char *serverName, U32 playerCount, U32 maxPlayers, bool dedicated, bool passwordRequired);
};

extern QueryServersUserInterface gQueryServersUserInterface;

};

#endif
