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

#ifndef _ENGINEEREDOBJECTS_H_
#define _ENGINEEREDOBJECTS_H_

#include "gameObject.h"
#include "item.h"
#include "barrier.h"

namespace Zap
{

extern void engClientCreateObject(GameConnection *connection, U32 object);

class EngineeredObject : public GameObject
{
   typedef GameObject Parent;
protected:
   F32 mHealth;
   Color mTeamColor;
   SafePtr<Item> mResource;
   SafePtr<Ship> mOwner;
   Point mAnchorPoint;
   Point mAnchorNormal;
   bool mIsDestroyed;
   S32 mOriginalTeam;

   enum MaskBits
   {
      InitialMask = BIT(0),
      HealthMask = BIT(1),
      TeamMask = BIT(2),
      NextFreeMask = BIT(3),
   };
   
public:
   EngineeredObject(S32 team = -1, Point anchorPoint = Point(), Point anchorNormal = Point());
   void setResource(Item *resource);
   bool checkDeploymentPosition();
   void computeExtent();
   virtual void onDestroyed() {}
   virtual void onDisabled() {}
   virtual void onEnabled() {}
   bool isEnabled();

   void processArguments(S32 argc, const char **argv);

   void explode();
   bool isDestroyed() { return mIsDestroyed; }
   void setOwner(Ship *owner);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void damageObject(DamageInfo *damageInfo);
   bool collide(GameObject *hitObject) { return true; }
   F32 getHealth() { return mHealth; }
};

class ForceField : public GameObject
{
   Point mStart, mEnd;
   Timer mDownTimer;
   bool mFieldUp;

public:
   enum Constants
   {
      InitialMask = BIT(0),
      StatusMask = BIT(1),

      FieldDownTime = 250,
   };

   ForceField(S32 team = -1, Point start = Point(), Point end = Point());
   bool collide(GameObject *hitObject);
   void idle(GameObject::IdleCallPath path);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   bool getCollisionPoly(Vector<Point> &polyPoints);
   void render();
   S32 getRenderSortValue() { return -1; }

   TNL_DECLARE_CLASS(ForceField);
};

class ForceFieldProjector : public EngineeredObject
{
   typedef EngineeredObject Parent;

   SafePtr<ForceField> mField;
public:
   ForceFieldProjector(S32 team = -1, Point anchorPoint = Point(), Point anchorNormal = Point()) :
      EngineeredObject(team, anchorPoint, anchorNormal) { mNetFlags.set(Ghostable); }

   bool getCollisionPoly(Vector<Point> &polyPoints);
   void render();
   void onEnabled();
   void onDisabled();
   TNL_DECLARE_CLASS(ForceFieldProjector);
};

class Turret : public EngineeredObject
{
   typedef EngineeredObject Parent;
   Timer mFireTimer;
   F32 mCurrentAngle;

public:
   enum {
      TurretAimOffset = 15,
      TurretRange = 600,
      TurretPerceptionDistance = 800,
      TurretProjectileVelocity = 600,
      TurretTurnRate = 4,
      TurretFireDelay = 150,

      AimMask = EngineeredObject::NextFreeMask,
   };

public:
   Turret(S32 team = -1, Point anchorPoint = Point(), Point anchorNormal = Point(1, 0));

   bool getCollisionPoly(Vector<Point> &polyPoints);
   void render();
   void idle(IdleCallPath path);
   void onAddedToGame(Game *theGame);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(Turret);
};

};

#endif
