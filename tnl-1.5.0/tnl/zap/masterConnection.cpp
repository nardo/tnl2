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

#include "../tnl/tnl.h"
#include "masterConnection.h"
#include "../tnl/tnlNetInterface.h"
#include "gameConnection.h"
#include "gameNetInterface.h"
#include "UIQueryServers.h"
#include "UIMenus.h"
#include "UINameEntry.h"
#include "gameObject.h"

namespace Zap
{

extern U32 gSimulatedPing;
extern F32 gSimulatedPacketLoss;
extern bool gQuit;

TNL_IMPLEMENT_NETCONNECTION(MasterServerConnection, NetClassGroupMaster, false);

void MasterServerConnection::startGameTypesQuery()
{
   // Kick off a game types query.
   mCurrentQueryId++;
   c2mQueryGameTypes(mCurrentQueryId);
}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cQueryGameTypesResponse, (U32 queryId, Vector<StringTableEntry> gameTypes, Vector<StringTableEntry> missionTypes))
{
   // Ignore old queries...
   if(queryId != mCurrentQueryId)
      return;

   // Inform the user of the results...
   logprintf("Got game types response - %d game types, %d mission types", gameTypes.size(), missionTypes.size());
   for(S32 i = 0; i < gameTypes.size(); i++)
      logprintf("G(%d): %s", i, gameTypes[i].getString());
   for(S32 i = 0; i < missionTypes.size(); i++)
      logprintf("M(%d): %s", i, missionTypes[i].getString());

   // when we receive the final list of mission and game types,
   // query for servers:
   if(gameTypes.size() == 0 && missionTypes.size() == 0)
   {
      // Invalidate old queries
      mCurrentQueryId++;

      // And automatically do a server query as well - you may not want to do things
      // in this order in your own clients.
      c2mQueryServers(mCurrentQueryId, 0xFFFFFFFF, 0, 128, 0, 128, 0, "", "");
   }
}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cQueryServersResponse, (U32 queryId, Vector<IPAddress> ipList))
{
   // Only process results from current query...
   if(queryId != mCurrentQueryId)
      return;

   // Display the results...
   logprintf("got a list of servers from the master: %d servers", ipList.size());
   for(S32 i = 0; i < ipList.size(); i++)
      logprintf("  %s", Address(ipList[i]).toString());

   gQueryServersUserInterface.addPingServers(ipList);
}

void MasterServerConnection::requestArrangedConnection(const Address &remoteAddress)
{
   mCurrentQueryId++;

   c2mRequestArrangedConnection(mCurrentQueryId, remoteAddress.toIPAddress(),
      getInterface()->getFirstBoundInterfaceAddress().toIPAddress(),
      new ByteBuffer((U8 *) "ZAP!", 5)); 
}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cClientRequestedArrangedConnection, (U32 requestId, Vector<IPAddress> possibleAddresses,
   ByteBufferPtr connectionParameters))
{
   if(!mIsGameServer)
   {
      logprintf("Rejecting arranged connection from %s", Address(possibleAddresses[0]).toString());
      c2mRejectArrangedConnection(requestId, connectionParameters);
      return;
   }

   Vector<Address> fullPossibleAddresses;
   for(S32 i = 0; i < possibleAddresses.size(); i++)
      fullPossibleAddresses.push_back(Address(possibleAddresses[i]));

   // First check if the specified host is banned on this server
   if(gServerGame->getNetInterface()->isHostBanned(fullPossibleAddresses[0]))
   {
      logprintf("Blocking connection from banned host %s", fullPossibleAddresses[0].toString());
      c2mRejectArrangedConnection(requestId, connectionParameters);
      return;
   }
   // Ok, let's do the arranged connection!
   U8 data[Nonce::NonceSize * 2 + SymmetricCipher::KeySize * 2];
   Random::read(data, sizeof(data));
   IPAddress localAddress = getInterface()->getFirstBoundInterfaceAddress().toIPAddress();
   ByteBufferPtr b = new ByteBuffer(data, sizeof(data));
   b->takeOwnership();

   c2mAcceptArrangedConnection(requestId, localAddress, b);
   GameConnection *conn = new GameConnection();

   conn->setNetAddress(fullPossibleAddresses[0]);

   logprintf("Accepting arranged connection from %s", Address(fullPossibleAddresses[0]).toString());
   logprintf("  Generated shared secret data: %s", b->encodeBase64()->getBuffer());

   ByteBufferPtr theSharedData = new ByteBuffer(data + 2 * Nonce::NonceSize, sizeof(data) - 2 * Nonce::NonceSize);
   Nonce nonce(data);
   Nonce serverNonce(data + Nonce::NonceSize);
   theSharedData->takeOwnership();

   conn->connectArranged(getInterface(), fullPossibleAddresses,
      nonce, serverNonce, theSharedData,false);
}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cArrangedConnectionAccepted, (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData))
{
   if(!mIsGameServer && requestId == mCurrentQueryId && connectionData->getBufferSize() >= Nonce::NonceSize * 2 + SymmetricCipher::KeySize * 2)
   {
      logprintf("Remote host accepted arranged connection.");
      logprintf("  Shared secret data: %s", connectionData->encodeBase64()->getBuffer());

      Vector<Address> fullPossibleAddresses;
      for(S32 i = 0; i < possibleAddresses.size(); i++)
         fullPossibleAddresses.push_back(Address(possibleAddresses[i]));

      ByteBufferPtr theSharedData =
                     new ByteBuffer(
                           (U8 *) connectionData->getBuffer() + Nonce::NonceSize * 2,
                           connectionData->getBufferSize() - Nonce::NonceSize * 2
                     );

      Nonce nonce(connectionData->getBuffer());
      Nonce serverNonce(connectionData->getBuffer() + Nonce::NonceSize);

      GameConnection *conn = new GameConnection();
      const char *name = gNameEntryUserInterface.getText();
      if(!name[0])
         name = "Playa";

      conn->setSimulatedNetParams(gSimulatedPacketLoss, gSimulatedPing);
      conn->setClientName(name);
      gClientGame->setConnectionToServer(conn);

      conn->connectArranged(getInterface(), fullPossibleAddresses,
         nonce, serverNonce, theSharedData,true);
   }
}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cArrangedConnectionRejected, (U32 requestId, ByteBufferPtr rejectData))
{
   if(!mIsGameServer && requestId == mCurrentQueryId)
   {
      logprintf("Remote host rejected arranged connection...");
      endGame();
      gMainMenuUserInterface.activate();
   }
}

TNL_IMPLEMENT_RPC_OVERRIDE(MasterServerConnection, m2cSetMOTD, (StringPtr motdString))
{
   gMainMenuUserInterface.setMOTD(motdString);
}

void MasterServerConnection::writeConnectRequest(BitStream *bstream)
{
   Parent::writeConnectRequest(bstream);

   bstream->writeString(ZAP_GAME_STRING); // Game Name
   if(bstream->writeFlag(mIsGameServer))
   {
      bstream->write((U32) 1000);        // CPU speed
      bstream->write((U32) 0xFFFFFFFF);  // region code
      bstream->write((U32) 5);           // num bots
      bstream->write((U32) 10);          // num players
      bstream->write((U32) 32);          // max players
      bstream->write((U32) 0);           // info flags

      bstream->writeString("ZAP Game Type");     // Game type
      bstream->writeString("ZAP Mission Type");  // Mission type
   }
}

void MasterServerConnection::onConnectionEstablished()
{
}

};

