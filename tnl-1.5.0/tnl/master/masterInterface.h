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

#ifndef _MASTERINTERFACE_H_
#define _MASTERINTERFACE_H_

#include "tnlEventConnection.h"
#include "tnlRPC.h"

using namespace TNL;

// Useful string constants...
static const char *MasterNoSuchHost = "No Such Host";
static const char *MasterRequestTimedOut = "Timed Out";

enum MasterConstants {
   ConnectRequestTimeout = 30000,
   IPMessageAddressCount = 30,
   GameMissionTypesPerPacket = 20,
};

/// The MasterServerInterface is the RPC interface to the TNL example Master Server.
/// The default Master Server tracks a list of public servers and allows clients
/// to query for them based on different filter criteria, including maximum number of players,
/// region codes, game or mission type, and others.
///
/// When a client wants to initiate a connection with a server listed by the master, it
/// can ask the Master Server to arranage a connection.  The masterclient example that
/// ships with TNL demonstrates a client/server console application that uses the Master
/// Server to arrange connections between a client and a server instance.
///
/// Client/Server programs using the Master Server for server listing should create
/// a subclass of MasterServerInterface named "MasterServerConnection", and override
/// all of the RPC methods that begin with m2c, as they signify master to client messages.
/// RPC methods can be overridden with the TNL_DECLARE_RPC_OVERRIDE and TNL_IMPLEMENT_RPC_OVERRIDE methods.
class MasterServerInterface : public EventConnection
{
protected:
public:
   enum {
      MasterServerInterfaceVersion = 1,
   };

   /// c2mQueryGameTypes is sent from the client to the master to request a list of 
   /// game and mission types that current game servers are reporting.  The queryId
   /// is specified by the client to identify the returning list from the master server.
   TNL_DECLARE_RPC(c2mQueryGameTypes, (U32 queryId));

   /// m2cQueryGameTypesResponse is sent by the master server in response to a c2mQueryGameTypes
   /// from a client.  The queryId will match the original queryId sent by the client.  Clients
   /// should override this method in their custom MasterServerConnection classes.  If there are
   /// more game or mission types than will fit in a single message, the master server will send
   /// multiple m2cQueryGameTypesResponse RPCs.  The master will always send a final
   /// m2cQueryGameTypesResponse with Vectors of size 0 to indicate that no more game or mission
   /// types are to be added.
   TNL_DECLARE_RPC(m2cQueryGameTypesResponse, (U32 queryId, Vector<StringTableEntry> gameTypes, Vector<StringTableEntry> missionTypes));

   /// c2mQueryServers is sent by the client to the master server to request a list of
   /// servers that match the specified filter criteria.  A c2mQueryServers request will
   /// result in one or more m2cQueryServersResponse RPCs, with the final call having an empty
   /// Vector of servers.
   TNL_DECLARE_RPC(c2mQueryServers, (U32 queryId, U32 regionMask,
      U32 minPlayers, U32 maxPlayers, U32 infoFlags,
      U32 maxBots, U32 minCPUSpeed, StringTableEntry gameType, StringTableEntry missionType));

   /// m2cQueryServersResponse is sent by the master server in response to a c2mQueryServers RPC, to
   /// return a partial list of the servers that matched the specified filter criteria.  Because packets
   /// are limited in size, the response server list is broken up into lists of at most IPMessageAddressCount IP addresses
   /// per message.  The Master Server will always send a final, empty m2cQueryServersResponse to signify that the list
   /// is complete.
   TNL_DECLARE_RPC(m2cQueryServersResponse, (U32 queryId, Vector<IPAddress> ipList));

   /// c2mRequestArrangedConnection is an RPC sent from the client to the master to request an arranged
   /// connection with the specified server address.  The internalAddress should be the client's own self-reported
   /// IP address.  The connectionParameters buffer will be sent without modification to the specified
   /// server.
   TNL_DECLARE_RPC(c2mRequestArrangedConnection, (U32 requestId,
      IPAddress remoteAddress, IPAddress internalAddress,
      ByteBufferPtr connectionParameters));

   /// m2cClientRequestedArranged connection is sent from the master to a server to notify it that
   /// a client has requested a connection.  The possibleAddresses vector is a list of possible IP addresses
   /// that the server should attempt to connect to for that client if it accepts the connection request.
   TNL_DECLARE_RPC(m2cClientRequestedArrangedConnection, (U32 requestId, Vector<IPAddress> possibleAddresses,
      ByteBufferPtr connectionParameters));
 
   /// c2mAcceptArrangedConnection is sent by a server to notify the master that it will accept the connection
   /// request from a client.  The requestId parameter sent by the MasterServer in m2cClientRequestedArrangedConnection
   /// should be sent back as the requestId field.  The internalAddress is the server's self-determined IP address.
   TNL_DECLARE_RPC(c2mAcceptArrangedConnection, (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData));

   /// c2mRejectArrangedConnection notifies the Master Server that the server is rejecting the arranged connection
   /// request specified by the requestId.  The rejectData will be passed along to the requesting client.
   TNL_DECLARE_RPC(c2mRejectArrangedConnection, (U32 requestId, ByteBufferPtr rejectData));

   /// m2cArrangedConnectionAccepted is sent to a client that has previously requested a connection to a listed server
   /// via c2mRequestArrangedConnection if the server accepted the connection.  The possibleAddresses vector is the list
   /// of IP addresses the client should attempt to connect to, and the connectionData buffer is the buffer sent by the
   /// server upon accepting the connection.
   TNL_DECLARE_RPC(m2cArrangedConnectionAccepted, (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData));

   /// m2cArrangedConnectionRejected is sent to a client when an arranged connection request is rejected by the
   /// server, or when the request times out because the server never responded.
   TNL_DECLARE_RPC(m2cArrangedConnectionRejected, (U32 requestId, ByteBufferPtr rejectData));

   /// c2mUpdateServerStatus updates the status of a server to the Master Server, specifying the current game
   /// and mission types, any player counts and the current info flags.
   TNL_DECLARE_RPC(c2mUpdateServerStatus, (
      StringTableEntry gameType, StringTableEntry missionType,
      U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags));

   // Version 1 protocol messages:

   /// m2cSetMOTD is sent to a client when the connection is established.  The
   /// client's game string is used to pick which MOTD will be sent.
   TNL_DECLARE_RPC(m2cSetMOTD, (StringPtr motdString));

};


#endif
