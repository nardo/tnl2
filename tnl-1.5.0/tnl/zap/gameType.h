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

#ifndef _GAMETYPE_H_
#define _GAMETYPE_H_

#include "gameObject.h"
#include "timer.h"
#include "sfx.h"
#include "voiceCodec.h"

namespace Zap
{

class FlagItem;
class GoalZone;
struct MenuItem;
class Item;

class ClientRef : public Object
{
public:
   StringTableEntry name;  /// Name of client - guaranteed to be unique of current clients
   S32 teamId;
   S32 score;
   Timer respawnTimer;

   bool wantsScoreboardUpdates;
   bool readyForRegularGhosts;

   SafePtr<GameConnection> clientConnection;
   RefPtr<SFXObject> voiceSFX;
   RefPtr<VoiceDecoder> decoder;

   U32 ping;
   ClientRef()
   {
      ping = 0;
      score = 0;
      readyForRegularGhosts = false;
      wantsScoreboardUpdates = false;
      teamId = 0;
   }
};

class GameType : public GameObject
{
public:
   virtual const char *getGameTypeString() { return "Zapmatch"; }
   virtual const char *getInstructionString() { return "Zap as many ships as you can!"; }
   enum
   {
      RespawnDelay = 1500,
   };

   struct BarrierRec
   {
      Vector<F32> verts;
      F32 width;
   };

   Vector<BarrierRec> mBarriers;
   Vector<RefPtr<ClientRef> > mClientList;

   virtual ClientRef *allocClientRef() { return new ClientRef; }

   struct Team
   {
      StringTableEntry name;
      Color color;
      Vector<Point> spawnPoints;
      U32 numPlayers;
      S32 score;
      Team() { numPlayers = 0; score = 0; }
   };
   Vector<Team> mTeams;

   StringTableEntry mLevelName;
   StringTableEntry mLevelDescription;

   struct ItemOfInterest
   {
      SafePtr<Item> theItem;
      U32 teamVisMask;
   };

   Vector<ItemOfInterest> mItemsOfInterest;

   void addItemOfInterest(Item *theItem);

   Timer mScoreboardUpdateTimer;
   Timer mGameTimer;
   Timer mGameTimeUpdateTimer;
   Timer mLevelInfoDisplayTimer;
   S32 mTeamScoreLimit;
   bool mGameOver; // set to true when an end condition is met

   enum {
      MaxPing = 999,
      DefaultGameTime = 20 * 60 * 1000,
      DefaultTeamScoreLimit = 8,
      LevelInfoDisplayTime = 6000,
   };

   static Vector<RangedU32<0, MaxPing> > mPingTimes; ///< Static vector used for constructing update RPCs
   static Vector<SignedInt<24> > mScores;

   GameType();
   void countTeamPlayers();

   Color getClientColor(const StringTableEntry &clientName)
   {
      ClientRef *cl = findClientRef(clientName);
      if(cl)
         return mTeams[cl->teamId].color;
      return Color();
   }

   ClientRef *findClientRef(const StringTableEntry &name);

   void processArguments(S32 argc, const char **argv);
   virtual bool processLevelItem(S32 argc, const char **argv);
   void onAddedToGame(Game *theGame);

   void idle(GameObject::IdleCallPath path);

   void gameOverManGameOver();
   virtual void onGameOver();

   virtual void serverAddClient(GameConnection *theClient);
   virtual void serverRemoveClient(GameConnection *theClient);

   virtual bool objectCanDamageObject(GameObject *damager, GameObject *victim);
   virtual void controlObjectForClientKilled(GameConnection *theClient, GameObject *clientObject, GameObject *killerObject);

   virtual void spawnShip(GameConnection *theClient);
   virtual void changeClientTeam(GameConnection *theClient);

   virtual void renderInterfaceOverlay(bool scoreboardVisible);
   void renderObjectiveArrow(GameObject *target, Color c, F32 alphaMod = 1.0f);
   void renderObjectiveArrow(Point p, Color c, F32 alphaMod = 1.0f);

   void renderTimeLeft();
   void renderTalkingClients();
   virtual void clientRequestLoadout(GameConnection *client, const Vector<U32> &loadout);
   virtual void updateShipLoadout(GameObject *shipObject); // called from LoadoutZone when a Ship touches the zone

   void setClientShipLoadout(ClientRef *cl, const Vector<U32> &loadout);

   virtual Color getShipColor(Ship *s);
   Color getTeamColor(S32 team);
   Color getTeamColor(GameObject *theObject);
   // game type flag methods for CTF, Rabbit, Football
   virtual void addFlag(FlagItem *theFlag) {}
   virtual void flagDropped(Ship *theShip, FlagItem *theFlag) {}
   virtual void shipTouchFlag(Ship *theShip, FlagItem *theFlag) {}

   virtual void addZone(GoalZone *theZone) {}
   virtual void shipTouchZone(Ship *theShip, GoalZone *theZone) {}

   void queryItemsOfInterest();
   void performScopeQuery(GhostConnection *connection);
   virtual void performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection);

   void onGhostAvailable(GhostConnection *theConnection);
   TNL_DECLARE_RPC(s2cSetLevelInfo, (StringTableEntry levelName, StringTableEntry levelDesc));
   TNL_DECLARE_RPC(s2cAddBarriers, (Vector<F32> barrier, F32 width));
   TNL_DECLARE_RPC(s2cAddTeam, (StringTableEntry teamName, F32 r, F32 g, F32 b));
   TNL_DECLARE_RPC(s2cAddClient, (StringTableEntry clientName, bool isMyClient));
   TNL_DECLARE_RPC(s2cClientJoinedTeam, (StringTableEntry clientName, U32 teamIndex));

   TNL_DECLARE_RPC(s2cSyncMessagesComplete, (U32 sequence));
   TNL_DECLARE_RPC(c2sSyncMessagesComplete, (U32 sequence));

   TNL_DECLARE_RPC(s2cSetGameOver, (bool gameOver));
   TNL_DECLARE_RPC(s2cSetTimeRemaining, (U32 timeLeft));

   TNL_DECLARE_RPC(s2cRemoveClient, (StringTableEntry clientName));

   void setTeamScore(S32 teamIndex, S32 newScore);
   TNL_DECLARE_RPC(s2cSetTeamScore, (U32 teamIndex, U32 score));

   TNL_DECLARE_RPC(c2sRequestScoreboardUpdates, (bool updates));
   TNL_DECLARE_RPC(s2cScoreboardUpdate, (Vector<RangedU32<0, MaxPing> > pingTimes, Vector<SignedInt<24> > scores));
   virtual void updateClientScoreboard(ClientRef *theClient);

   TNL_DECLARE_RPC(c2sAdvanceWeapon, ());

   virtual void addClientGameMenuOptions(Vector<MenuItem> &menuOptions);
   virtual void processClientGameMenuOption(U32 index);
   TNL_DECLARE_RPC(c2sChangeTeams, ());

   void sendChatDisplayEvent(ClientRef *cl, bool global, NetEvent *theEvent);
   TNL_DECLARE_RPC(c2sSendChat, (bool global, StringPtr message));
   TNL_DECLARE_RPC(c2sSendChatSTE, (bool global, StringTableEntry ste));
   TNL_DECLARE_RPC(s2cDisplayChatMessage, (bool global, StringTableEntry clientName, StringPtr message));
   TNL_DECLARE_RPC(s2cDisplayChatMessageSTE, (bool global, StringTableEntry clientName, StringTableEntry message));

   TNL_DECLARE_RPC(s2cKillMessage, (StringTableEntry victim, StringTableEntry killer));

   TNL_DECLARE_RPC(c2sVoiceChat, (bool echo, ByteBufferPtr compressedVoice));
   TNL_DECLARE_RPC(s2cVoiceChat, (StringTableEntry client, ByteBufferPtr compressedVoice));

   TNL_DECLARE_CLASS(GameType);
};

#define GAMETYPE_RPC_S2C(className, methodName, args, argNames) \
   TNL_IMPLEMENT_NETOBJECT_RPC(className, methodName, args, argNames, NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)

#define GAMETYPE_RPC_C2S(className, methodName, args, argNames) \
   TNL_IMPLEMENT_NETOBJECT_RPC(className, methodName, args, argNames, NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhostParent, 0)



};

#endif

