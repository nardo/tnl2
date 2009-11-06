//-----------------------------------------------------------------------------------
//
//   Torque Network Library - Master Server
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


#include "masterInterface.h"

// Since this is an interface, we implement a bunch of stubs.

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mQueryGameTypes, (U32 queryId), (queryId),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cQueryGameTypesResponse, 
   (U32 queryId, Vector<StringTableEntry> gameTypes, Vector<StringTableEntry> missionTypes), (queryId, gameTypes, missionTypes),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mQueryServers,
   (U32 queryId, U32 regionMask, U32 minPlayers, U32 maxPlayers, U32 infoFlags,
   U32 maxBots, U32 minCPUSpeed, StringTableEntry gameType, StringTableEntry missionType),
   (queryId, regionMask, minPlayers, maxPlayers, infoFlags, maxBots, minCPUSpeed, gameType, missionType),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cQueryServersResponse,
   (U32 queryId, Vector<IPAddress> ipList), (queryId, ipList),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mRequestArrangedConnection, (U32 requestId,
   IPAddress remoteAddress, IPAddress internalAddress, ByteBufferPtr connectionParameters),
   (requestId, remoteAddress, internalAddress, connectionParameters),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cClientRequestedArrangedConnection,
   (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionParameters),
   (requestId, possibleAddresses, connectionParameters),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mAcceptArrangedConnection,
   (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData),
   (requestId, internalAddress, connectionData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mRejectArrangedConnection,
   (U32 requestId, ByteBufferPtr rejectData),
   (requestId, rejectData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cArrangedConnectionAccepted,
   (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData),
   (requestId, possibleAddresses, connectionData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cArrangedConnectionRejected,
   (U32 requestId, ByteBufferPtr rejectData),
   (requestId, rejectData),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, c2mUpdateServerStatus, (
   StringTableEntry gameType, StringTableEntry missionType,
   U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags),
   (gameType, missionType, botCount, playerCount, maxPlayers, infoFlags),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0) {}

TNL_IMPLEMENT_RPC(MasterServerInterface, m2cSetMOTD, (StringPtr motdString), (motdString),
   NetClassGroupMasterMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0) {}