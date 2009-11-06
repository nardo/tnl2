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

#include "tnlNetInterface.h"
#include "masterInterface.h"
#include "tnlVector.h"
#include "tnlAsymmetricKey.h"

using namespace TNL;

NetInterface *gNetInterface = NULL;
class MissionGameType : public Object
{
public:
   MissionGameType(const StringTableEntry &name) { mName = name; }
   StringTableEntry mName;
};

Vector<char *> MOTDTypeVec;
Vector<char *> MOTDStringVec;


class MasterServerConnection;

class GameConnectRequest
{
public:
   SafePtr<MasterServerConnection> initiator;
   SafePtr<MasterServerConnection> host;

   U32 initiatorQueryId;
   U32 hostQueryId;
   U32 requestTime;
};

class MasterServerConnection : public MasterServerInterface
{
private:
   typedef MasterServerInterface Parent;
protected:

   /// @name Linked List
   ///
   /// The server stores its connections on a linked list.
   ///
   /// @{

   ///
   MasterServerConnection *mNext;
   MasterServerConnection *mPrev;

   /// @}

   /// @name Globals
   /// @{

   ///
   static MasterServerConnection             gServerList;
   static Vector< SafePtr<MissionGameType> > gMissionTypeList;
   static Vector< SafePtr<MissionGameType> > gGameTypeList;
   static Vector< GameConnectRequest* >      gConnectList;


   /// @}


   /// @name Connection Info
   ///
   /// General information about this connection.
   ///
   /// @{

   ///
   bool             mIsGameServer;     ///< True if this is a game server.
   U32              mStrikeCount;      ///< Number of "strikes" this connection has... 3 strikes and you're out!
   U32              mLastQueryId;      ///< The last query id for info from this master.
   U32              mLastActivityTime; ///< The last time we got a request or an update from this host.

   /// A list of connection requests we're working on fulfilling for this connection.
   Vector< GameConnectRequest* > mConnectList;

   /// @}

   /// @name Server Info
   ///
   /// This info is filled in if this connection maps to a
   /// game server.
   ///
   /// @{

   U32              mRegionCode;       ///< The region code in which this server operates.
   StringTableEntry mGameString;       ///< The unique game string for this server or client.
   U32              mCPUSpeed;         ///< The CPU speed of this server.
   U32              mInfoFlags;        ///< Info flags describing this server.
   U32              mPlayerCount;      ///< Current number of players on this server.
   U32              mMaxPlayers;       ///< Maximum number of players on this server.
   U32              mNumBots;          ///< Current number of bots on this server.
   RefPtr<MissionGameType> mCurrentGameType;
   RefPtr<MissionGameType> mCurrentMissionType;



   void setGameType(const StringTableEntry &gameType)
   {
      for(S32 i = 0; i < gGameTypeList.size(); i++)
      {
         if(gGameTypeList[i].isValid() && gGameTypeList[i]->mName == gameType)
         {
            mCurrentGameType = gGameTypeList[i];
            return;
         }
      }
      mCurrentGameType = new MissionGameType(gameType);
      gGameTypeList.push_back((MissionGameType *)mCurrentGameType);
   }

   void setMissionType(const StringTableEntry &missionType)
   {
      for(S32 i = 0; i < gMissionTypeList.size(); i++)
      {
         if(gMissionTypeList[i].isValid() && gMissionTypeList[i]->mName == missionType)
         {
            mCurrentMissionType = gMissionTypeList[i];
            return;
         }
      }
      mCurrentMissionType = new MissionGameType(missionType);
      gMissionTypeList.push_back((MissionGameType *) mCurrentMissionType);
   }

   /// @}

public:

   /// Constructor initializes the linked list info  with
   /// "safe" values so we don't explode if we destruct
   /// right away.
   MasterServerConnection()
   {
      mStrikeCount = 0;
      mLastActivityTime = 0;
      mNext = this;
      mPrev = this;
      setIsConnectionToClient();
      setIsAdaptive();
   }

   /// Destructor removes the connection from the doubly linked list of
   /// server connections.
   ~MasterServerConnection()
   {
      // unlink it if it's in the list
      mPrev->mNext = mNext;
      mNext->mPrev = mPrev;
      logprintf("%s disconnected", getNetAddress().toString());
   }

   /// Adds this connection to the doubly linked list of servers.
   void linkToServerList()
   {
      mNext = gServerList.mNext;
      mPrev = gServerList.mNext->mPrev;
      mNext->mPrev = this;
      mPrev->mNext = this;
   }

   /// RPC's a list of mission and game types to the requesting client.
   /// This function also cleans up any game types from the global lists
   /// that are no longer referenced.
   TNL_DECLARE_RPC_OVERRIDE(c2mQueryGameTypes, (U32 queryId))
   {
      Vector<StringTableEntry> gameTypes(GameMissionTypesPerPacket);
      Vector<StringTableEntry> missionTypes(GameMissionTypesPerPacket);
      U32 listSize = 0;

      // Iterate through game types list, culling out any null entries.
      // Add all non-null entries to the gameTypes vector.
      for(S32 i = 0; i < gGameTypeList.size(); )
      {
         if(gGameTypeList[i].isNull())
         {
            gGameTypeList.erase_fast(i);
            continue;
         }

         gameTypes.push_back(gGameTypeList[i]->mName);
         i++;

         listSize++;
         if(listSize >= GameMissionTypesPerPacket)
         {
            m2cQueryGameTypesResponse(queryId, gameTypes, missionTypes);
            listSize = 0;
            gameTypes.clear();
         }
      }


      // Iterate through mission types list, culling out any null entries.
      // Add all non-null entries to the missionTypes vector.
      for(S32 i = 0; i < gMissionTypeList.size(); )
      {
         if(gMissionTypeList[i].isNull())
         {
            gMissionTypeList.erase_fast(i);
            continue;
         }
         missionTypes.push_back(gMissionTypeList[i]->mName);
         i++;
         listSize++;
         if(listSize >= GameMissionTypesPerPacket)
         {
            m2cQueryGameTypesResponse(queryId, gameTypes, missionTypes);
            listSize = 0;
            gameTypes.clear();
            missionTypes.clear();
         }
      }

      // Send the last lists to the client.
      m2cQueryGameTypesResponse(queryId, gameTypes, missionTypes);

      // Send a pair of empty lists to the client to signify that the
      // query is done.
      if(gameTypes.size() || missionTypes.size())
      {
         gameTypes.clear();
         missionTypes.clear();
         m2cQueryGameTypesResponse(queryId, gameTypes, missionTypes);
      }
   }

   /// The query server method builds a piecewise list of servers
   /// that match the client's particular filter criteria and
   /// sends it to the client, followed by a QueryServersDone RPC.
   TNL_DECLARE_RPC_OVERRIDE(c2mQueryServers,
                (U32 queryId, U32 regionMask, U32 minPlayers, U32 maxPlayers,
                 U32 infoFlags, U32 maxBots, U32 minCPUSpeed,
                 StringTableEntry gameType, StringTableEntry missionType)
   )
   {
      Vector<IPAddress> theVector(IPMessageAddressCount);
      theVector.reserve(IPMessageAddressCount);

      for(MasterServerConnection *walk = gServerList.mNext; walk != &gServerList; walk = walk->mNext)
      {
         // Skip to the next if we don't match on any particular...
         if(walk->mGameString != mGameString)
            continue;
         if(!(walk->mRegionCode & regionMask))
            continue;
         if(walk->mPlayerCount > maxPlayers || walk->mPlayerCount < minPlayers)
            continue;
         if(infoFlags & ~walk->mInfoFlags)
            continue;
         if(maxBots < walk->mNumBots)
            continue;
         if(minCPUSpeed > walk->mCPUSpeed)
            continue;
         if(gameType.isNotNull() && (gameType != walk->mCurrentGameType->mName))
            continue;
         if(missionType.isNotNull() && (missionType != walk->mCurrentMissionType->mName))
            continue;

         // Somehow we matched! Add us to the results list.
         theVector.push_back(walk->getNetAddress().toIPAddress());

         // If we get a packet's worth, send it to the client and empty our buffer...
         if(theVector.size() == IPMessageAddressCount)
         {
            m2cQueryServersResponse(queryId, theVector);
            theVector.clear();
         }
      }
      m2cQueryServersResponse(queryId, theVector);
      // If we sent any with the previous message, send another list with no servers.
      if(theVector.size())
      {
         theVector.clear();
         m2cQueryServersResponse(queryId, theVector);
      }
   }

   /// checkActivityTime validates that this particular connection is
   /// not issuing too many requests at once in an attempt to DOS
   /// by flooding either the master server or any other server
   /// connected to it.  A client whose last activity time falls
   /// within the specified delta gets a strike... 3 strikes and
   /// you're out!  Strikes go away after being good for a while.
   void checkActivityTime(U32 timeDeltaMinimum)
   {
      U32 currentTime = Platform::getRealMilliseconds();
      if(currentTime - mLastActivityTime < timeDeltaMinimum)
      {
         mStrikeCount++;
         if(mStrikeCount == 3)
            disconnect("You're out!");
      }
      else if(mStrikeCount > 0)
         mStrikeCount--;
   }

   void removeConnectRequest(GameConnectRequest *gcr)
   {
      for(S32 j = 0; j < mConnectList.size(); j++)
      {
         if(gcr == mConnectList[j])
         {
            mConnectList.erase_fast(j);
            break;
         }
      }
   }

   GameConnectRequest *findAndRemoveRequest(U32 requestId)
   {
      GameConnectRequest *req = NULL;
      for(S32 j = 0; j < mConnectList.size(); j++)
      {
         if(mConnectList[j]->hostQueryId == requestId)
         {
            req = mConnectList[j];
            mConnectList.erase_fast(j);
            break;
         }
      }
      if(!req)
         return NULL;

      if(req->initiator.isValid())
         req->initiator->removeConnectRequest(req);
      for(S32 j = 0; j < gConnectList.size(); j++)
      {
         if(gConnectList[j] == req)
         {
            gConnectList.erase_fast(j);
            break;
         }
      }
      return req;
   }

   // This is called when a client wishes to arrange a connection with a
   // server.
   TNL_DECLARE_RPC_OVERRIDE(c2mRequestArrangedConnection, (U32 requestId,
      IPAddress remoteAddress, IPAddress internalAddress,
      ByteBufferPtr connectionParameters))
   {
      // First, make sure that we're connected with the server that they're requesting a connection with.
      MasterServerConnection *conn = (MasterServerConnection *) gNetInterface->findConnection(remoteAddress);
      if(!conn)
      {
         ByteBufferPtr ptr = new ByteBuffer((U8 *) MasterNoSuchHost, strlen(MasterNoSuchHost) + 1);
         c2mRejectArrangedConnection(requestId, ptr);
         return;
      }

      // Record the request...
      GameConnectRequest *req = new GameConnectRequest;
      req->initiator = this;
      req->host = conn;
      req->initiatorQueryId = requestId;
      req->hostQueryId = mLastQueryId++;
      req->requestTime = Platform::getRealMilliseconds();

      char buf[256];
      strcpy(buf, getNetAddress().toString());
      logprintf("Client: %s requested connection to %s",
         buf, conn->getNetAddress().toString());

      // Add the request to the relevant lists (the global list, this connection's list,
      // and the other connection's list).
      mConnectList.push_back(req);
      conn->mConnectList.push_back(req);
      gConnectList.push_back(req);

      // Do some DOS checking...
      checkActivityTime(2000);

      // Get our address...
      Address theAddress = getNetAddress();

      // Record some different addresses to try...
      Vector<IPAddress> possibleAddresses;

      // The address, but port+1
      theAddress.port++;
      possibleAddresses.push_back(theAddress.toIPAddress());

      // The address, with the original port
      theAddress.port--;
      possibleAddresses.push_back(theAddress.toIPAddress());

      // Or the address the port thinks it's talking to.
      Address theInternalAddress(internalAddress);
      Address anyAddress;

      // (Only store that last one if it's not the any address.)
      if(!theInternalAddress.isEqualAddress(anyAddress) && theInternalAddress != theAddress)
         possibleAddresses.push_back(internalAddress);

      // And inform the other part of the request.
      conn->m2cClientRequestedArrangedConnection(req->hostQueryId, possibleAddresses, connectionParameters);
   }

   // Called to indicate a connect request is being accepted.
   TNL_DECLARE_RPC_OVERRIDE(c2mAcceptArrangedConnection, (U32 requestId, IPAddress internalAddress, ByteBufferPtr connectionData))
   {
      GameConnectRequest *req = findAndRemoveRequest(requestId);
      if(!req)
         return;

      Address theAddress = getNetAddress();
      Vector<IPAddress> possibleAddresses;
      theAddress.port++;
      possibleAddresses.push_back(theAddress.toIPAddress());
      theAddress.port--;
      possibleAddresses.push_back(theAddress.toIPAddress());
      Address theInternalAddress(internalAddress);
      Address anyAddress;

      if(!theInternalAddress.isEqualAddress(anyAddress) && theInternalAddress != theAddress)
         possibleAddresses.push_back(internalAddress);

      char buffer[256];
      strcpy(buffer, getNetAddress().toString());

      logprintf("Server: %s accept connection request from %s", buffer,
         req->initiator.isValid() ? req->initiator->getNetAddress().toString() : "Unknown");

      // If we still know about the requestor, tell him his connection was accepted...
      if(req->initiator.isValid())
         req->initiator->m2cArrangedConnectionAccepted(req->initiatorQueryId, possibleAddresses, connectionData);

      delete req;
   }


   // Called to indicate a connect request is being rejected.
   TNL_DECLARE_RPC_OVERRIDE(c2mRejectArrangedConnection, (U32 requestId, ByteBufferPtr rejectData))
   {
      GameConnectRequest *req = findAndRemoveRequest(requestId);
      if(!req)
         return;

      logprintf("Server: %s reject connection request from %s",
         getNetAddress().toString(),
         req->initiator.isValid() ? req->initiator->getNetAddress().toString() : "Unknown");

      if(req->initiator.isValid())
         req->initiator->m2cArrangedConnectionRejected(req->initiatorQueryId, rejectData);

      delete req;
   }

   // Called to update the status of a game server.
   TNL_DECLARE_RPC_OVERRIDE(c2mUpdateServerStatus, (
      StringTableEntry gameType, StringTableEntry missionType,
      U32 botCount, U32 playerCount, U32 maxPlayers, U32 infoFlags))
   {
      // If we didn't know we were a game server, don't accept updates.
      if(!mIsGameServer)
         return;

      setGameType(gameType);
      setMissionType(missionType);
      mNumBots = botCount;
      mPlayerCount = playerCount;
      mMaxPlayers = maxPlayers;
      mInfoFlags = infoFlags;

      checkActivityTime(15000);

      logprintf("Server: %s updated server status (%s, %s, %d, %d, %d)", getNetAddress().toString(), gameType.getString(), missionType.getString(), botCount, playerCount, maxPlayers);
   }

   bool readConnectRequest(BitStream *bstream, const char **errorString)
   {
      if(!Parent::readConnectRequest(bstream, errorString))
         return false;

      char gameString[256];
      bstream->readString(gameString);
      mGameString = gameString;

      // If it's a game server, read status info...
      if((mIsGameServer = bstream->readFlag()) == true)
      {
         bstream->read(&mCPUSpeed);
         bstream->read(&mRegionCode);
         bstream->read(&mNumBots);
         bstream->read(&mPlayerCount);
         bstream->read(&mMaxPlayers);
         bstream->read(&mInfoFlags);
         bstream->readString(gameString);

         setGameType(StringTableEntry(gameString));
         bstream->readString(gameString);
         setMissionType(StringTableEntry(gameString));

         linkToServerList();
      }
      logprintf("%s online at %s", mIsGameServer ? "Server" : "client", getNetAddress().toString());
      if(getEventClassVersion() > 0)
      {
         U32 matchLen = 0;
         const char *motdString = "Welcome to TNL.  Have a nice day.";
         for(S32 i = 0; i < MOTDTypeVec.size(); i++)
         {
            U32 len;
            const char *type = MOTDTypeVec[i];
            for(len = 0; type[len] == gameString[len] && type[len] != 0; len++)
               ;
            if(len > matchLen)
            {
               matchLen = len;
               motdString = MOTDStringVec[i];
            }
         }
         m2cSetMOTD(motdString);
      }
      return true;
   }

   static void checkConnectTimeouts()
   {
      U32 currentTime = Platform::getRealMilliseconds();

      // Expire any connect requests that have grown old...
      for(S32 i = 0; i < gConnectList.size(); )
      {
         GameConnectRequest *gcr = gConnectList[i];
         if(currentTime - gcr->requestTime > ConnectRequestTimeout)
         {
            // It's old!

            // So remove it from the initiator's list...
            if(gcr->initiator.isValid())
            {
               gcr->initiator->removeConnectRequest(gcr);
               ByteBufferPtr reqTimeoutBuffer = new ByteBuffer((U8 *) MasterRequestTimedOut, strlen(MasterRequestTimedOut) + 1);
               gcr->initiator->c2mRejectArrangedConnection(gcr->initiatorQueryId, reqTimeoutBuffer);
            }

            // And the host's lists..
            if(gcr->host.isValid())
               gcr->host->removeConnectRequest(gcr);

            // Delete it...
            delete gcr;

            // And remove it from our list, too.
            gConnectList.erase_fast(i);
            continue;
         }

         i++;
      }
   }

   TNL_DECLARE_NETCONNECTION(MasterServerConnection);
};

TNL_IMPLEMENT_NETCONNECTION(MasterServerConnection, NetClassGroupMaster, true);

Vector< SafePtr<MissionGameType> > MasterServerConnection::gMissionTypeList;
Vector< SafePtr<MissionGameType> > MasterServerConnection::gGameTypeList;
Vector< GameConnectRequest* > MasterServerConnection::gConnectList;

MasterServerConnection MasterServerConnection::gServerList;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

#include <stdio.h>

class StdoutLogConsumer : public LogConsumer
{
public:
   void logString(const char *string)
   {
      printf("%s\n", string);
   }
} gStdoutLogConsumer;

enum {
   DefaultMasterPort = 29005,
};

U32 gMasterPort = DefaultMasterPort;
extern void readConfigFile();

int main(int argc, const char **argv)
{
   // Parse command line parameters... 
   readConfigFile();

   // Initialize our net interface so we can accept connections...
   gNetInterface = new NetInterface(Address(IPProtocol, Address::Any, gMasterPort));

   //for the master server alone, we don't need a key exchange - that would be a waste
   //gNetInterface->setRequiresKeyExchange(true);
   //gNetInterface->setPrivateKey(new AsymmetricKey(20));

   logprintf("Master Server created - listening on port %d", gMasterPort);

   // And until infinity, process whatever comes our way.
   U32 lastConfigReadTime = Platform::getRealMilliseconds();

   for(;;)
   {
      U32 currentTime = Platform::getRealMilliseconds();
      gNetInterface->checkIncomingPackets();
      gNetInterface->processConnections();
      if(currentTime - lastConfigReadTime > 5000)
      {
         lastConfigReadTime = currentTime;
         readConfigFile();
      }
      Platform::sleep(1);
   }
   return 0;
}
