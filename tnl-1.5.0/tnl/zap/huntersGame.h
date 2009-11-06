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

#ifndef _HUNTERSGAME_H_
#define _HUNTERSGAME_H_

#include "gameType.h"
#include "item.h"

namespace Zap
{

class Ship;
class HuntersFlagItem;
class HuntersNexusObject;

class HuntersGameType : public GameType
{
   typedef GameType Parent;

   S32 mNexusReturnDelay;
   S32 mNexusCapDelay;
   Timer mNexusReturnTimer;
   Timer mNexusCapTimer;

   struct YardSaleWaypoint
   {
      Timer timeLeft;
      Point pos;
   };
   Vector<YardSaleWaypoint> mYardSaleWaypoints;
   SafePtr<HuntersNexusObject> mNexus;
public:
   bool mCanNexusCap;

   HuntersGameType();
   void processArguments(S32 argc, const char **argv);
   
   void addNexus(HuntersNexusObject *theObject);
   void shipTouchNexus(Ship *theShip, HuntersNexusObject *theNexus);
   void onGhostAvailable(GhostConnection *theConnection);
   void idle(GameObject::IdleCallPath path);
   void renderInterfaceOverlay(bool scoreboardVisible);
   
   void controlObjectForClientKilled(GameConnection *theClient, GameObject *clientObject, GameObject *killerObject);
   void spawnShip(GameConnection *theClient);
   const char *getGameTypeString() { return "Hunters"; }
   const char *getInstructionString() { return "Collect flags from opposing players and bring them to the Nexus!"; }
   
   enum {
      DefaultNexusReturnDelay = 60000,
      DefaultNexusCapDelay = 15000,
      YardSaleWaypointTime = 5000,
      YardSaleCount = 8,
   };
   enum {
      HuntersMsgScore,
      HuntersMsgYardSale,
      HuntersMsgGameOverWin,
      HuntersMsgGameOverTie,
   };

   TNL_DECLARE_RPC(s2cAddYardSaleWaypoint, (F32 x, F32 y));
   TNL_DECLARE_RPC(s2cSetNexusTimer, (U32 nexusTime, bool canCap));
   TNL_DECLARE_RPC(s2cHuntersMessage, (U32 msgIndex, StringTableEntry clientName, U32 flagCount));
   TNL_DECLARE_CLASS(HuntersGameType);
};

class HuntersFlagItem : public Item
{
   typedef Item Parent;

protected:
   enum MaskBits {
      FlagCountMask = Parent::FirstFreeMask << 0,
      FirstFreeMask = Parent::FirstFreeMask << 1,
   };

   U32 mFlagCount;

public:
   HuntersFlagItem(Point pos = Point());

   void renderItem(Point pos);
   void onMountDestroyed();
   void setActualVel(Point v);
   bool collide(GameObject *hitObject);

   void changeFlagCount(U32 change) { mFlagCount = change; setMaskBits(FlagCountMask); }
   U32 getFlagCount() { return mFlagCount; }

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(HuntersFlagItem);
};

class HuntersNexusObject : public GameObject
{
   typedef GameObject Parent;
   Rect nexusBounds;

public:
   HuntersNexusObject();

   void onAddedToGame(Game *theGame);
   void processArguments(S32 argc, const char **argv);

   void render();
   S32 getRenderSortValue() { return -1; }

   bool getCollisionPoly(Vector<Point> &polyPoints);
   bool collide(GameObject *hitObject);

   U32 packUpdate(GhostConnection *connection, U32 mask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(HuntersNexusObject);
};

};

#endif  // _HUNTERSGAME_H_
