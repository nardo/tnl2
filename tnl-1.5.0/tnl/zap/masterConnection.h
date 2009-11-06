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

#ifndef _MASTERCONNECTION_H_
#define _MASTERCONNECTION_H_

#include "../master/masterInterface.h"

namespace Zap
{

class MasterServerConnection : public MasterServerInterface
{
   typedef MasterServerInterface Parent;

   // ID of our current query.
   U32 mCurrentQueryId;

   bool mIsGameServer;

public:
   MasterServerConnection(bool isGameServer = false)
   {
      mIsGameServer = isGameServer;
      mCurrentQueryId = 0;
      setIsConnectionToServer();
      setIsAdaptive();
   }

   void startGameTypesQuery();
   void cancelArrangedConnectionAttempt() { mCurrentQueryId++; }
   void requestArrangedConnection(const Address &remoteAddress);

   TNL_DECLARE_RPC_OVERRIDE(m2cQueryGameTypesResponse, (U32 queryId, Vector<StringTableEntry> gameTypes, Vector<StringTableEntry> missionTypes));
   TNL_DECLARE_RPC_OVERRIDE(m2cQueryServersResponse, (U32 queryId, Vector<IPAddress> ipList));
   TNL_DECLARE_RPC_OVERRIDE(m2cClientRequestedArrangedConnection, (U32 requestId, Vector<IPAddress> possibleAddresses,
      ByteBufferPtr connectionParameters));
   TNL_DECLARE_RPC_OVERRIDE(m2cArrangedConnectionAccepted, (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData));
   TNL_DECLARE_RPC_OVERRIDE(m2cArrangedConnectionRejected, (U32 requestId, ByteBufferPtr rejectData));
   TNL_DECLARE_RPC_OVERRIDE(m2cSetMOTD, (StringPtr motdString));

   void writeConnectRequest(BitStream *bstream);
   void onConnectionEstablished();

   TNL_DECLARE_NETCONNECTION(MasterServerConnection);
};

};

#endif
