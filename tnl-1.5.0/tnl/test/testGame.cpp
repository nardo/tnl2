//-----------------------------------------------------------------------------------
//
//   Torque Network Library - TNLTest example program
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

#include <math.h>

#include "testGame.h"
#include "../tnl/tnlBitStream.h"
#include "../tnl/tnlNetConnection.h"
#include "../tnl/tnlRandom.h"
#include "../tnl/tnlSymmetricCipher.h"
#include "../tnl/tnlAsymmetricKey.h"

namespace TNLTest {

TestGame *clientGame = NULL;
TestGame *serverGame = NULL;

//---------------------------------------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(Player); 

// Player constructor
Player::Player(Player::PlayerType pt)
{
   // assign a random starting position for the player.
   startPos.x = TNL::Random::readF();
   startPos.y = TNL::Random::readF();
   endPos.x = startPos.x;
   endPos.y = startPos.y;
   renderPos = startPos;

   // preset the movement parameter for the player to the end of
   // the path
   t = 1.0;

   tDelta = 0;
   myPlayerType = pt;

   mNetFlags.set(Ghostable);
   game = NULL;
}

Player::~Player()
{
   if(!game)
      return;

   // remove the player from the list of players in the game.
   for( TNL::S32 i = 0; i < game->players.size(); i++)
   {
      if(game->players[i] == this)
      {
         game->players.erase_fast(i);
         return;
      }
   }
}

void Player::addToGame(TestGame *theGame)
{
   // add the player to the list of players in the game.
   theGame->players.push_back(this);
   game = theGame;

   if(myPlayerType == PlayerTypeMyClient)
   {
      // set the client player for the game for drawing the
      // scoping radius circle.
      game->clientPlayer = this;
   }
}

bool Player::onGhostAdd(TNL::GhostConnection *theConnection)
{
   addToGame(((TestNetInterface *) theConnection->getInterface())->game);
   return true;
}

void Player::performScopeQuery(TNL::GhostConnection *connection)
{
   // find all the objects that are "in scope" - for the purposes
   // of this test program, all buildings are considered to be in
   // scope always, as well as all "players" in a circle of radius
   // 0.25 around the scope object, or a radius squared of 0.0625

   for(TNL::S32 i = 0; i < game->buildings.size(); i++)
      connection->objectInScope(game->buildings[i]);

   for(TNL::S32 i = 0; i < game->players.size(); i++)
   {
      Position playerP = game->players[i]->renderPos;
      TNL::F32 dx = playerP.x - renderPos.x;
      TNL::F32 dy = playerP.y - renderPos.y;
      TNL::F32 distSquared = dx * dx + dy * dy;
      if(distSquared < 0.0625)
         connection->objectInScope(game->players[i]);
   }
}

TNL::U32 Player::packUpdate(TNL::GhostConnection *connection, TNL::U32 updateMask, TNL::BitStream *stream)
{
   // if this is the initial update, write out some static
   // information about this player.  Since we never call
   // setMaskBits(InitialMask), we're guaranteed that this state
   // will only be updated on the first update for this object.
   if(stream->writeFlag(updateMask & InitialMask))
   {
      // write one bit if it's not an AI
      if(stream->writeFlag(myPlayerType != PlayerTypeAI))
      {
         // write a flag to the client if this is the player that
         // that client controls.
         stream->writeFlag(connection->getScopeObject() == this);
      }
   }

   // see if we need to write out the position data
   // we must write a bit to let the unpackUpdate know whether to
   // read the position data out of the stream.

   if(stream->writeFlag(updateMask & PositionMask))
   {
      // since the position data is all 0 to 1, we can use
      // the bit stream's writeFloat method to write it out.
      // 12 bits should be sufficient precision
      stream->writeFloat(startPos.x, 12); 
      stream->writeFloat(startPos.y, 12); 
      stream->writeFloat(endPos.x, 12); 
      stream->writeFloat(endPos.y, 12); 
      stream->write(t); // fully precise t and tDelta
      stream->write(tDelta);
   }


   // if I had other states on the player that changed independently
   // of position, I could test for and update them here.
   // ...

   // the return value from packUpdate is the update mask for this
   // object on this client _AFTER_ this update has been sent.
   // Normally this will be zero, but it could be nonzero if this
   // object has a state that depends on some other object that
   // hasn't yet been ghosted.
   return 0;
}

void Player::unpackUpdate(TNL::GhostConnection *connection, TNL::BitStream *stream)
{
   // see if the initial packet data was written:
   if(stream->readFlag())
   {
      // check to see if it's not an AI player.
      if(stream->readFlag())
      {
         if(stream->readFlag())
            myPlayerType = PlayerTypeMyClient;
         else
            myPlayerType = PlayerTypeClient;
      }
      else
         myPlayerType = PlayerTypeAIDummy;
   }
   // see if the player's position has been updated:
   if(stream->readFlag())
   {
      startPos.x = stream->readFloat(12);
      startPos.y = stream->readFloat(12);
      endPos.x = stream->readFloat(12);
      endPos.y = stream->readFloat(12);
      stream->read(&t);
      stream->read(&tDelta);
      // update the render position
      update(0);
   }
}

void Player::serverSetPosition(Position inStartPos, Position inEndPos, TNL::F32 inT, TNL::F32 inTDelta)
{
   // update the instance variables of the object
   startPos = inStartPos;
   endPos = inEndPos;
   t = inT;
   tDelta = inTDelta;

   // notify the network system that the position state of this object has changed:
   setMaskBits(PositionMask);

   // call a quick RPC to all the connections that have this object in scope
   rpcPlayerDidMove(inEndPos.x, inEndPos.y);
}

void Player::update(TNL::F32 timeDelta)
{
   t += tDelta * timeDelta;
   if(t >= 1.0)
   {
      t = 1.0;
      tDelta = 0;
      renderPos = endPos;
      // if this is an AI player on the server, 
      if(myPlayerType == PlayerTypeAI)
      {
         startPos = renderPos;
         t = 0;
         endPos.x = TNL::Random::readF();  
         endPos.y = TNL::Random::readF();
         tDelta = 0.2f + TNL::Random::readF() * 0.1f;
         setMaskBits(PositionMask); // notify the network system that the network state has been updated
      }
   }
   renderPos.x = startPos.x + (endPos.x - startPos.x) * t;
   renderPos.y = startPos.y + (endPos.y - startPos.y) * t;
}

void Player::onGhostAvailable(TNL::GhostConnection *theConnection)
{
   // this function is called every time a ghost of this object is known
   // to be available on a given client.
   // we'll use this to demonstrate targeting a NetObject RPC 
   // to a specific connection.  Normally a RPC method marked as
   // RPCToGhost will be broadcast to ALL ghosts of the object currently in
   // scope.

   // first we construct an event that will represent the RPC call...
   // the RPC macros create a RPCNAME_construct function that returns
   // a NetEvent that encapsulates the RPC.
   TNL::NetEvent *theRPCEvent = TNL_RPC_CONSTRUCT_NETEVENT(this, rpcPlayerIsInScope, (renderPos.x, renderPos.y));

   // then we can just post the event to whatever connections we want to have
   // receive the message.
   // In the case of NetObject RPCs, if the source object is not ghosted,
   // the RPC will just be silently dropped.
   theConnection->postNetEvent(theRPCEvent);
}

/*
class RPCEV_Player_rpcPlayerIsInScope : 
   public TNL::NetObjectRPCEvent 
{ 
public: TNL::FunctorDecl<void (Player::*)(TNL::Float<6> x, TNL::Float<6> y)> mFunctorDecl; 
        RPCEV_Player_rpcPlayerIsInScope() : 
        mFunctorDecl(Player::rpcPlayerIsInScope_remote), 
           RPCEvent(TNL::RPCGuaranteedOrdered, TNL::RPCToGhost) 
        { mFunctor = &mFunctorDecl; } 
        static TNL::NetClassRepInstance<RPCEV_Player_rpcPlayerIsInScope> dynClassRep; 
        virtual TNL::NetClassRep* getClassRep() const; 
        bool checkClassType(TNL::Object *theObject) 
        { return dynamic_cast<Player *>(theObject) != __null; } 
}; 
TNL::NetClassRep* RPCEV_Player_rpcPlayerIsInScope::getClassRep() 
const { return &RPCEV_Player_rpcPlayerIsInScope::dynClassRep; } 
TNL::NetClassRepInstance<RPCEV_Player_rpcPlayerIsInScope> RPCEV_Player_rpcPlayerIsInScope::dynClassRep("RPCEV_Player_rpcPlayerIsInScope",TNL::NetClassGroupGameMask, TNL::NetClassTypeEvent, 0); 
void Player::rpcPlayerIsInScope (TNL::Float<6> x, TNL::Float<6> y) 
{ 
   RPCEV_Player_rpcPlayerIsInScope *theEvent = new RPCEV_Player_rpcPlayerIsInScope; 
   theEvent->mFunctorDecl.set (x, y) ; 
   postNetEvent(theEvent); 
} 

TNL::NetEvent * Player::rpcPlayerIsInScope_construct (TNL::Float<6> x, TNL::Float<6> y) { RPCEV_Player_rpcPlayerIsInScope *theEvent = new RPCEV_Player_rpcPlayerIsInScope; theEvent->mFunctorDecl.set (x, y) ; return theEvent; } void Player::rpcPlayerIsInScope_test (TNL::Float<6> x, TNL::Float<6> y) { RPCEV_Player_rpcPlayerIsInScope *theEvent = new RPCEV_Player_rpcPlayerIsInScope; theEvent->mFunctorDecl.set (x, y) ; TNL::PacketStream ps; theEvent->pack(this, &ps); ps.setBytePosition(0); theEvent->unpack(this, &ps); theEvent->process(this); } void Player::rpcPlayerIsInScope_remote (TNL::Float<6> x, TNL::Float<6> y)


{
   TNL::F32 fx = x, fy = y;
   TNL::logprintf("A player is now in scope at %g, %g", fx, fy);
}

*/
TNL_IMPLEMENT_NETOBJECT_RPC(Player, rpcPlayerIsInScope,
   (TNL::Float<6> x, TNL::Float<6> y), (x, y),
   TNL::NetClassGroupGameMask, TNL::RPCGuaranteedOrdered, TNL::RPCToGhost, 0)
{
   TNL::F32 fx = x, fy = y;
   TNL::logprintf("A player is now in scope at %g, %g", fx, fy);
}

TNL_IMPLEMENT_NETOBJECT_RPC(Player, rpcPlayerWillMove,
                            (TNL::StringPtr testString), (testString), 
   TNL::NetClassGroupGameMask, TNL::RPCGuaranteedOrdered, TNL::RPCToGhostParent, 0)
{
   TNL::logprintf("Expecting a player move from the connection: %s", testString.getString());
}

TNL_IMPLEMENT_NETOBJECT_RPC(Player, rpcPlayerDidMove,
   (TNL::Float<6> x, TNL::Float<6> y), (x, y),
   TNL::NetClassGroupGameMask, TNL::RPCGuaranteedOrdered, TNL::RPCToGhost, 0)
{
   TNL::F32 fx = x, fy = y;
   TNL::logprintf("A player moved to %g, %g", fx, fy);
}

//---------------------------------------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(Building);

Building::Building()
{
   // place the "building" in a random position on the screen
   upperLeft.x = TNL::Random::readF();
   upperLeft.y = TNL::Random::readF();

   lowerRight.x = upperLeft.x + TNL::Random::readF() * 0.1f + 0.025f;
   lowerRight.y = upperLeft.y + TNL::Random::readF() * 0.1f + 0.025f;

   game = NULL;
   
   // always scope the buildings to the clients
   mNetFlags.set(Ghostable);
}

Building::~Building()
{
   if(!game)
      return;

   // remove the building from the list of buildings in the game.
   for( TNL::S32 i = 0; i < game->buildings.size(); i++)
   {
      if(game->buildings[i] == this)
      {
         game->buildings.erase_fast(i);
         return;
      }
   }   
}

void Building::addToGame(TestGame *theGame)
{
   // add it to the list of buildings in the game
   theGame->buildings.push_back(this);
   game = theGame;
}

bool Building::onGhostAdd(TNL::GhostConnection *theConnection)
{
   addToGame(((TestNetInterface *) theConnection->getInterface())->game);
   return true;
}

TNL::U32 Building::packUpdate(TNL::GhostConnection *connection, TNL::U32 updateMask, TNL::BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
   {
      // we know all the positions are 0 to 1.
      // since this object is scope always, we don't care quite as much
      // how efficient the initial state updating is, since we're only
      // ever going to send this data when the client first connects
      // to this mission.
      stream->write(upperLeft.x);
      stream->write(upperLeft.y);
      stream->write(lowerRight.x);
      stream->write(lowerRight.y);
   }
   // for later - add a "color" field to the buildings that can change
   // when AIs or players move over them.

   return 0;
}

void Building::unpackUpdate(TNL::GhostConnection *connection, TNL::BitStream *stream)
{
   if(stream->readFlag())
   {
      stream->read(&upperLeft.x);
      stream->read(&upperLeft.y);
      stream->read(&lowerRight.x);
      stream->read(&lowerRight.y);
   }  
}

//---------------------------------------------------------------------------------

TNL_IMPLEMENT_NETCONNECTION(TestConnection, TNL::NetClassGroupGame, true);

TNL_IMPLEMENT_RPC(TestConnection, rpcGotPlayerPos,
      (bool b1, bool b2, TNL::StringTableEntry string, TNL::Float<TestConnection::PlayerPosReplyBitSize> x, TNL::Float<TestConnection::PlayerPosReplyBitSize> y), (b1, b2, string, x, y),
      TNL::NetClassGroupGameMask, TNL::RPCGuaranteedOrdered, TNL::RPCDirAny, 0)
{
   TNL::F32 xv = x, yv = y;
   TNL::logprintf("Server acknowledged position update - %d %d %s %g %g", b1, b2, string.getString(), xv, yv);
}

TNL_IMPLEMENT_RPC(TestConnection, rpcSetPlayerPos, 
      (TNL::F32 x, TNL::F32 y), (x, y),
      TNL::NetClassGroupGameMask, TNL::RPCGuaranteedOrdered, TNL::RPCDirClientToServer, 0)
{
   Position newPosition;
   newPosition.x = x;
   newPosition.y = y;
     
   TNL::logprintf("%s - received new position (%g, %g) from client", 
               getNetAddressString(), 
               newPosition.x, newPosition.y);

   myPlayer->serverSetPosition(myPlayer->renderPos, newPosition, 0, 0.2f);
   
   // send an RPC back the other way!
   TNL::StringTableEntry helloString("Hello World!!");
   rpcGotPlayerPos(true, false, helloString, x, y);
};

TestConnection::TestConnection()
{
   // every connection class instance must have a net class group set
   // before it is used.

   //setIsAdaptive(); // <-- Uncomment me if you want to use adaptive rate instead of fixed rate...
}

bool TestConnection::isDataToTransmit()
{
   // we always want packets to be sent.
   return true;
}

void TestConnection::onConnectTerminated(NetConnection::TerminationReason reason, const char *string)
{
   ((TestNetInterface *) getInterface())->pingingServers = true;
}

void TestConnection::onConnectionTerminated(NetConnection::TerminationReason reason, const char *edString)
{
   logprintf("%s - %s connection terminated - reason %d.", getNetAddressString(), isConnectionToServer() ? "server" : "client", reason);

   if(isConnectionToServer())
      ((TestNetInterface *) getInterface())->pingingServers = true;
   else
      delete (Player*)myPlayer;
}

void TestConnection::onConnectionEstablished()
{
   // call the parent's onConnectionEstablished.
   // by default this will set the initiator to be a connection
   // to "server" and the non-initiator to be a connection to "client"
   Parent::onConnectionEstablished();

   // To see how this program performs with 50% packet loss,
   // Try uncommenting the next line :)
   //setSimulatedNetParams(0.5, 0);
   
   if(isInitiator())
   {
      setGhostFrom(false);
      setGhostTo(true);
      TNL::logprintf("%s - connected to server.", getNetAddressString());
      ((TestNetInterface *) getInterface())->connectionToServer = this;
   }
   else
   {
      // on the server, we create a player object that will be the scope object
      // for this client.
      Player *player = new Player;
      myPlayer = player;
      myPlayer->addToGame(((TestNetInterface *) getInterface())->game);
      setScopeObject(myPlayer);
      setGhostFrom(true);
      setGhostTo(false);
      activateGhosting();
      TNL::logprintf("%s - client connected.", getNetAddressString());
   }
}

//---------------------------------------------------------------------------------

TestNetInterface::TestNetInterface(TestGame *theGame, bool server, const TNL::Address &bindAddress, const TNL::Address &pingAddr) : NetInterface(bindAddress)
{
   game           = theGame;
   isServer       = server;
   pingingServers = !server;
   lastPingTime   = 0;
   pingAddress    = pingAddr;
}

void TestNetInterface::sendPing()
{
   TNL::PacketStream writeStream;

   // the ping packet right now just has one byte for packet type
   writeStream.write(TNL::U8(GamePingRequest));

   writeStream.sendto(mSocket, pingAddress);

   TNL::logprintf("%s - sending ping.", pingAddress.toString());
}

void TestNetInterface::tick()
{
   TNL::U32 currentTime = TNL::Platform::getRealMilliseconds();

   if(pingingServers && (lastPingTime + PingDelayTime < currentTime))
   {
      lastPingTime = currentTime;
      sendPing();
   }
   checkIncomingPackets();
   processConnections();
}

// handleInfoPacket for the test game is very simple - if this instance
// is a client, it pings for servers until one is found to connect to.
// If this instance is a server, it responds to ping packets from clients.
// More complicated games could maintain server lists, request game information
// and more.

void TestNetInterface::handleInfoPacket(const TNL::Address &address, TNL::U8 packetType, TNL::BitStream *stream)
{
   TNL::PacketStream writeStream;
   if(packetType == GamePingRequest && isServer)
   {
      TNL::logprintf("%s - received ping.", address.toString());
      // we're a server, and we got a ping packet from a client,
      // so send back a GamePingResponse to let the client know it
      // has found a server.
      writeStream.write(TNL::U8(GamePingResponse));
      writeStream.sendto(mSocket, address);

      TNL::logprintf("%s - sending ping response.", address.toString());
   }
   else if(packetType == GamePingResponse && pingingServers)
   {
      // we were pinging servers and we got a response.  Stop the server
      // pinging, and try to connect to the server.

      TNL::logprintf("%s - received ping response.", address.toString());

      TestConnection *connection = new TestConnection;
      connection->connect(this, address); // connect to the server through the game's network interface

      TNL::logprintf("Connecting to server: %s", address.toString());

      pingingServers = false;
   }
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

TestGame::TestGame(bool server, const TNL::Address &interfaceBindAddr, const TNL::Address &pingAddress)
{
   isServer = server;
   myNetInterface = new TestNetInterface(this, isServer, interfaceBindAddr, pingAddress);
   TNL::AsymmetricKey *theKey = new TNL::AsymmetricKey(32);
   myNetInterface->setPrivateKey(theKey);
   myNetInterface->setRequiresKeyExchange(true);

   lastTime = TNL::Platform::getRealMilliseconds();

   if(isServer)
   {
      // generate some buildings and AIs:
      for(TNL::S32 i = 0; i < 50; i ++)
      {
         Building *building = new Building;
         building->addToGame(this);
      }
      for(TNL::S32 i = 0; i < 15; i ++)
      {
         Player *aiPlayer = new Player(Player::PlayerTypeAI);
         aiPlayer->addToGame(this);
      }
      serverPlayer = new Player(Player::PlayerTypeMyClient);
      serverPlayer->addToGame(this);
   }

   TNL::logprintf("Created a %s...", (server ? "server" : "client"));
}

TestGame::~TestGame()
{
   delete myNetInterface;
   for(TNL::S32 i = 0; i < buildings.size(); i++)
      delete buildings[i];
   for(TNL::S32 i = 0; i < players.size(); i++)
      delete players[i];

   TNL::logprintf("Destroyed a %s...", (this->isServer ? "server" : "client"));
}

void TestGame::createLocalConnection(TestGame *serverGame)
{
   myNetInterface->pingingServers = false;
   TestConnection *theConnection = new TestConnection;
   theConnection->connectLocal(myNetInterface, serverGame->myNetInterface);
}

void TestGame::tick()
{
   TNL::U32 currentTime = TNL::Platform::getRealMilliseconds();
   if(currentTime == lastTime)
      return;

   TNL::F32 timeDelta = (currentTime - lastTime) / 1000.0f;
   for(TNL::S32 i = 0; i < players.size(); i++)  
      players[i]->update(timeDelta);
   myNetInterface->tick();
   lastTime = currentTime;
}

void TestGame::moveMyPlayerTo(Position newPosition)
{
   if(isServer)
   {
      serverPlayer->serverSetPosition(serverPlayer->renderPos, newPosition, 0, 0.2f);
   }
   else if(!myNetInterface->connectionToServer.isNull())
   {
      TNL::logprintf("posting new position (%g, %g) to server", newPosition.x, newPosition.y);
      if(!clientPlayer.isNull())
         clientPlayer->rpcPlayerWillMove("Whee! Foo!");
      myNetInterface->connectionToServer->rpcSetPlayerPos(newPosition.x, newPosition.y);
   }
}

};
