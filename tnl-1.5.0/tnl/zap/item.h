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

#ifndef _ITEM_H_
#define _ITEM_H_

#include "moveObject.h"
#include "timer.h"

namespace Zap
{

class Ship;
class GoalZone;

class Item : public MoveObject
{
protected:
   enum MaskBits {
      InitialMask = BIT(0),
      PositionMask = BIT(1),
      WarpPositionMask = BIT(2),
      MountMask = BIT(3),
      ZoneMask = BIT(4),
      FirstFreeMask = BIT(5),
   };

   SafePtr<Ship> mMount;
   SafePtr<GoalZone> mZone;

   bool mIsMounted;
   bool mIsCollideable;
public:
   void idle(GameObject::IdleCallPath path);

   void processArguments(S32 argc, const char **argv);
   
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   Item(Point p = Point(0,0), bool collideable = false, float radius = 1, float mass = 1);

   void setActualPos(Point p);
   void setActualVel(Point vel);

   void mountToShip(Ship *theShip);
   bool isMounted() { return mIsMounted; }
   void setZone(GoalZone *theZone);
   GoalZone *getZone();

   Ship *getMount();
   void dismount();
   void render();
   virtual void renderItem(Point pos) = 0;

   virtual void onMountDestroyed();

   bool collide(GameObject *otherObject);
};

class PickupItem : public Item
{
   typedef Item Parent;
   bool mIsVisible;
   Timer mRepopTimer;
protected:
   enum MaskBits {
      PickupMask = Parent::FirstFreeMask << 0,
      FirstFreeMask = Parent::FirstFreeMask << 1,
   };
public:
   PickupItem(Point p = Point(), float radius = 1);
   void idle(GameObject::IdleCallPath path);
   bool isVisible() { return mIsVisible; }

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   bool collide(GameObject *otherObject);
   virtual bool pickup(Ship *theShip) = 0;
   virtual U32 getRepopDelay() = 0;
   virtual void onClientPickup() = 0;
};

};

#endif

