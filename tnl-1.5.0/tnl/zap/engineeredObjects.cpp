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

#include "engineeredObjects.h"
#include "ship.h"
#include "glutInclude.h"
#include "projectile.h"
#include "gameType.h"
#include "gameWeapons.h"
#include "sfx.h"
#include "gameObjectRender.h"

namespace Zap
{

static Vector<GameObject *> fillVector;

void engClientCreateObject(GameConnection *connection, U32 object)
{
   Ship *ship = (Ship *) connection->getControlObject();
   if(!ship)
      return;

   if(!ship->carryingResource())
      return;

   Point startPoint = ship->getActualPos();
   Point endPoint = startPoint + ship->getAimVector() * Ship::MaxEngineerDistance;

   F32 collisionTime;
   Point collisionNormal;

   GameObject *hitObject = ship->findObjectLOS(BarrierType, 
      MoveObject::ActualState, startPoint, endPoint, collisionTime, collisionNormal);

   if(!hitObject)
      return;

   Point deployPosition = startPoint + (endPoint - startPoint) * collisionTime;

   // move the deploy point away from the wall by one unit...
   deployPosition += collisionNormal;

   EngineeredObject *deployedObject = NULL;
   switch(object)
   {
      case EngineeredTurret:
         deployedObject = new Turret(ship->getTeam(), deployPosition, collisionNormal);
         break;
      case EngineeredForceField:
         deployedObject = new ForceFieldProjector(ship->getTeam(), deployPosition, collisionNormal);
         break;
   }
   deployedObject->setOwner(ship);

   deployedObject->computeExtent();
   if(!deployedObject || !deployedObject->checkDeploymentPosition())
   {
      static StringTableEntry message("Unable to deploy in that location.");

      connection->s2cDisplayMessage(GameConnection::ColorAqua, SFXNone, message);
      delete deployedObject;
      return;
   }
   if(!ship->engineerBuildObject())
   {
      static StringTableEntry message("Not enough energy to build object.");

      connection->s2cDisplayMessage(GameConnection::ColorAqua, SFXNone, message);
      delete deployedObject;
      return;
   }
   deployedObject->addToGame(gServerGame);
   deployedObject->onEnabled();
   Item *theItem = ship->unmountResource();

   deployedObject->setResource(theItem);
}

EngineeredObject::EngineeredObject(S32 team, Point anchorPoint, Point anchorNormal)
{
   mHealth = 1.f;
   mTeam = team;
   mOriginalTeam = mTeam;
   mAnchorPoint = anchorPoint;
   mAnchorNormal= anchorNormal;
   mObjectTypeMask = EngineeredType | CommandMapVisType;
   mIsDestroyed = false;
}

void EngineeredObject::processArguments(S32 argc, const char **argv)
{
   if(argc != 3)
      return;
   Point p;
   mTeam = atoi(argv[0]);
   mOriginalTeam = mTeam;
   if(mTeam == -1)
      mHealth = 0;
   p.read(argv + 1);
   p *= getGame()->getGridSize();

   // find the mount point:
   F32 minDist = 1000;
   Point normal;
   Point anchor;

   for(F32 theta = 0; theta < Float2Pi; theta += FloatPi * 0.125)
   {
      Point dir(cos(theta), sin(theta));
      dir *= 100;

      F32 t;
      Point n;
      if(findObjectLOS(BarrierType, 0, p, p + dir, t, n))
      {
         if(t < minDist)
         {
            anchor = p + dir * t;
            normal = n;
            minDist = t;
         }
      }
   }
   if(minDist > 1)
      return;
   mAnchorPoint = anchor + normal;
   mAnchorNormal = normal;
   computeExtent();
   if(mHealth != 0)
      onEnabled();
}

void EngineeredObject::setOwner(Ship *owner)
{
   mOwner = owner;
}

void EngineeredObject::setResource(Item *resource)
{
   TNLAssert(resource->isMounted() == false, "Doh!");
   mResource = resource;
   mResource->removeFromDatabase();
}

static const F32 disabledLevel = 0.25;

bool EngineeredObject::isEnabled()
{
   return mHealth >= disabledLevel;
}

void EngineeredObject::damageObject(DamageInfo *di)
{
   F32 prevHealth = mHealth;

   if(di->damageAmount > 0)
      mHealth -= di->damageAmount * .25f;
   else
      mHealth -= di->damageAmount;

   if(mHealth < 0)
      mHealth = 0;
   if(prevHealth == mHealth)
      return;

   setMaskBits(HealthMask);

   if(prevHealth >= disabledLevel && mHealth < disabledLevel)
   {
      if(mTeam != mOriginalTeam)
      {
         mTeam = mOriginalTeam;
         setMaskBits(TeamMask);
      }
      onDisabled();
   }
   else if(prevHealth < disabledLevel && mHealth >= disabledLevel)
   {
      if(mTeam == -1)
      {
         if(di->damagingObject)
         {
            mTeam = di->damagingObject->getTeam();
            setMaskBits(TeamMask);
         }
      }
      onEnabled();
   }

   if(mHealth == 0 && mResource.isValid())
   {
      mIsDestroyed = true;
      onDestroyed();

      mResource->addToDatabase();
      mResource->setActualPos(mAnchorPoint + mAnchorNormal * mResource->getRadius());

      deleteObject(500);
   }
}

void EngineeredObject::computeExtent()
{
   Vector<Point> v;
   getCollisionPoly(v);
   Rect r(v[0], v[0]);
   for(S32 i = 1; i < v.size(); i++)
      r.unionPoint(v[i]);
   setExtent(r);
}

void EngineeredObject::explode()
{
   enum {
      NumShipExplosionColors = 12,
   };

   static Color ShipExplosionColors[NumShipExplosionColors] = {
   Color(1, 0, 0),
   Color(0.9, 0.5, 0),
   Color(1, 1, 1),
   Color(1, 1, 0),
   Color(1, 0, 0),
   Color(0.8, 1.0, 0),
   Color(1, 0.5, 0),
   Color(1, 1, 1),
   Color(1, 0, 0),
   Color(0.9, 0.5, 0),
   Color(1, 1, 1),
   Color(1, 1, 0),
   };

   SFXObject::play(SFXShipExplode, getActualPos(), Point());

   F32 a, b;
   a = Random::readF() * 0.4 + 0.5;
   b = Random::readF() * 0.2 + 0.9;

   F32 c, d;
   c = Random::readF() * 0.15 + 0.125;
   d = Random::readF() * 0.2 + 0.9;

   FXManager::emitExplosion(getActualPos(), 0.65, ShipExplosionColors, NumShipExplosionColors);
   FXManager::emitBurst(getActualPos(), Point(a,c) * 0.6, Color(1,1,0.25), Color(1,0,0));
   FXManager::emitBurst(getActualPos(), Point(b,d) * 0.6, Color(1,1,0), Color(0,1,1));

   disableCollision();
}

bool PolygonsIntersect(Vector<Point> &p1, Vector<Point> &p2)
{
   Point rp1 = p1[p1.size() - 1];
   for(S32 i = 0; i < p1.size(); i++)
   {
      Point rp2 = p1[i];

      Point cp1 = p2[p2.size() - 1];
      for(S32 j = 0; j < p2.size(); j++)
      {
         Point cp2 = p2[j];
         Point ce = cp2 - cp1;
         Point n(-ce.y, ce.x);

         F32 distToZero = n.dot(cp1);

         F32 d1 = n.dot(rp1);
         F32 d2 = n.dot(rp2);

         bool d1in = d1 >= distToZero;
         bool d2in = d2 >= distToZero;

         if(!d1in && !d2in) // both points are outside this edge of the poly, so...
            break;
         else if((d1in && !d2in) || (d2in && !d1in))
         {
            // find the clip intersection point:
            F32 t = (distToZero - d1) / (d2 - d1);
            Point clipPoint = rp1 + (rp2 - rp1) * t;

            if(d1in)
               rp2 = clipPoint;
            else
               rp1 = clipPoint;
         }
         else if(j == p2.size() - 1)
            return true;

         // if both are in, just go to the next edge.
         cp1 = cp2;
      }
      rp1 = rp2;
   }
   return false;
}

bool EngineeredObject::checkDeploymentPosition()
{
   Vector<GameObject *> go;
   Vector<Point> polyBounds;
   getCollisionPoly(polyBounds);

   Rect queryRect = getExtent();
   gServerGame->getGridDatabase()->findObjects(BarrierType | EngineeredType, go, queryRect);
   for(S32 i = 0; i < go.size(); i++)
   {
      Vector<Point> compareBounds;
      go[i]->getCollisionPoly(compareBounds);
      if(PolygonsIntersect(polyBounds, compareBounds))
         return false;
   }
   return true;
}

U32 EngineeredObject::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
   {
      stream->write(mAnchorPoint.x);
      stream->write(mAnchorPoint.y);
      stream->write(mAnchorNormal.x);
      stream->write(mAnchorNormal.y);
   }
   if(stream->writeFlag(updateMask & TeamMask))
      stream->write(mTeam);
   
   if(stream->writeFlag(updateMask & HealthMask))
   {
      stream->writeFloat(mHealth, 6);
      stream->writeFlag(mIsDestroyed);
   }
   return 0;
}

void EngineeredObject::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;
   if(stream->readFlag())
   {
      initial = true;
      stream->read(&mAnchorPoint.x);
      stream->read(&mAnchorPoint.y);
      stream->read(&mAnchorNormal.x);
      stream->read(&mAnchorNormal.y);
      computeExtent();
   }
   if(stream->readFlag())
      stream->read(&mTeam);
   if(stream->readFlag())
   {
      mHealth = stream->readFloat(6);
      bool wasDestroyed = mIsDestroyed;
      mIsDestroyed = stream->readFlag();
      if(mIsDestroyed && !wasDestroyed && !initial)
         explode();
   }
}

TNL_IMPLEMENT_NETOBJECT(ForceFieldProjector);

void ForceFieldProjector::onDisabled()
{
   if(mField.isValid())
      mField->deleteObject(0);
}

void ForceFieldProjector::onEnabled()
{
   Point start = mAnchorPoint + mAnchorNormal * 15;
   Point end = mAnchorPoint + mAnchorNormal * 500;

   F32 t;
   Point n;

   if(findObjectLOS(BarrierType, 0, start, end, t, n))
      end = start + (end - start) * t;

   mField = new ForceField(mTeam, start, end);
   mField->addToGame(getGame());
}

bool ForceFieldProjector::getCollisionPoly(Vector<Point> &polyPoints)
{
   Point cross(mAnchorNormal.y, -mAnchorNormal.x);
   polyPoints.push_back(mAnchorPoint + cross * 12);
   polyPoints.push_back(mAnchorPoint + mAnchorNormal * 15);
   polyPoints.push_back(mAnchorPoint - cross * 12);
   return true;
}

void ForceFieldProjector::render()
{
   renderForceFieldProjector(mAnchorPoint, mAnchorNormal, getGame()->getGameType()->getTeamColor(getTeam()), isEnabled());
}

TNL_IMPLEMENT_NETOBJECT(ForceField);

ForceField::ForceField(S32 team, Point start, Point end)
{
   mTeam = team;
   mStart = start;
   mEnd = end;

   Rect extent(mStart, mEnd);
   extent.expand(Point(5,5));
   setExtent(extent);

   mFieldUp = true;
   mObjectTypeMask = ForceFieldType | CommandMapVisType;
   mNetFlags.set(Ghostable);
}

bool ForceField::collide(GameObject *hitObject)
{
   if(!mFieldUp)
      return false;

   if(!(hitObject->getObjectTypeMask() & ShipType))
      return true;

   if(hitObject->getTeam() == mTeam)
   {
      if(!isGhost())
      {
         mFieldUp = false;
         mDownTimer.reset(FieldDownTime);
         setMaskBits(StatusMask);
      }
      return false;
   }
   return true;
}

void ForceField::idle(GameObject::IdleCallPath path)
{
   if(path == ServerIdleMainLoop)
   {
      if(mDownTimer.update(mCurrentMove.time))
      {
         // do an LOS test to see if anything is in the field:
         F32 t;
         Point n;
         if(!findObjectLOS(ShipType | ItemType, 0, mStart, mEnd, t, n))
         {
            mFieldUp = true;
            setMaskBits(StatusMask);
         }
         else
            mDownTimer.reset(10);
      }
   }
}

U32 ForceField::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitialMask))
   {
      stream->write(mStart.x);
      stream->write(mStart.y);
      stream->write(mEnd.x);
      stream->write(mEnd.y);
      stream->write(mTeam);
   }
   stream->writeFlag(mFieldUp);
   return 0;
}

void ForceField::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool initial = false;
   if(stream->readFlag())
   {
      initial = true;
      stream->read(&mStart.x);
      stream->read(&mStart.y);
      stream->read(&mEnd.x);
      stream->read(&mEnd.y);
      stream->read(&mTeam);

      Rect extent(mStart, mEnd);
      extent.expand(Point(5,5));
      setExtent(extent);
   }
   bool wasUp = mFieldUp;
   mFieldUp = stream->readFlag();

   if(initial || (wasUp != mFieldUp))
      SFXObject::play(mFieldUp ? SFXForceFieldUp : SFXForceFieldDown, mStart, Point());
}

bool ForceField::getCollisionPoly(Vector<Point> &p)
{
   Point normal(mEnd.y - mStart.y, mStart.x - mEnd.x);
   normal.normalize(2.5);

   p.push_back(mStart + normal);
   p.push_back(mEnd + normal);
   p.push_back(mEnd - normal);
   p.push_back(mStart - normal);
   return true;
}

void ForceField::render()
{
   Color c = getGame()->getGameType()->getTeamColor(mTeam);
   renderForceField(mStart, mEnd, c, mFieldUp);
}

TNL_IMPLEMENT_NETOBJECT(Turret);

Turret::Turret(S32 team, Point anchorPoint, Point anchorNormal) :
   EngineeredObject(team, anchorPoint, anchorNormal)
{
   mNetFlags.set(Ghostable);
}


bool Turret::getCollisionPoly(Vector<Point> &polyPoints)
{
   Point cross(mAnchorNormal.y, -mAnchorNormal.x);
   polyPoints.push_back(mAnchorPoint + cross * 25);
   polyPoints.push_back(mAnchorPoint + cross * 10 + mAnchorNormal * 45);
   polyPoints.push_back(mAnchorPoint - cross * 10 + mAnchorNormal * 45);
   polyPoints.push_back(mAnchorPoint - cross * 25);
   return true;
}

void Turret::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);
   mCurrentAngle = atan2(mAnchorNormal.y, mAnchorNormal.x);
}

void Turret::render()
{
   Color c, lightColor;

   if(mTeam != -1 && gClientGame->getGameType())
      c = gClientGame->getGameType()->mTeams[mTeam].color;
   else
      c = Color(1,1,1);

   renderTurret(c, mAnchorPoint, mAnchorNormal, isEnabled(), mHealth, mCurrentAngle, TurretAimOffset);
}

U32 Turret::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 ret = Parent::packUpdate(connection, updateMask, stream);
   if(stream->writeFlag(updateMask & AimMask))
      stream->write(mCurrentAngle);

   return ret;
}

void Turret::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);
   if(stream->readFlag())
      stream->read(&mCurrentAngle);
}

extern bool FindLowestRootInInterval(Point::member_type inA, Point::member_type inB, Point::member_type inC, Point::member_type inUpperBound, Point::member_type &outX);

void Turret::idle(IdleCallPath path)
{
   if(path != ServerIdleMainLoop || !isEnabled())
      return;

   mFireTimer.update(mCurrentMove.time);

   // Choose best target:

   Point aimPos = mAnchorPoint + mAnchorNormal * TurretAimOffset;
   Point cross(mAnchorNormal.y, -mAnchorNormal.x);

   Rect queryRect(aimPos, aimPos);
   queryRect.unionPoint(aimPos + cross * TurretPerceptionDistance);
   queryRect.unionPoint(aimPos - cross * TurretPerceptionDistance);
   queryRect.unionPoint(aimPos + mAnchorNormal * TurretPerceptionDistance);
   fillVector.clear();
   findObjects(TurretTargetType, fillVector, queryRect);

   GameObject * bestTarget = NULL;
   F32 bestRange = 10000.f;
   Point bestDelta;

   Point delta;

   F32 timeScale = F32(mCurrentMove.time) * 0.001f;

   for(S32 i=0; i<fillVector.size(); i++)
   {
      if(fillVector[i]->getObjectTypeMask() & ShipType)
      {
         Ship *potential = (Ship*)fillVector[i];

         // Is it dead or cloaked?
         if((potential->isCloakActive() && !potential->areItemsMounted()) || potential->hasExploded)
            continue;
      }
      GameObject *potential = fillVector[i];

      // Is it on our team?
      if(potential->getTeam() == mTeam)
         continue;

      // Calculate where we have to shoot to hit this...
      const F32 projVel = TurretProjectileVelocity;
      Point Vs = potential->getActualVel();
      F32 S = gWeapons[WeaponTurretBlaster].projVelocity;
      Point d = potential->getRenderPos() - aimPos;

      F32 t;
      if(!FindLowestRootInInterval(Vs.dot(Vs) - S * S, 2 * Vs.dot(d), d.dot(d), gWeapons[WeaponTurretBlaster].projLiveTime * 0.001f, t))
         continue;

      Point leadPos = potential->getRenderPos() + Vs * t;

      // Calculate distance
      delta = (leadPos - aimPos);

      Point angleCheck = delta;
      angleCheck.normalize();
      // Check that we're facing it...
      if(angleCheck.dot(mAnchorNormal) <= -0.1f)
         continue;

      // See if we can see it...
      Point n;
      if(findObjectLOS(BarrierType, 0, aimPos, potential->getActualPos(), t, n))
         continue;

      // See if we're gonna clobber our own stuff...
      disableCollision();
      Point delta2 = delta;
      delta2.normalize(TurretRange);
      GameObject *hitObject = findObjectLOS(ShipType | BarrierType | EngineeredType, 0, aimPos, aimPos + delta2, t, n);
      enableCollision();

      if(hitObject && hitObject->getTeam() == mTeam)
         continue;

      F32 dist = delta.len();

      if(dist < bestRange)
      {
         bestDelta  = delta;
         bestRange  = dist;
         bestTarget = potential;
      }
   }

   if(!bestTarget)
      return;

   // ok, now change our aim to be towards the best target:
   F32 destAngle = atan2(bestDelta.y, bestDelta.x);

   F32 angleDelta = destAngle - mCurrentAngle;
   if(angleDelta > FloatPi)
      angleDelta -= Float2Pi;
   if(angleDelta < -FloatPi)
      angleDelta += Float2Pi;

   bool canFire = false;
   F32 maxTurn = TurretTurnRate * mCurrentMove.time * 0.001f;
   if(angleDelta != 0)
      setMaskBits(AimMask);

   if(angleDelta > maxTurn)
      mCurrentAngle += maxTurn;
   else if(angleDelta < -maxTurn)
      mCurrentAngle -= maxTurn;
   else
   {
      mCurrentAngle = destAngle;

      if(mFireTimer.getCurrent() == 0)
      {
         bestDelta.normalize();
         Point velocity;
         createWeaponProjectiles(WeaponTurretBlaster, bestDelta, aimPos, velocity, 30.0f, this);
         mFireTimer.reset(TurretFireDelay);
      }
   }
}

};
