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

#include "CTFGame.h"
#include "ship.h"
#include "UIGame.h"
#include "flagItem.h"
#include "sfx.h"

#include "glutInclude.h"
#include <stdio.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(CTFGameType);

void CTFGameType::addFlag(FlagItem *theFlag)
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

void CTFGameType::shipTouchFlag(Ship *theShip, FlagItem *theFlag)
{
   GameConnection *controlConnection = theShip->getControllingClient();
   ClientRef *cl = controlConnection->getClientRef();

   if(!cl)
      return;

   if(cl->teamId == theFlag->getTeam())
   {
      if(!theFlag->isAtHome())
      {
         static StringTableEntry returnString("%e0 returned the %e1 flag.");
         Vector<StringTableEntry> e;
         e.push_back(cl->name);
         e.push_back(mTeams[theFlag->getTeam()].name);
         for(S32 i = 0; i < mClientList.size(); i++)
            mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagReturn, returnString, e);

         theFlag->sendHome();
         cl->score += ReturnScore;
      }
      else
      {
         // check if this client has an enemy flag mounted
         for(S32 i = 0; i < theShip->mMountedItems.size(); i++)
         {
            Item *theItem = theShip->mMountedItems[i];
            FlagItem *mountedFlag = dynamic_cast<FlagItem *>(theItem);
            if(mountedFlag)
            {
               setTeamScore(cl->teamId, mTeams[cl->teamId].score + 1);

               static StringTableEntry capString("%e0 captured the %e1 flag!");
               Vector<StringTableEntry> e;
               e.push_back(cl->name);
               e.push_back(mTeams[mountedFlag->getTeam()].name);
               for(S32 i = 0; i < mClientList.size(); i++)
                  mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagCapture, capString, e);

               // score the flag for the client's team...
               mountedFlag->dismount();
               mountedFlag->sendHome();
               cl->score += CapScore;
            }
         }
      }
   }
   else
   {
      static StringTableEntry takeString("%e0 took the %e1 flag!");
      Vector<StringTableEntry> e;
      e.push_back(cl->name);
      e.push_back(mTeams[theFlag->getTeam()].name);
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);
      theFlag->mountToShip(theShip);
   }
}

void CTFGameType::flagDropped(Ship *theShip, FlagItem *theFlag)
{
   static StringTableEntry dropString("%e0 dropped the %e1 flag!");
   Vector<StringTableEntry> e;
   e.push_back(theShip->mPlayerName);
   e.push_back(mTeams[theFlag->getTeam()].name);
   for(S32 i = 0; i < mClientList.size(); i++)
      mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
}

void CTFGameType::performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection)
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


void CTFGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   Parent::renderInterfaceOverlay(scoreboardVisible);
   Ship *u = (Ship *) gClientGame->getConnectionToServer()->getControlObject();
   if(!u)
      return;

   for(S32 i = 0; i < mFlags.size(); i++)
   {
      if(!mFlags[i].isValid())
         continue;

      if(mFlags[i]->isMounted())
      {
         Ship *mount = mFlags[i]->getMount();
         if(mount)
            renderObjectiveArrow(mount, getTeamColor(mount->getTeam()));
      }
      else
         renderObjectiveArrow(mFlags[i], getTeamColor(mFlags[i]->getTeam()));
   }
}


};

