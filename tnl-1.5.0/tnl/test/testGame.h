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

#include "../tnl/tnl.h"
#include "../tnl/tnlNetBase.h"
#include "../tnl/tnlGhostConnection.h"
#include "../tnl/tnlNetInterface.h"
#include "../tnl/tnlNetObject.h"
#include "../tnl/tnlLog.h"
#include "../tnl/tnlRPC.h"
#include "../tnl/tnlString.h"

/// TNL Graphical Test Application.
///
/// The TNLTest application demonstrates some of the more useful
/// features of the Torque Network Library.  The application presents
/// a single window, representing an abstract simulation area.  Red
/// rectangles are used to represent "buildings" that small squares
/// representing players move over.
///
/// On the linux and Win32 platforms, TNLTest uses the wxWindows user
/// interface API.  On Mac OSX TNLTest uses the native Cocoa application
/// framework to render.
///
/// Each instance of TNLTest can run in one of four modes: as a server,
/// able to host multiple clients; as a client, which searches for and then
/// connects to a server for game data; a client pinging the localhost for
/// connecting to a server on the local machine, and as a combined server
/// and client.
namespace TNLTest
{

class TestGame;

/// Position structure used for positions of objects in the test game.
struct Position
{
   TNL::F32 x; ///< X position of the object, from 0 ... 1
   TNL::F32 y; ///< Y position of the object, from 0 ... 1
};

/// The Player is an example of an object that can move around and update its 
/// position to clients in the system.  Each client in the server controls
/// a Player instance constructed for it upon joining the game.
/// Client users click on the game window, which generates an event to the server to 
/// move that client's player to the desired point.
class Player : public TNL::NetObject
{
   typedef TNL::NetObject Parent;
public:
   Position startPos, endPos; ///< All players move along a line from startPos to endPos.
   Position renderPos;        ///< Position at which to render the player - computed during update().
   TNL::F32 t;                     ///< Parameter of how far the player is along the line from startPos to endPos.
   TNL::F32 tDelta;                ///< Change in t per second (ie, velocity).
   TestGame *game;            ///< The game object this player is associated with

   /// Enumeration of possible player types in the game.
   enum PlayerType {
      PlayerTypeAI,           ///< The player is an AI controlled player on the server.
      PlayerTypeAIDummy,      ///< The player is the ghost of an AI controlled player on the server.
      PlayerTypeClient,       ///< The player is owned by a client.  If this is a ghost, it is owned by a user other than this client.
      PlayerTypeMyClient,     ///< The player controlled by this client.
   };
   
   PlayerType myPlayerType;   ///< What type of player is this?
   
   /// Player constructor, assigns a random position in the playing field to the player, and
   /// if it is AI controlled will pick a first destination point.
   Player(PlayerType pt = PlayerTypeClient);

   /// Player destructor removes the player from the game.
   ~Player();

   /// Mask bits used for determining which object states need to be updated
   /// to all the clients.
   enum MaskBits {
      InitialMask = (1 << 0),  ///< This mask bit is never set explicitly, so it can be used for initialization data.
      PositionMask = (1 << 1), ///< This mask bit is set when the position information changes on the server.
   };

   /// performScopeQuery is called to determine which objects are "in scope" for the client
   /// that controls this Player instance.  In the TNLTest program, all objects in a circle
   /// of radius 0.25 around the scope object are considered to be in scope.
   void performScopeQuery(TNL::GhostConnection *connection);

   /// packUpdate writes the Player's ghost update from the server to the client.
   /// This function takes advantage of the fact that the InitialMask is only ever set
   /// for the first update of the object to send once-only state to the client.
   TNL::U32 packUpdate(TNL::GhostConnection *connection, TNL::U32 updateMask, TNL::BitStream *stream);

   /// unpackUpdate reads the data the server wrote in packUpdate from the packet.
   void unpackUpdate(TNL::GhostConnection *connection, TNL::BitStream *stream);

   /// serverSetPosition is called on the server when it receives notice from a client
   /// to change the position of the player it controls.  serverSetPosition will call
   /// setMaskBits(PositionMask) to notify the network system that this object has
   /// changed state.
   void serverSetPosition(Position startPos, Position endPos, TNL::F32 t, TNL::F32 tDelta);

   /// Move this object along its path.
   ///
   /// If it hits the end point, and it's an AI, it will generate a new destination.
   void update(TNL::F32 timeDelta);   

   /// onGhostAvailable is called on the server when it knows that this Player has been
   /// constructed on the specified client as a result of being "in scope".  In TNLTest
   /// this method call is used to test the per-ghost targeted RPC functionality of
   /// NetObject subclasses by calling rpcPlayerIsInScope on the Player ghost on
   /// the specified connection.
   void onGhostAvailable(TNL::GhostConnection *theConnection);

   /// onGhostAdd is called for every NetObject on the client after the ghost has
   /// been constructed and its initial unpackUpdate is called.  A return value of
   /// false from this function would indicate than an error had occured, notifying the
   /// network system that the connection should be terminated.
   bool onGhostAdd(TNL::GhostConnection *theConnection);

   /// Adds this Player to the list of Player instances in the specified game.
   void addToGame(TestGame *theGame);

   /// rpcPlayerWillMove is used in TNLTest to demonstrate ghost->parent NetObject RPCs.
   TNL_DECLARE_RPC(rpcPlayerWillMove,  (TNL::StringPtr testString));

   /// rpcPlayerDidMove is used in TNLTest to demostrate a broadcast RPC from the server
   /// to all of the ghosts on clients scoping this object.  It also demonstrates the
   /// usage of the Float<> template argument, in this case using 6 bits for each X and
   /// Y position.  Float arguments to RPCs are between 0 and 1.
   TNL_DECLARE_RPC(rpcPlayerDidMove,   (TNL::Float<6> x, TNL::Float<6> y));

   /// rpcPlayerIsInScope is the RPC method called by onGhostAvailable to demonstrate
   /// targeted NetObject RPCs.  onGhostAvailable uses the TNL_RPC_CONSTRUCT_NETEVENT
   /// macro to construct a NetEvent from the RPC invocation and then posts it only
   /// to the client for which the object just came into scope.
   TNL_DECLARE_RPC(rpcPlayerIsInScope, (TNL::Float<6> x, TNL::Float<6> y));

   /// This macro invocation declares the Player class to be known
   /// to the TNL class management system.
   TNL_DECLARE_CLASS(Player);
};

/// The Building class is an example of a NetObject that is ScopeAlways.
/// ScopeAlways objects are transmitted to all clients that are currently
/// ghosting, regardless of whether or not the scope object calls GhostConnection::objectInScope
/// for them or not.  The "buildings" are represented by red rectangles on the
/// playing field, and are constructed with random position and extents.
class Building : public TNL::NetObject
{
   typedef TNL::NetObject Parent;
public:
   /// Mask bits used to determine what states are out of date for this
   /// object and what then represent.
   enum MaskBits {
      InitialMask = (1 << 0), ///< Building's only mask bit is the initial mask, as no other states are ever set.
   };
   TestGame *game;      ///< The game object this building is associated with
   Position upperLeft;  ///< Upper left corner of the building rectangle on the screen.
   Position lowerRight; ///< Lower right corner of the building rectangle on the screen.

   /// The Building constructor creates a random position and extent for the building, and marks it as scopeAlways.
   Building();

   /// The Building destructor removes the Building from the game, if it is associated with a game object.
   ~Building();

   /// Called on the client when this Building object has been ghosted to the
   /// client and its first unpackUpdate has been called.  onGhostAdd adds
   /// the building to the client game.
   bool onGhostAdd(TNL::GhostConnection *theConnection);

   /// addToGame is a helper function called by onGhostAdd and on the server
   /// to add the building to the specified game.
   void addToGame(TestGame *game);

   /// packUpdate is called on the server Building object to update any out-of-date network
   /// state information to the client.  Since the Building object only declares an initial
   /// mask bit state and never calls setMaskBits, this method will only be invoked when
   /// the ghost is being created on the client.
   TNL::U32 packUpdate(TNL::GhostConnection *connection, TNL::U32 updateMask, TNL::BitStream *stream);

   /// Reads the update information about the building from the specified packet BitStream.
   void unpackUpdate(TNL::GhostConnection *connection, TNL::BitStream *stream);

   /// This macro declares Building to be a part of the TNL network class management system.
   TNL_DECLARE_CLASS(Building);
};

/// TestConnection is the TNLTest connection class.
///
/// The TestConnection class overrides particular methods for
/// connection housekeeping, and for connected clients manages
/// the transmission of client inputs to the server for player
/// position updates.
///
/// When a client's connection to the server is lost, the 
/// TestConnection notifies the network interface that it should begin
/// searching for a new server to join.
class TestConnection : public TNL::GhostConnection
{
   typedef TNL::GhostConnection Parent;
public:

   /// The TestConnection constructor.  This method contains a line that can be
   /// uncommented to put the TestConnection into adaptive communications mode.
   TestConnection();

   /// The player object associated with this connection.
   TNL::SafePtr<Player> myPlayer;

   /// onConnectTerminated is called when the connection request to the server
   /// is unable to complete due to server rejection, timeout or other error.
   /// When a TestConnection connect request to a server is terminated, the client's network interface
   /// is notified so it can begin searching for another server to connect to.
   void onConnectTerminated(TNL::NetConnection::TerminationReason reason, const char *rejectionString);

   /// onConnectionTerminated is called when an established connection is terminated, whether
   /// from the local or remote hosts explicitly disconnecting, timing out or network error.
   /// When a TestConnection to a server is disconnected, the client's network interface
   /// is notified so it can begin searching for another server to connect to.
   void onConnectionTerminated(TNL::NetConnection::TerminationReason reason, const char *string);

   /// onConnectionEstablished is called on both ends of a connection when the connection is established.  
   /// On the server this will create a player for this client, and set it as the client's
   /// scope object.  On both sides this will set the proper ghosting behavior for the connection (ie server to client).
   void onConnectionEstablished(); 

   /// isDataToTransmit is called each time the connection is ready to send a packet.  If
   /// the NetConnection subclass has data to send it should return true.  In the case of a simulation,
   /// this should always return true.
   bool isDataToTransmit();

   /// Remote function that client calls to set the position of the player on the server.
   TNL_DECLARE_RPC(rpcSetPlayerPos, (TNL::F32 x, TNL::F32 y));

   /// Enumeration constant used to specify the size field of Float<> parameters in the rpcGotPlayerPos method.
   enum RPCEnumerationValues {
      PlayerPosReplyBitSize = 6, ///< Size, in bits of the floats the server sends to the client to acknowledge position updates.
   };

   /// Remote function the server sends back to the client when it gets a player position.
   /// We only use 6 bits of precision on each float to illustrate the float compression
   /// that's possible using TNL's RPC.  This RPC also sends bool and StringTableEntry data,
   /// as well as showing the use of the TNL_DECLARE_RPC_ENUM macro.
   TNL_DECLARE_RPC(rpcGotPlayerPos, (bool b1, bool b2, TNL::StringTableEntry string, TNL::Float<PlayerPosReplyBitSize> x, TNL::Float<PlayerPosReplyBitSize> y));

   /// TNL_DECLARE_NETCONNECTION is used to declare that TestConnection is a valid connection class to the
   /// TNL network system.
   TNL_DECLARE_NETCONNECTION(TestConnection);
};

/// TestNetInterface - the NetInterface subclass used in TNLTest.
/// TestNetInterface subclasses TNLTest to provide server pinging and response
/// info packets.
/// When a client TestNetInterface starts, it begins sending ping
/// packets to the pingAddr address specified in the constructor.  When a server
/// receives a GamePingRequest packet, it sends a GamePingResponse packet to the
/// source address of the GamePingRequest.  Upon receipt of this response packet,
/// the client attempts to connect to the server that returned the response.
/// When the connection or connection attempt to that server is terminated, the
/// TestNetInterface resumes pinging for available TNLTest servers. 
class TestNetInterface : public TNL::NetInterface
{
   typedef TNL::NetInterface Parent;
public:
   /// Constants used in this NetInterface
   enum Constants {
      PingDelayTime     = 2000,                    ///< Milliseconds to wait between sending GamePingRequest packets.
      GamePingRequest   = FirstValidInfoPacketId,  ///< Byte value of the first byte of a GamePingRequest packet.
      GamePingResponse,                            ///< Byte value of the first byte of a GamePingResponse packet.
   };

   bool pingingServers; ///< True if this is a client that is pinging for active servers.
   TNL::U32 lastPingTime;    ///< The last platform time that a GamePingRequest was sent from this network interface.
   bool isServer;       ///< True if this network interface is a server, false if it is a client.

   TNL::SafePtr<TestConnection> connectionToServer; ///< A safe pointer to the current connection to the server, if this is a client.
   TNL::Address pingAddress; ///< Network address to ping in searching for a server.  This can be a specific host address or it can be a LAN broadcast address.
   TestGame *game;   ///< The game object associated with this network interface.

   /// Constructor for this network interface, called from the constructor for TestGame.
   /// The constructor initializes and binds the network interface, as well as sets
   /// parameters for the associated game and whether or not it is a server.
   TestNetInterface(TestGame *theGame, bool server, const TNL::Address &bindAddress, const TNL::Address &pingAddr);
   
   /// handleInfoPacket overrides the method in the NetInterface class to handle processing
   /// of the GamePingRequest and GamePingResponse packet types.
   void handleInfoPacket(const TNL::Address &address, TNL::U8 packetType, TNL::BitStream *stream);

   /// sendPing sends a GamePingRequest packet to pingAddress of this TestNetInterface.
   void sendPing();

   /// Tick checks to see if it is an appropriate time to send a ping packet, in addition
   /// to checking for incoming packets and processing connections.
   void tick();
};

/// The TestGame class manages a TNLTest client or server instance.
/// TestGame maintains a list of all the Player and Building objects in the
/// playing area, and interfaces with the specific platform's windowing
/// system to respond to user input and render the current game display.
class TestGame {
public:
   TNL::Vector<Player *> players;     ///< vector of player objects in the game
   TNL::Vector<Building *> buildings; ///< vector of buildings in the game
   bool isServer;                ///< was this game created to be a server?
   TestNetInterface *myNetInterface; ///< network interface for this game
   TNL::U32 lastTime; ///< last time that tick() was called
   TNL::SafePtr<Player> serverPlayer; ///< the player that the server controls
   TNL::SafePtr<Player> clientPlayer; ///< the player that this client controls, if this game is a client

   /// Constructor for TestGame, determines whether this game will be a client
   /// or a server, and what addresses to bind to and ping.  If this game is a
   /// server, it will construct 50 random buildings and 15 random AI players to
   /// populate the "world" with.  TestGame also constructs an AsymmetricKey to
   /// demonstrate establishing secure connections with clients and servers.
   TestGame(bool server, const TNL::Address &bindAddress, const TNL::Address &pingAddress);

   /// Destroys a game, freeing all Player and Building objects associated with it.
   ~TestGame();

   /// createLocalConnection demonstrates the use of so-called "short circuit" connections
   /// for connecting to a server NetInterface in the same process as this client game.
   void createLocalConnection(TestGame *serverGame);

   /// Called periodically by the platform windowing code, tick will update
   /// all the players in the simulation as well as tick() the game's
   /// network interface.
   void tick();

   /// renderFrame is called by the platform windowing code to notify the game
   /// that it should render the current world using the specified window area.
   void renderFrame(int width, int height);

   /// moveMyPlayerTo is called by the platform windowing code in response to
   /// user input.
   void moveMyPlayerTo(Position newPosition);
};

/// The instance of the client game, if there is a client currently running.
extern TestGame *clientGame;

/// The instance of the server game, if there is a server currently running.
extern TestGame *serverGame;

};

using namespace TNLTest;
