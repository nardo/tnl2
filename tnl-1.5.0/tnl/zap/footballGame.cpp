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

#include "goalZone.h"
#include "gameType.h"
#include "ship.h"
#include "flagItem.h"
#include "gameObjectRender.h"

namespace Zap
{
class Ship;

class ZoneControlGameType : public GameType
{
   typedef GameType Parent;

   Vector<GoalZone*> mZones;
   SafePtr<FlagItem> mFlag;
   S32 mFlagTeam;
public:
   ZoneControlGameType()
   {
      mFlagTeam = -1;
   }

   void onGhostAvailable(GhostConnection *theConnection)
   {
      Parent::onGhostAvailable(theConnection);
      NetObject::setRPCDestConnection(theConnection);
      s2cSetFlagTeam(mFlagTeam);
      NetObject::setRPCDestConnection(NULL);
   }

   void shipTouchFlag(Ship *theShip, FlagItem *theFlag);
   void flagDropped(Ship *theShip, FlagItem *theFlag);
   void addZone(GoalZone *z);
   void addFlag(FlagItem *theFlag)
   {
      mFlag = theFlag;
      addItemOfInterest(theFlag);
   }
   void shipTouchZone(Ship *s, GoalZone *z);
   const char *getGameTypeString() { return "Zone Control"; }
   const char *getInstructionString() { return "Carry the flag into each of the capture zones!"; }
   void renderInterfaceOverlay(bool scoreboardVisible);
   void performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection);

   TNL_DECLARE_RPC(s2cSetFlagTeam, (S32 newFlagTeam));
   TNL_DECLARE_CLASS(ZoneControlGameType);
};

TNL_IMPLEMENT_NETOBJECT(ZoneControlGameType);

GAMETYPE_RPC_S2C(ZoneControlGameType, s2cSetFlagTeam, (S32 flagTeam), (flagTeam))
{
   mFlagTeam = flagTeam;
}

void ZoneControlGameType::shipTouchFlag(Ship *theShip, FlagItem *theFlag)
{
   GameConnection *controlConnection = theShip->getControllingClient();
   ClientRef *cl = controlConnection->getClientRef();
   if(!cl)
      return;

   static StringTableEntry takeString("%e0 of team %e1 has the flag!");
   Vector<StringTableEntry> e;
   e.push_back(cl->name);
   e.push_back(mTeams[cl->teamId].name);
   for(S32 i = 0; i < mClientList.size(); i++)
      mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);
   theFlag->mountToShip(theShip);
   mFlagTeam = theShip->getTeam();
   s2cSetFlagTeam(mFlagTeam);
}

void ZoneControlGameType::flagDropped(Ship *theShip, FlagItem *theFlag)
{
   s2cSetFlagTeam(-1);
   static StringTableEntry dropString("%e0 dropped the flag!");
   Vector<StringTableEntry> e;
   e.push_back(theShip->mPlayerName);
   for(S32 i = 0; i < mClientList.size(); i++)
      mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
}

void ZoneControlGameType::addZone(GoalZone *z)
{
   mZones.push_back(z);
}

void ZoneControlGameType::shipTouchZone(Ship *s, GoalZone *z)
{
   if(z->getTeam() == s->getTeam())
      return;

   S32 i;
   for(i = 0; i < s->mMountedItems.size(); i++)
      if(s->mMountedItems[i].isValid() && (s->mMountedItems[i]->getObjectTypeMask() & FlagType))
         break;
   if(i == s->mMountedItems.size())
      return;

   S32 oldTeam = z->getTeam();
   if(oldTeam != -1)
   {
      static StringTableEntry takeString("%e0 captured a zone from team %e1!");
      Vector<StringTableEntry> e;
      e.push_back(s->mPlayerName);
      e.push_back(mTeams[oldTeam].name);

      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);
      setTeamScore(oldTeam, mTeams[oldTeam].score - 1);
   }
   else
   {
      static StringTableEntry takeString("%e0 captured an unclaimed zone!");
      Vector<StringTableEntry> e;
      e.push_back(s->mPlayerName);
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);
   }
   setTeamScore(s->getTeam(), mTeams[s->getTeam()].score + 1);
   z->setTeam(s->getTeam());
   for(S32 i = 0; i < mZones.size(); i++)
   {
      if(mZones[i]->getTeam() != s->getTeam())
         return;
   }
   
   static StringTableEntry tdString("Team %e0 scored a touchdown!");
   Vector<StringTableEntry> e;
   e.push_back(mTeams[s->getTeam()].name);
   for(S32 i = 0; i < mClientList.size(); i++)
      mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagCapture, tdString, e);

   for(S32 i = 0; i < mZones.size(); i++)
      mZones[i]->setTeam(-1);
   for(S32 i = 0; i < s->mMountedItems.size(); i++)
   {
      Item *theItem = s->mMountedItems[i];
      FlagItem *mountedFlag = dynamic_cast<FlagItem *>(theItem);
      if(mountedFlag)
      {
         mountedFlag->dismount();
         mountedFlag->sendHome();
      }
   }
   mFlagTeam = -1;
   s2cSetFlagTeam(-1);
}

void ZoneControlGameType::performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection)
{
   Parent::performProxyScopeQuery(scopeObject, connection);
   S32 uTeam = scopeObject->getTeam();
   if(mFlag.isValid())
   {
      if(mFlag->isAtHome())
         connection->objectInScope(mFlag);
      else
      {
         Ship *mount = mFlag->getMount();
         if(mount && mount->getTeam() == uTeam)
         {
            connection->objectInScope(mount);
            connection->objectInScope(mFlag);
         }
      }
   }
}

void ZoneControlGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   Parent::renderInterfaceOverlay(scoreboardVisible);
   Ship *u = (Ship *) gClientGame->getConnectionToServer()->getControlObject();
   if(!u)
      return;

   bool hasFlag = mFlag.isValid() && mFlag->getMount() == u;

   // first render all zones that are not yet taken by the flag holding team
   if(mFlagTeam != -1)
   {
      for(S32 i = 0; i < mZones.size(); i++)
      {
         if(mZones[i]->getTeam() != mFlagTeam)
            renderObjectiveArrow(mZones[i], getTeamColor(mZones[i]), hasFlag ? 1.0 : 0.4);
         else if(mZones[i]->didRecentlyChangeTeam() && mZones[i]->getTeam() != -1 && u->getTeam() != mFlagTeam)
         {
            // render a blinky arrow for a recently taken zone
            Color c = getTeamColor(mZones[i]);
            if(mZones[i]->isFlashing())
               c *= 0.7;
            renderObjectiveArrow(mZones[i], c);
         }
      }

   }
   if(!hasFlag)
   {
      if(mFlag.isValid())
      {
         if(!mFlag->isMounted())
            renderObjectiveArrow(mFlag, getTeamColor(mFlag));
         else
         {
            Ship *mount = mFlag->getMount();
            if(mount)
               renderObjectiveArrow(mount, getTeamColor(mount));
         }
      }
   }
}


};