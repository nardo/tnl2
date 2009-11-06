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

#include "teleporter.h"
#include "glutInclude.h"

using namespace TNL;
#include "ship.h"
#include "sparkManager.h"
#include "gameLoader.h"
#include "sfx.h"
#include "gameObjectRender.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(Teleporter);

Teleporter::Teleporter(Point start, Point end)
{
   mNetFlags.set(Ghostable);
   pos = start;
   dest = end;
   timeout = 0;

   Rect r(pos, pos);
   r.expand(Point(TeleporterRadius, TeleporterRadius));
   setExtent(r);
   mObjectTypeMask |= CommandMapVisType;

   mTime = 0;
}

void Teleporter::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();
}

void Teleporter::processArguments(S32 argc, const char **argv)
{
   if(argc != 4)
      return;

   pos.read(argv);
   pos *= getGame()->getGridSize();

   dest.read(argv + 2);
   dest *= getGame()->getGridSize();

   Rect r(pos, pos);
   r.expand(Point(TeleporterRadius, TeleporterRadius));
   setExtent(r);
}

U32 Teleporter::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   bool isInitial = (updateMask & BIT(3));

   if(stream->writeFlag(updateMask & InitMask))
   {
      stream->write(pos.x);
      stream->write(pos.y);
      stream->write(dest.x);
      stream->write(dest.y);
   }

   stream->writeFlag((updateMask & TeleportMask) && !isInitial);

   return 0;
}

void Teleporter::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())
   {
      stream->read(&pos.x);
      stream->read(&pos.y);
      stream->read(&dest.x);
      stream->read(&dest.y);

      Rect r(pos, pos);
      r.expand(Point(TeleporterRadius, TeleporterRadius));
      setExtent(r);
   }
   if(stream->readFlag() && isGhost())
   {
      FXManager::emitTeleportInEffect(dest, 0);
      SFXObject::play(SFXTeleportIn, dest, Point());
      SFXObject::play(SFXTeleportOut, pos, Point());
      timeout = TeleporterDelay;
   }
}

static Vector<GameObject *> fillVector2;

void Teleporter::idle(GameObject::IdleCallPath path)
{
   U32 deltaT = mCurrentMove.time;
   mTime += deltaT;
   // Deal with our timeout...
   if(timeout > deltaT)
   { 
      timeout -= deltaT;
      return;
   }
   else
      timeout = 0;

   if(path != GameObject::ServerIdleMainLoop)
      return;

   // Check for players within range
   // if so, blast them to dest
   Rect queryRect(pos, pos);
   queryRect.expand(Point(TeleporterRadius, TeleporterRadius));

   fillVector2.clear();
   findObjects(ShipType, fillVector2, queryRect);

   // First see if we're triggered...
   bool isTriggered = false;

   for(S32 i=0; i<fillVector2.size(); i++)
   {
      Ship *s = (Ship*)fillVector2[i];
      if((pos - s->getActualPos()).len() < TeleporterTriggerRadius)
      {
         isTriggered = true;
         setMaskBits(TeleportMask);
         timeout = TeleporterDelay;
      }
   }
   
   if(!isTriggered)
      return;

   for(S32 i=0; i<fillVector2.size(); i++)
   {
      Ship *s = (Ship*)fillVector2[i];
      if((pos - s->getRenderPos()).len() < TeleporterRadius + s->getRadius())
      {
         Point newPos = s->getActualPos() - pos + dest;
         s->setActualPos(newPos);
      }
   }
}

inline Point polarToRect(Point p)
{
   F32 &r  = p.x;
   F32 &th = p.y;

   return Point(
      cos(th) * r,
      sin(th) * r
      );

}

void Teleporter::render()
{
   F32 r;
   if(timeout > TeleporterExpandTime)
      r = (timeout - TeleporterExpandTime) / F32(TeleporterDelay - TeleporterExpandTime);
   else
      r = F32(TeleporterExpandTime - timeout) / F32(TeleporterExpandTime);
   renderTeleporter(pos, 0, true, mTime, r, TeleporterRadius, 1.0);
}

};
