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
#include "gameObjectRender.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(GoalZone);

GoalZone::GoalZone()
{
   mTeam = -1;
   mNetFlags.set(Ghostable);
   mObjectTypeMask = CommandMapVisType;
   mFlashCount = 0;
}

void GoalZone::render()
{
   renderGoalZone(mPolyBounds, getGame()->getGameType()->getTeamColor(getTeam()), isFlashing());
}

S32 GoalZone::getRenderSortValue()
{
   return -1;
}

void GoalZone::processArguments(S32 argc, const char **argv)
{
   if(argc < 7)
      return;

   mTeam = atoi(argv[0]);
   for(S32 i = 2; i < argc; i += 2)
   {
      Point p;
      p.x = atof(argv[i-1]) * getGame()->getGridSize();
      p.y = atof(argv[i]) * getGame()->getGridSize();
      mPolyBounds.push_back(p);
   }
   computeExtent();
}

void GoalZone::setTeam(S32 team)
{
   mTeam = team;
   setMaskBits(TeamMask);
}

void GoalZone::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways(); 
   theGame->getGameType()->addZone(this);
}

void GoalZone::computeExtent()
{
   Rect extent(mPolyBounds[0], mPolyBounds[0]);
   for(S32 i = 1; i < mPolyBounds.size(); i++)
      extent.unionPoint(mPolyBounds[i]);
   setExtent(extent);
}

bool GoalZone::getCollisionPoly(Vector<Point> &polyPoints)
{
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      polyPoints.push_back(mPolyBounds[i]);
   return true;
}

bool GoalZone::collide(GameObject *hitObject)
{
   if(!isGhost() && (hitObject->getObjectTypeMask() & ShipType))
      getGame()->getGameType()->shipTouchZone((Ship *) hitObject, this);

   return false;
}

U32 GoalZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
   {
      stream->writeEnum(mPolyBounds.size(), MaxPoints);
      for(S32 i = 0; i < mPolyBounds.size(); i++)
      {
         stream->write(mPolyBounds[i].x);
         stream->write(mPolyBounds[i].y);
      }
   }
   if(stream->writeFlag(updateMask & TeamMask))
      stream->write(mTeam);
   return 0;
}

void GoalZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())
   {
      S32 size = stream->readEnum(MaxPoints);
      for(S32 i = 0; i < size; i++)
      {
         Point p;
         stream->read(&p.x);
         stream->read(&p.y);
         mPolyBounds.push_back(p);
      }
      if(size)
         computeExtent();
   }
   if(stream->readFlag())
   {
      stream->read(&mTeam);
      if(!isInitialUpdate())
      {
         mFlashTimer.reset(FlashDelay);
         mFlashCount = FlashCount;
      }
   }
}

void GoalZone::idle(GameObject::IdleCallPath path)
{
   if(path != GameObject::ClientIdleMainRemote || mFlashCount == 0)
      return;

   if(mFlashTimer.update(mCurrentMove.time))
   {
      mFlashTimer.reset(FlashDelay);
      mFlashCount--;
   }
}


};