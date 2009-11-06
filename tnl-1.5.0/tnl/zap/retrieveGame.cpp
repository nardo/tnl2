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

class RetrieveGameType : public GameType
{
   typedef GameType Parent;
   Vector<GoalZone *> mZones;
   Vector<SafePtr<FlagItem> > mFlags;
   enum {
      CapScore = 2,
   };
public:
   void addFlag(FlagItem *theFlag)
   {
      S32 i;
      for(i = 0; i < mFlags.size(); i++)
      {
         if(mFlags[i] == NULL)
         {
            mFlags[i] = theFlag;
            break;
         }
      }
      if(i == mFlags.size())
         mFlags.push_back(theFlag);

      if(!isGhost())
         addItemOfInterest(theFlag);
   }

   void addZone(GoalZone *zone)
   {
      mZones.push_back(zone);
   }

   void shipTouchFlag(Ship *theShip, FlagItem *theFlag)
   {
      // see if the ship is already carrying a flag - can only carry one at a time
      for(S32 i = 0; i < theShip->mMountedItems.size(); i++)
         if(theShip->mMountedItems[i].isValid() && (theShip->mMountedItems[i]->getObjectTypeMask() & FlagType))
            return;

      S32 flagIndex;

      for(flagIndex = 0; flagIndex < mFlags.size(); flagIndex++)
         if(mFlags[flagIndex] == theFlag)
            break;

      GameConnection *controlConnection = theShip->getControllingClient();
      ClientRef *cl = controlConnection->getClientRef();
      if(!cl)
         return;

      // see if this flag is already in a flag zone owned by the ship's team
      if(theFlag->getZone() != NULL && theFlag->getZone()->getTeam() == theShip->getTeam())
         return;

      static StringTableEntry stealString("%e0 stole a flag from team %e1!");
      static StringTableEntry takeString("%e0 of team %e1 took a flag!");
      static StringTableEntry oneFlagTakeString("%e0 of team %e1 took the flag!");
      StringTableEntry r = takeString;
      if(mFlags.size() == 1)
         r = oneFlagTakeString;
      S32 teamIndex;

      if(theFlag->getZone() == NULL)
         teamIndex = cl->teamId;
      else
      {
         r = stealString;
         teamIndex = theFlag->getZone()->getTeam();
         setTeamScore(teamIndex, mTeams[teamIndex].score - 1);
      }
      Vector<StringTableEntry> e;
      e.push_back(cl->name);
      e.push_back(mTeams[teamIndex].name);
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(
            GameConnection::ColorNuclearGreen, SFXFlagSnatch, r, e);
      theFlag->mountToShip(theShip);
      theFlag->setZone(NULL);
   }

   void flagDropped(Ship *theShip, FlagItem *theFlag)
   {
      static StringTableEntry dropString("%e0 dropped a flag!");
      Vector<StringTableEntry> e;
      e.push_back(theShip->mPlayerName);
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
   }

   void shipTouchZone(Ship *s, GoalZone *z)
   {
      GameConnection *controlConnection = s->getControllingClient();
      ClientRef *cl = controlConnection->getClientRef();

      if(!cl)
         return;

      // see if this is an opposing team's zone
      if(s->getTeam() != z->getTeam())
         return;

      // see if this zone already has a flag in it...
      for(S32 i = 0; i < mFlags.size(); i++)
         if(mFlags[i]->getZone() == z)
            return;

      // ok, it's an empty zone on our team:
      // see if this ship is carrying a flag
      S32 i;
      for(i = 0; i < s->mMountedItems.size(); i++)
         if(s->mMountedItems[i].isValid() && (s->mMountedItems[i]->getObjectTypeMask() & FlagType))
            break;
      if(i == s->mMountedItems.size())
         return;

      // ok, the ship has a flag and it's on the ship...
      Item *theItem = s->mMountedItems[i];
      FlagItem *mountedFlag = dynamic_cast<FlagItem *>(theItem);
      if(mountedFlag)
      {
         setTeamScore(cl->teamId, mTeams[cl->teamId].score + 1);

         static StringTableEntry capString("%e0 retrieved a flag!");
         static StringTableEntry oneFlagCapString("%e0 retrieved the flag!");

         Vector<StringTableEntry> e;
         e.push_back(cl->name);
         for(S32 i = 0; i < mClientList.size(); i++)
            mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, 
            SFXFlagCapture, (mFlags.size() == 1) ? oneFlagCapString : capString, e);

         // score the flag for the client's team...
         mountedFlag->dismount();

         S32 flagIndex;
         for(flagIndex = 0; flagIndex < mFlags.size(); flagIndex++)
            if(mFlags[flagIndex] == mountedFlag)
               break;

         mFlags[flagIndex]->setZone(z);

         mountedFlag->setActualPos(z->getExtent().getCenter());
         cl->score += CapScore;
         // see if all the flags are owned by one team
         for(S32 i = 0; i < mFlags.size(); i++)
         {
            if(!mFlags[i]->getZone() || mFlags[i]->getZone()->getTeam() != cl->teamId)
               return;
         }

         if(mFlags.size() != 1)
         {
            static StringTableEntry capAllString("Team %e0 retrieved all the flags!");
            e[0] = mTeams[cl->teamId].name;
            for(S32 i = 0; i < mClientList.size(); i++)
               mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagCapture, capAllString, e);
         }
         for(S32 i = 0; i < mFlags.size(); i++)
         {
            mFlags[i]->setZone(NULL);
            mFlags[i]->sendHome();
         }
      }
   }

   void performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection)
   {
      Parent::performProxyScopeQuery(scopeObject, connection);
      S32 uTeam = scopeObject->getTeam();

      for(S32 i = 0; i < mFlags.size(); i++)
      {
         if(mFlags[i]->isAtHome() || mFlags[i]->getZone())
            connection->objectInScope(mFlags[i]);
         else
         {
            Ship *mount = mFlags[i]->getMount();
            if(mount && mount->getTeam() == uTeam)
            {
               connection->objectInScope(mount);
               connection->objectInScope(mFlags[i]);
            }
         }
      }
   }

   void renderInterfaceOverlay(bool scoreboardVisible)
   {
      Parent::renderInterfaceOverlay(scoreboardVisible);
      Ship *u = (Ship *) gClientGame->getConnectionToServer()->getControlObject();
      if(!u)
         return;
      bool uFlag = false;

      for(S32 i = 0; i < mFlags.size(); i++)
      {
         if(mFlags[i].isValid() && mFlags[i]->getMount() == u)
         {
            for(S32 j = 0; j < mZones.size(); j++)
            {
               // see if this is one of our zones and that it doesn't have a flag in it.
               if(mZones[j]->getTeam() != u->getTeam())
                  continue;
               S32 k;
               for(k = 0; k < mFlags.size(); k++)
               {
                  if(!mFlags[k].isValid())
                     continue;
                  if(mFlags[k]->getZone() == mZones[j])
                     break;
               }
               if(k == mFlags.size())
                  renderObjectiveArrow(mZones[j], getTeamColor(u->getTeam()));
            }
            uFlag = true;
            break;
         }
      }

      for(S32 i = 0; i < mFlags.size(); i++)
      {
         if(!mFlags[i].isValid())
            continue;

         if(!mFlags[i]->isMounted() && !uFlag)
         {
            GoalZone *gz = mFlags[i]->getZone();
            if(gz && gz->getTeam() != u->getTeam())
               renderObjectiveArrow(mFlags[i], getTeamColor(gz->getTeam()));
            else if(!gz)
               renderObjectiveArrow(mFlags[i], getTeamColor(-1));
         }
         else
         {
            Ship *mount = mFlags[i]->getMount();
            if(mount && mount != u)
               renderObjectiveArrow(mount, getTeamColor(mount->getTeam()));
         }
      }
   }

   const char *getGameTypeString() { return "Retrieve"; }
   const char *getInstructionString() { return "Bring all the flags to your capture zones!"; }
   TNL_DECLARE_CLASS(RetrieveGameType);
};

TNL_IMPLEMENT_NETOBJECT(RetrieveGameType);


};