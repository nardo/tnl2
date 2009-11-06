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

#ifndef _SHIP_H_
#define _SHIP_H_

#include "moveObject.h"
#include "sparkManager.h"
#include "sfx.h"
#include "timer.h"
#include "shipItems.h"

namespace Zap
{

class Item;

class Ship : public MoveObject
{
   typedef MoveObject Parent;
public:
   enum {
      MaxVelocity = 450, // points per second
      Acceleration = 2500, // points per second per second
      BoostMaxVelocity = 700, // points per second
      BoostAcceleration = 5000, // points per second per second

      RepairRadius = 65,
      RepairDisplayRadius = 18,
      CollisionRadius = 24,
      VisibilityRadius = 30,
      KillDeleteDelay = 1500,
      ExplosionFadeTime = 300,
      MaxControlObjectInterpDistance = 200,
      TrailCount = 2,
      EnergyMax = 100000,
      EnergyRechargeRate = 6000, // How many percent/second
      EnergyBoostDrain = 15000,
      EnergyShieldDrain = 27000,
      EnergyRepairDrain = 15000, 
      EnergySensorDrain = 8000,
      EnergyCloakDrain = 8000,
      EnergyEngineerCost = 75000,
      EnergyShieldHitDrain = 20000,
      EnergyCooldownThreshold = 15000,
      WeaponFireDecloakTime = 350,
      ShipModuleCount = 2,
      ShipWeaponCount = 3,
      SensorZoomTime = 300,
      CloakFadeTime = 300,
      CloakCheckRadius = 200,
      RepairHundredthsPerSecond = 16,
      MaxEngineerDistance = 100,
      WarpFadeInTime = 500,
   };

   enum MaskBits {
      InitialMask = BIT(0),
      PositionMask = BIT(1),
      MoveMask = BIT(2),
      WarpPositionMask = BIT(3),
      ExplosionMask = BIT(4),
      HealthMask = BIT(5),
      PowersMask = BIT(6),
      LoadoutMask = BIT(7),  
   };

   Timer mFireTimer;
   Timer mWarpInTimer;
   F32 mHealth;
   S32 mEnergy;
   StringTableEntry mPlayerName;
   bool mCooldown;
   U32 mSensorStartTime;

   U32 mModule[ShipModuleCount];
   bool mModuleActive[ModuleCount];
   SFXHandle mModuleSound[ModuleCount];

   U32 mWeapon[ShipWeaponCount];
   U32 mActiveWeapon;

   void selectWeapon();
   void selectWeapon(U32 weaponIndex);

   Timer mSensorZoomTimer;
   Timer mWeaponFireDecloakTimer;
   Timer mCloakTimer;

   U32 mSparkElapsed;
   S32 mLastTrailPoint[TrailCount];
   FXTrail mTrail[TrailCount];

   F32 mass; // mass of ship
   bool hasExploded;

   Vector<SafePtr<Item> > mMountedItems;
   Vector<SafePtr<GameObject> > mRepairTargets;

   void render();
   Ship(StringTableEntry playerName="", S32 team = -1, Point p = Point(0,0), F32 m = 1.0);

   F32 getHealth() { return mHealth; }

   void onGhostRemove();

   bool isShieldActive() { return mModuleActive[ModuleShield]; }
   bool isBoostActive() { return mModuleActive[ModuleBoost]; }
   bool isCloakActive() { return mModuleActive[ModuleCloak]; }
   bool isSensorActive() { return mModuleActive[ModuleSensor]; }
   bool isRepairActive() { return mModuleActive[ModuleRepair]; }
   bool isEngineerActive() { return mModuleActive[ModuleEngineer]; }
   bool engineerBuildObject()
   {
      if(mEnergy < EnergyEngineerCost)
         return false;
      mEnergy -= EnergyEngineerCost;
      return true;
   }

   bool hasEngineeringModule() { return mModule[0] == ModuleEngineer ||
                                        mModule[1] == ModuleEngineer; }

   bool isDestroyed() { return hasExploded; }
   bool areItemsMounted() { return mMountedItems.size() != 0; }
   bool carryingResource();
   Item *unmountResource();

   F32 getSensorZoomFraction() { return mSensorZoomTimer.getFraction(); }
   Point getAimVector();

   void setLoadout(U32 module1, U32 module2, U32 weapon1, U32 weapon2, U32 weapon3);
   void idle(IdleCallPath path);

   void processMove(U32 stateIndex);
   void processWeaponFire();
   void processEnergy();
   void updateModuleSounds();
   void emitMovementSparks();
   bool findRepairTargets();
   void repairTargets();

   void controlMoveReplayComplete();

   void emitShipExplosion(Point pos);
   void setActualPos(Point p);

   virtual void damageObject(DamageInfo *theInfo);
   void kill(DamageInfo *theInfo);
   void kill();

   void writeControlState(BitStream *stream);
   void readControlState(BitStream *stream);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void processArguments(S32 argc, const char **argv);

   TNL_DECLARE_CLASS(Ship);
};

};

#endif
