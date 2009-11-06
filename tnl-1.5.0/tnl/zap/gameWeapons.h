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

#ifndef _GAMEWEAPONS_H_
#define _GAMEWEAPONS_H_

#include "tnlPlatform.h"
#include "point.h"
#include "gameObject.h"

using namespace TNL;
namespace Zap
{

enum WeaponTypes
{
   WeaponPhaser = 0,
   WeaponBounce,
   WeaponTriple,
   WeaponBurst,
   WeaponMineLayer,
   WeaponTurretBlaster,
   WeaponCount,
};

enum WeaponConsts
{
   MaxFireDelay = 2048,
};

struct ShipWeaponInfo
{
   ShipWeaponInfo(StringTableEntry _name, U32 _fireDelay, U32 _minEnergy, U32 _drainEnergy, U32 _projVelocity, U32 _projLiveTime, U32 _projectileType)
   {
      name = _name;
      fireDelay = _fireDelay;
      minEnergy = _minEnergy;
      drainEnergy = _drainEnergy;
      projVelocity = _projVelocity;
      projLiveTime = _projLiveTime;
      projectileType = _projectileType;
   }

   StringTableEntry name; // Display name of the weapon.
   U32 fireDelay;    // Delay between shots.
   U32 minEnergy;    // Minimum energy to fire.
   U32 drainEnergy;  // Amount of energy to drain per shot.
   U32 projVelocity;
   U32 projLiveTime;
   U32 projectileType;
};

extern ShipWeaponInfo gWeapons[WeaponCount];
extern void createWeaponProjectiles(U32 weapon, Point &dir, Point &shooterPos, Point &shooterVel, F32 shooterRadius, GameObject *shooter);

enum {
   NumSparkColors = 4,
};

enum ProjectileType
{
   ProjectilePhaser,
   ProjectileBounce,
   ProjectileTriple,
   ProjectileTurret,
   ProjectileTypeCount,
};

struct ProjectileInfo
{
   ProjectileInfo(F32 _damageAmount, Color _sparkColor1, Color _sparkColor2, Color _sparkColor3, Color _sparkColor4, Color _projColor1, Color _projColor2, F32 _scaleFactor, U32 _projectileSound, U32 _impactSound )
   {
      damageAmount = _damageAmount;
      sparkColors[0] = _sparkColor1;
      sparkColors[1] = _sparkColor2;
      sparkColors[2] = _sparkColor3;
      sparkColors[3] = _sparkColor4;
      projColors[0] = _projColor1;
      projColors[1] = _projColor2;
      scaleFactor = _scaleFactor;
      projectileSound = _projectileSound;
      impactSound = _impactSound;
   }

   F32   damageAmount;
   Color sparkColors[NumSparkColors];
   Color projColors[2];
   F32   scaleFactor;
   U32   projectileSound;
   U32   impactSound;
};

extern ProjectileInfo gProjInfo[ProjectileTypeCount];

};

#endif