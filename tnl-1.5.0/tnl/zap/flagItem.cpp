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

#include "flagItem.h"
#include "gameType.h"
#include "ship.h"
#include "glutInclude.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(FlagItem);

FlagItem::FlagItem(Point pos) : Item(pos, false, 20)
{
   mTeam = 0;
   mNetFlags.set(Ghostable);
   mObjectTypeMask |= FlagType | CommandMapVisType;
}

void FlagItem::onAddedToGame(Game *theGame)
{
   theGame->getGameType()->addFlag(this);
}


void FlagItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 3)
      return;

   mTeam = atoi(argv[0]);
   Parent::processArguments(argc-1, argv+1);
   initialPos = mMoveState[ActualState].pos;
}

U32 FlagItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
      stream->writeInt(mTeam + 1, 4);
   return Parent::packUpdate(connection, updateMask, stream);
}

void FlagItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())
      mTeam = stream->readInt(4) - 1;
   Parent::unpackUpdate(connection, stream);
}

bool FlagItem::isAtHome()
{
   return mMoveState[ActualState].pos == initialPos;
}

void FlagItem::sendHome()
{
   mMoveState[ActualState].pos = mMoveState[RenderState].pos = initialPos;
   mMoveState[ActualState].vel = Point(0,0);
   setMaskBits(PositionMask);
   updateExtent();
}

void FlagItem::renderItem(Point pos)
{
   Point offset;

   if(mIsMounted)
      offset.set(15, -15);

   Color c;
   GameType *gt = getGame()->getGameType();

   c = gt->getTeamColor(getTeam());

   renderFlag(pos + offset, c);
}

bool FlagItem::collide(GameObject *hitObject)
{
   if(mIsMounted)
      return false;
   if(hitObject->getObjectTypeMask() & (BarrierType | ForceFieldType))
      return true;
   if(!(hitObject->getObjectTypeMask() & ShipType))
      return false;

   if(isGhost() || ((Ship *) hitObject)->hasExploded)
      return false;

   GameType *gt = getGame()->getGameType();
   if(gt)
      gt->shipTouchFlag((Ship *) hitObject, this);
   return false;
}

void FlagItem::onMountDestroyed()
{
   GameType *gt = getGame()->getGameType();
   if(!gt)
      return;

   if(!mMount.isValid())
      return;

   gt->flagDropped(mMount, this);
   dismount();
}

};