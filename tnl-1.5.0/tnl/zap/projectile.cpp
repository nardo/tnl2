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

#include "gameWeapons.h"
#include "projectile.h"
#include "ship.h"
#include "sparkManager.h"
#include "sfx.h"
#include "gameObject.h"
#include "gameObjectRender.h"
#include "glutInclude.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(Projectile);

Projectile::Projectile(U32 type, Point p, Point v, U32 t, GameObject *shooter)
{
   mObjectTypeMask = BIT(4); //ProjectileType

   mNetFlags.set(Ghostable);
   pos = p;
   velocity = v;
   mTimeRemaining = t;
   collided = false;
   alive = true;
   mShooter = shooter;
   if(shooter)
   {
      setOwner(shooter->getOwner());
      mTeam = shooter->getTeam();
   }
   mType = type;
}

U32 Projectile::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
   {
      ((GameConnection *) connection)->writeCompressedPoint(pos, stream);
      writeCompressedVelocity(velocity, CompressedVelocityMax, stream);

      stream->writeEnum(mType, ProjectileTypeCount);

      S32 index = -1;
      if(mShooter.isValid())
         index = connection->getGhostIndex(mShooter);
      if(stream->writeFlag(index != -1))
         stream->writeInt(index, GhostConnection::GhostIdBitSize);
   }
   stream->writeFlag(collided);
   stream->writeFlag(alive);
   return 0;
}

void Projectile::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;

   if(stream->readFlag())
   {
      ((GameConnection *) connection)->readCompressedPoint(pos, stream);
      readCompressedVelocity(velocity, CompressedVelocityMax, stream);

      mType = stream->readEnum(ProjectileTypeCount);

      if(stream->readFlag())
         mShooter = (Ship *) connection->resolveGhost(stream->readInt(GhostConnection::GhostIdBitSize));
      pos += velocity * -0.020f;
      Rect newExtent(pos,pos);
      setExtent(newExtent);
      initial = true;
      SFXObject::play(gProjInfo[mType].projectileSound, pos, velocity);
   }
   bool preCollided = collided;
   collided = stream->readFlag();
   alive = stream->readFlag();

   if(!preCollided && collided)
      explode(NULL, pos);

   if(!collided && initial)
   {
      mCurrentMove.time = U32(connection->getOneWayTime());
     // idle(GameObject::ClientIdleMainRemote);
   }
}

void Projectile::handleCollision(GameObject *hitObject, Point collisionPoint)
{
   collided = true;

   if(!isGhost())
   {
      DamageInfo theInfo;
      theInfo.collisionPoint = collisionPoint;
      theInfo.damageAmount = gProjInfo[mType].damageAmount;
      theInfo.damageType = 0;
      theInfo.damagingObject = this;
      theInfo.impulseVector = velocity;

      hitObject->damageObject(&theInfo);
   }

   mTimeRemaining = 0;
   explode(hitObject, collisionPoint);
}

void Projectile::idle(GameObject::IdleCallPath path)
{
   U32 deltaT = mCurrentMove.time;
   if(!collided && alive)
   {
      Point endPos = pos + velocity * deltaT * 0.001;
      static Vector<GameObject *> disableVector;

      Rect queryRect(pos, endPos);

      float collisionTime;
      disableVector.clear();

      U32 aliveTime = getGame()->getCurrentTime() - getCreationTime();
      if(mShooter.isValid() && aliveTime < 500)
      {
         disableVector.push_back(mShooter);
         mShooter->disableCollision();
      }

      GameObject *hitObject;
      Point surfNormal;
      for(;;)
      {
         hitObject = findObjectLOS(MoveableType | BarrierType | EngineeredType | ForceFieldType, MoveObject::RenderState, pos, endPos, collisionTime, surfNormal);
         if(!hitObject || hitObject->collide(this))
            break;
         disableVector.push_back(hitObject);
         hitObject->disableCollision();
      }

      for(S32 i = 0; i < disableVector.size(); i++)
         disableVector[i]->enableCollision();

      if(hitObject)
      {
         bool bounce = false;
         U32 typeMask = hitObject->getObjectTypeMask();
         
         if(mType == ProjectileBounce && (typeMask & BarrierType))
            bounce = true;
         else if(typeMask & ShipType)
         {
            Ship *s = (Ship *) hitObject;
            if(s->isShieldActive())
               bounce = true;
         }

         if(bounce)
         {
            // We hit something that we should bounce from, so bounce!
            velocity -= surfNormal * surfNormal.dot(velocity) * 2;
            Point collisionPoint = pos + (endPos - pos) * collisionTime;
            pos = collisionPoint + surfNormal;

            SFXObject::play(SFXBounceShield, collisionPoint, surfNormal * surfNormal.dot(velocity) * 2);
         }
         else
         {
            Point collisionPoint = pos + (endPos - pos) * collisionTime;
            handleCollision(hitObject, collisionPoint);
         }
      }
      else
         pos = endPos;

      Rect newExtent(pos,pos);
      setExtent(newExtent);
   }

   if(alive && path == GameObject::ServerIdleMainLoop)
   {
      if(mTimeRemaining <= deltaT)
      {
         deleteObject(500);
         mTimeRemaining = 0;
         alive = false;
         setMaskBits(ExplodedMask);
      }
      else
         mTimeRemaining -= deltaT;
   }
}

void Projectile::explode(GameObject *hitObject, Point thePos)
{
   // Do some particle spew...
   if(isGhost())
   {
      FXManager::emitExplosion(thePos, 0.3, gProjInfo[mType].sparkColors, NumSparkColors);

      Ship *s = dynamic_cast<Ship*>(hitObject);
      if(s && s->isShieldActive())
         SFXObject::play(SFXBounceShield, thePos, velocity);
      else
         SFXObject::play(gProjInfo[mType].impactSound, thePos, velocity);
   }
}

void Projectile::render()
{
   if(collided || !alive)
      return;
   renderProjectile(pos, mType, getGame()->getCurrentTime() - getCreationTime());
}

//-----------------------------------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(Mine);

Mine::Mine(Point pos, Ship *planter)
 : GrenadeProjectile(pos, Point())
{
   mObjectTypeMask |= MineType;

   if(planter)
   {
      setOwner(planter->getOwner());
      mTeam = planter->getTeam();
   }
   else
   {
      mTeam = -1;
   }
   mArmed = false;
}

static Vector<GameObject*> fillVector;

void Mine::idle(IdleCallPath path)
{
   // Skip the grenade timing goofiness...
   Item::idle(path);

   if(exploded || path != GameObject::ServerIdleMainLoop)
      return;

   // And check for enemies in the area...
   Point pos = getActualPos();
   Rect queryRect(pos, pos);
   queryRect.expand(Point(SensorRadius, SensorRadius));

   fillVector.clear();
   findObjects(MotionTriggerTypes | MineType, fillVector, queryRect);

   // Found something!
   bool foundItem = false;
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      F32 radius;
      Point ipos;
      if(fillVector[i]->getCollisionCircle(MoveObject::RenderState, ipos, radius))
      {
         if((ipos - pos).len() < (radius + SensorRadius))
         {
            bool isMine = fillVector[i]->getObjectTypeMask() & MineType;
            if(!isMine)
            {
               foundItem = true;
               break;
            }
            else if(mArmed && fillVector[i] != this)
            {
               foundItem = true;
               break;
            }
         }
      }
   }
   if(foundItem)
   {
      if(mArmed)
         explode(getActualPos());
   }
   else
   {
      if(!mArmed)
      {
         setMaskBits(ArmedMask);
         mArmed = true;
      }
   }
}

bool Mine::collide(GameObject *otherObj)
{
   if(otherObj->getObjectTypeMask() & (ProjectileType))
      explode(getActualPos());
   return false;
}

void Mine::damageObject(DamageInfo *info)
{
   if(info->damageAmount > 0.f && !exploded)
      explode(getActualPos());
}

U32  Mine::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 ret = Parent::packUpdate(connection, updateMask, stream);
   if(stream->writeFlag(updateMask & InitialMask))
      stream->write(mTeam);
   stream->writeFlag(mArmed);
   return ret;
}

void Mine::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
   {
      initial = true;
      stream->read(&mTeam);
   }
   bool wasArmed = mArmed;
   mArmed = stream->readFlag();
   if(initial && !mArmed)
      SFXObject::play(SFXMineDeploy, getActualPos(), Point());
   else if(!initial && !wasArmed && mArmed)
      SFXObject::play(SFXMineArm, getActualPos(), Point());
}

void Mine::renderItem(Point pos)
{
   if(exploded)
      return;

   Ship *co = (Ship *) gClientGame->getConnectionToServer()->getControlObject();

   if(!co)
      return;
   bool visible = co->getTeam() == getTeam() || co->isSensorActive();
   renderMine(pos, mArmed, visible);
}

//-----------------------------------------------------------------------------
TNL_IMPLEMENT_NETOBJECT(GrenadeProjectile);

GrenadeProjectile::GrenadeProjectile(Point pos, Point vel, U32 liveTime, GameObject *shooter)
 : Item(pos, true, 7.f, 1.f)
{
   mObjectTypeMask = MoveableType | ProjectileType;

   mNetFlags.set(Ghostable);

   mMoveState[0].pos = pos;
   mMoveState[0].vel = vel;
   setMaskBits(PositionMask);

   updateExtent();

   ttl = liveTime;
   exploded = false;
   if(shooter)
   {
      setOwner(shooter->getOwner());
      mTeam = shooter->getTeam();
   }
}

void GrenadeProjectile::idle(IdleCallPath path)
{
   Parent::idle(path);

   // Do some drag...
   mMoveState[0].vel -= mMoveState[0].vel * (F32(mCurrentMove.time) / 1000.f);

   if(!exploded)
   {
      if(getActualVel().len() < 4.0)
        explode(getActualPos());
   }

   if(isGhost()) return;

   // Update TTL
   U32 deltaT = mCurrentMove.time;
   if(path == GameObject::ClientIdleMainRemote)
      ttl += deltaT;
   else if(!exploded)
   {
      if(ttl <= deltaT)
        explode(getActualPos());
      else
         ttl -= deltaT;
   }

}

U32  GrenadeProjectile::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 ret = Parent::packUpdate(connection, updateMask, stream);
   stream->writeFlag(exploded);
   stream->writeFlag(updateMask & InitialMask);
   return ret;
}

void GrenadeProjectile::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
   {
      explode(getActualPos());
   }

   if(stream->readFlag())
   {
      SFXObject::play(SFXGrenadeProjectile, getActualPos(), getActualVel());
   }
}

void GrenadeProjectile::damageObject(DamageInfo *theInfo)
{
   // If we're being damaged by another grenade, explode...
   if(theInfo->damageType == 1)
   {
      explode(getActualPos());
      return;
   }

   // Bounce off of stuff.
   Point dv = theInfo->impulseVector - mMoveState[ActualState].vel;
   Point iv = mMoveState[ActualState].pos - theInfo->collisionPoint;
   iv.normalize();
   mMoveState[ActualState].vel += iv * dv.dot(iv) * 0.3;

   setMaskBits(PositionMask);
}

void GrenadeProjectile::explode(Point pos)
{
   if(exploded) return;

   if(isGhost())
   {
      // Make us go boom!
      Color b(1,1,1);

      FXManager::emitExplosion(getRenderPos(), 0.5, gProjInfo[ProjectilePhaser].sparkColors, NumSparkColors);
      SFXObject::play(SFXMineExplode, getActualPos(), Point());
   }

   disableCollision();

   if(!isGhost())
   {
      setMaskBits(PositionMask);
      deleteObject(100);

      DamageInfo info;
      info.collisionPoint = pos;
      info.damagingObject = this;
      info.damageAmount   = 0.5;
      info.damageType     = 1;

      radiusDamage(pos, 100.f, 250.f, DamagableTypes, info);
   }

   exploded = true;

}

void GrenadeProjectile::renderItem(Point pos)
{
   if(exploded)
      return;
   renderGrenade(pos);
}

};
