//-----------------------------------------------------------------------------------
//
//   Torque Network Library - Master Server Game Client/Server example
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

// Master server test client.
//
// This program operates in one of two modes - as a game client
// or a game server.
//
// In game server mode, which you can access by running:
//
//      masterclient <masterAddress> -server <port>
//
// the process creates a MasterServerConnection to the master server
// and periodically updates its server status with a random game type
// and mission string.  If the connection to the master server times
// out or is disconnected, the server will attempt to reconnect to the
// master.  If a client requests an arranged connection, the server will
// accept it 75% of the time, using a GameConnection instance for
// the arranged connection.  The GameConnection lasts between 15 and
// 30 seconds (random), at which point the server will force a disconnect
// with the client.
//
// The client behaves in much the same way.  It will log in to the
// master server, and upon successful connection will query the master
// for a list of game servers.  It will pick one of the servers
// out of the list to connect to randomly and will initiate an
// arranged connection with that host.  If the connection to the
// server is rejected, or if it connects succesfully and later disconnects,
// it will restart the process of requesting a server list from the master.
//
// This example client demonstrates everything you need to know to implement
// your own master server clients.


#include <stdio.h>
#include "tnlNetInterface.h"
#include "../master/masterInterface.h"
#include "tnlVector.h"
#include "tnlRandom.h"

using namespace TNL;

class MasterServerConnection;
class GameConnection;

//------------------------------------------------------------------------

RefPtr<MasterServerConnection> gMasterServerConnection;
RefPtr<GameConnection> gClientConnection;
StringTableEntry gCurrentGameType("SomeGameType");
StringTableEntry gCurrentMissionType("SomeMissionType");

bool gIsServer = false;
bool gQuit = false;

void GameConnectionDone();

//------------------------------------------------------------------------

class GameConnection : public EventConnection
{
public:
   // The server maintains a linked list of clients...
   GameConnection *mNext;
   GameConnection *mPrev;
   static GameConnection gClientList;

   // Time in milliseconds at which we were created.
   U32 mCreateTime;

   GameConnection()
   {
      mNext = mPrev = this;
      mCreateTime = Platform::getRealMilliseconds();
   }

   ~GameConnection()
   {
      // unlink ourselves if we're in the client list
      mPrev->mNext = mNext;
      mNext->mPrev = mPrev;

      // Tell the user...
      logprintf("%s disconnected", getNetAddress().toString());
   }

   /// Adds this connection to the doubly linked list of clients.
   void linkToClientList()
   {
      mNext = gClientList.mNext;
      mPrev = gClientList.mNext->mPrev;
      mNext->mPrev = this;
      mPrev->mNext = this;
   }

   void onConnectionEstablished()
   {
      logprintf("connection to %s - %s established.", isInitiator() ? "server" : "client", getNetAddress().toString());

      // If we're a server (ie, being connected to by the client) add this new connection to
      // our list of clients.
      if(!isInitiator())
         linkToClientList();
   }

   static void checkGameTimeouts()
   {
      // Look for people who have been connected longer than the threshold and
      // disconnect them.

      U32 currentTime = Platform::getRealMilliseconds();
      for(GameConnection *walk = gClientList.mNext; walk != &gClientList; walk = walk->mNext)
      {
         if(currentTime - walk->mCreateTime > 15000)
         {
            walk->disconnect("You're done!");
            break;
         }
      }
   }

   // Various things that should trigger us to try another server...

   void onConnectTerminated(NetConnection::TerminationReason, const char *)
   {
      GameConnectionDone();
   }

   void onConnectionTerminated(NetConnection::TerminationReason, const char *reason)
   {
      logprintf("Connection to remote host terminated - %s", reason);
      GameConnectionDone();
   }

   TNL_DECLARE_NETCONNECTION(GameConnection);
};

// Global list of clients (if we're a server).
GameConnection GameConnection::gClientList;

TNL_IMPLEMENT_NETCONNECTION(GameConnection, NetClassGroupGame, false);

//------------------------------------------------------------------------

class MasterServerConnection : public MasterServerInterface
{
   typedef MasterServerInterface Parent;

   // ID of our current query.
   U32 mCurrentQueryId;

   // List of game servers from our last query.
   Vector<IPAddress> mIPList;

public:
   MasterServerConnection()
   {
      mCurrentQueryId = 0;
      setIsConnectionToServer();
   }

   void startGameTypesQuery()
   {
      // Kick off a game types query.
      mCurrentQueryId++;
      c2mQueryGameTypes(mCurrentQueryId);
   }

   TNL_DECLARE_RPC_OVERRIDE(m2cQueryGameTypesResponse, (U32 queryId, Vector<StringTableEntry> gameTypes, Vector<StringTableEntry> missionTypes))
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
         mIPList.clear();

         // And automatically do a server query as well - you may not want to do things
         // in this order in your own clients.
         c2mQueryServers(mCurrentQueryId, 0xFFFFFFFF, 0, 128, 0, 128, 0, "", "");
      }
   }

   TNL_DECLARE_RPC_OVERRIDE(m2cQueryServersResponse, (U32 queryId, Vector<IPAddress> ipList))
   {
      // Only process results from current query...
      if(queryId != mCurrentQueryId)
         return;

      // Add the new results to our master result list...
      for(S32 i = 0; i < ipList.size(); i++)
         mIPList.push_back(ipList[i]);

      // if this was the last response, then echo out the list of servers,
      // and attempt to connect to one of them:
      if(ipList.size() == 0)
      {
         // Display the results...
         logprintf("got a list of servers from the master: %d servers", mIPList.size());
         for(S32 i = 0; i < mIPList.size(); i++)
            logprintf("  %s", Address(mIPList[i]).toString());

         // If we got anything...
         if(mIPList.size())
         {
            // pick a random server to connect to:
            U32 index = Random::readI(0, mIPList.size() - 1);

            // Invalidate the query...
            mCurrentQueryId++;

            // And request an arranged connnection (notice gratuitous hardcoded payload)
            c2mRequestArrangedConnection(mCurrentQueryId, mIPList[index],
               getInterface()->getFirstBoundInterfaceAddress().toIPAddress(),
               new ByteBuffer((U8 *) "Hello World!", 13));

            logprintf("Requesting arranged connection with %s", Address(mIPList[index]).toString());
         }
         else
         {
            logprintf("No game servers available... exiting.");
            gQuit = true;
         }
      }
   }

   TNL_DECLARE_RPC_OVERRIDE(m2cClientRequestedArrangedConnection, (U32 requestId, Vector<IPAddress> possibleAddresses,
      ByteBufferPtr connectionParameters))
   {
      if(!gIsServer || Random::readF() > 0.75)
      {
         // We reject connections about 75% of the time...

         logprintf("Rejecting arranged connection from %s", Address(possibleAddresses[0]).toString());
         c2mRejectArrangedConnection(requestId, connectionParameters);
      }
      else
      {
         // Ok, let's do the arranged connection!

         U8 data[Nonce::NonceSize * 2 + SymmetricCipher::KeySize * 2];
         Random::read(data, sizeof(data));
         IPAddress localAddress = getInterface()->getFirstBoundInterfaceAddress().toIPAddress();

         ByteBufferPtr b = new ByteBuffer(data, sizeof(data));
         b->takeOwnership();
         c2mAcceptArrangedConnection(requestId, localAddress, b);
         GameConnection *conn = new GameConnection();

         Vector<Address> fullPossibleAddresses;
         for(S32 i = 0; i < possibleAddresses.size(); i++)
            fullPossibleAddresses.push_back(Address(possibleAddresses[i]));

         logprintf("Accepting arranged connection from %s", Address(fullPossibleAddresses[0]).toString());

         logprintf("  Generated shared secret data: %s", b->encodeBase64()->getBuffer());

         ByteBufferPtr theSharedData = new ByteBuffer(data + 2 * Nonce::NonceSize, sizeof(data) - 2 * Nonce::NonceSize);
         theSharedData->takeOwnership();
         Nonce nonce(data);
         Nonce serverNonce(data + Nonce::NonceSize);

         conn->connectArranged(getInterface(), fullPossibleAddresses,
            nonce, serverNonce, theSharedData,false);
      }
   }

   TNL_DECLARE_RPC_OVERRIDE(m2cArrangedConnectionAccepted, (U32 requestId, Vector<IPAddress> possibleAddresses, ByteBufferPtr connectionData))
   {
      if(!gIsServer && requestId == mCurrentQueryId && connectionData->getBufferSize() >= Nonce::NonceSize * 2 + SymmetricCipher::KeySize * 2)
      {
         logprintf("Remote host accepted arranged connection.");
         logprintf("  Shared secret data: %s", connectionData->encodeBase64()->getBuffer());
         GameConnection *conn = new GameConnection();

         Vector<Address> fullPossibleAddresses;
         for(S32 i = 0; i < possibleAddresses.size(); i++)
            fullPossibleAddresses.push_back(Address(possibleAddresses[i]));

         ByteBufferPtr theSharedData =
                        new ByteBuffer(
                            (U8 *) connectionData->getBuffer() + Nonce::NonceSize * 2,
                            connectionData->getBufferSize() - Nonce::NonceSize * 2
                        );
         theSharedData->takeOwnership();

         Nonce nonce(connectionData->getBuffer());
         Nonce serverNonce(connectionData->getBuffer() + Nonce::NonceSize);

         conn->connectArranged(getInterface(), fullPossibleAddresses,
            nonce, serverNonce, theSharedData,true);
      }
   }

   TNL_DECLARE_RPC_OVERRIDE(m2cArrangedConnectionRejected, (U32 requestId, ByteBufferPtr rejectData))
   {
      if(!gIsServer && requestId == mCurrentQueryId)
      {
         logprintf("Remote host rejected arranged connection...");
         logprintf("Requesting new game types list.");
         startGameTypesQuery();
      }
   }
   void writeConnectRequest(BitStream *bstream)
   {
      Parent::writeConnectRequest(bstream);

      bstream->writeString("MasterServerTestGame"); // Game Name
      if(bstream->writeFlag(gIsServer))
      {
         bstream->write((U32) 1000);        // CPU speed
         bstream->write((U32) 0xFFFFFFFF);  // region code
         bstream->write((U32) 5);           // num bots
         bstream->write((U32) 10);          // num players
         bstream->write((U32) 32);          // max players
         bstream->write((U32) 0);           // info flags

         bstream->writeString(gCurrentGameType.getString());     // Game type
         bstream->writeString(gCurrentMissionType.getString());  // Mission type
      }
   }

   void onConnectionEstablished()
   {
      if(!gIsServer)
         startGameTypesQuery();
   }

   TNL_DECLARE_NETCONNECTION(MasterServerConnection);
};

TNL_IMPLEMENT_NETCONNECTION(MasterServerConnection, NetClassGroupMaster, false);

//------------------------------------------------------------------------

// This is called when we lose our connection to a game server.
void GameConnectionDone()
{
   // If we're still talking to the master server...
   if(gMasterServerConnection.isValid() &&
        gMasterServerConnection->getConnectionState() == NetConnection::Connected)
   {
      // Query again!
      gMasterServerConnection->startGameTypesQuery();
   }
   else
   {
       logprintf("GameConnectionDone: No connection to master server available - terminating!");
       gQuit = true;
   }
}

//------------------------------------------------------------------------

class StdoutLogConsumer : public LogConsumer
{
public:
   void logString(const char *string)
   {
      printf("%s\n", string);
   }
} gStdoutLogConsumer;

//------------------------------------------------------------------------

int main(int argc, const char **argv)
{

   // Parse command line arguments...
   if(argc < 2)
   {
      logprintf("Usage: masterclient <masterAddress> [-client|-server] [port]");
      return 0;
   }

   Address masterAddress(argv[1]);
   if(argc > 2)
      gIsServer = !stricmp(argv[2], "-server");
   U32 port = 0;
   if(argc > 3)
      port = atoi(argv[3]);

   // Announce master status.
   logprintf("%s started - master is at %s.", gIsServer ? "Server" : "Client", masterAddress.toString());

   // Initialize the random number generator.
   U32 value = Platform::getRealMilliseconds();
   Random::addEntropy((U8 *) &value, sizeof(U32));
   Random::addEntropy((U8 *) &value, sizeof(U32));
   Random::addEntropy((U8 *) &value, sizeof(U32));
   Random::addEntropy((U8 *) &value, sizeof(U32));

   // Open a network port...
   NetInterface *theInterface = new NetInterface(Address(IPProtocol, Address::Any, port));

   // And start processing.
   while(!gQuit)
   {
      // If we don't have a connection to the master server, then
      // create one and connect to it.
      if(!gMasterServerConnection.isValid() ||
         (gMasterServerConnection->getConnectionState() == NetConnection::TimedOut ||
          gMasterServerConnection->getConnectionState() == NetConnection::Disconnected))
      {
         logprintf("Connecting to master server at %s", masterAddress.toString());
         gMasterServerConnection = new MasterServerConnection;
         gMasterServerConnection->connect(theInterface, masterAddress);
      }

      // Make sure to process network traffic and connected clients...
      theInterface->checkIncomingPackets();
      theInterface->processConnections();
      GameConnection::checkGameTimeouts();

      // Sleep a bit so we don't saturate the system.
      Platform::sleep(1);
   }

   // All done!
   return 0;
}
