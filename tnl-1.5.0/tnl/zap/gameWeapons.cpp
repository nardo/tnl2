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

namespace Zap
{


// do not add a weapon with a fire delay > Ship::MaxFireDelay
// or update the constant.

ShipWeaponInfo gWeapons[] =
{
   ShipWeaponInfo(StringTableEntry("Phaser"),   100,  500,   500,   600, 1000, ProjectilePhaser ),
   ShipWeaponInfo(StringTableEntry("Bouncer"),  100,  1800,  1800,  540, 1500, ProjectileBounce ),
   ShipWeaponInfo(StringTableEntry("Triple"),   200,  2100,  2100,  550, 850,  ProjectileTriple ),
   ShipWeaponInfo(StringTableEntry("Burst"),    700,  5000,  5000,  500, 1000, 0 ),
   ShipWeaponInfo(StringTableEntry("Mine"),     900,  65000, 65000, 500, 1000, 0 ),
   ShipWeaponInfo(StringTableEntry("Turret"),   0,    0,     0,     800, 800,  ProjectileTurret ),
};

ProjectileInfo gProjInfo[ProjectileTypeCount] = {
   ProjectileInfo( .21f, Color(1,0,1), Color(1,1,1), Color(0,0,1), Color(1,0,0), Color(1, 0, 0.5), Color(0.5, 0, 1), 1, SFXPhaserProjectile, SFXPhaserImpact ),
   ProjectileInfo( 0.15f,Color(1,1,0), Color(1,0,0), Color(1,0.5,0), Color(1,1,1), Color(1, 1, 0), Color(1, 0, 0), 1.3, SFXBounceProjectile, SFXBounceImpact ),
   ProjectileInfo( .14f, Color(0,0,1), Color(0,1,0), Color(0,0.5,1), Color(0,1,0.5), Color(0, 0.5, 1), Color(0, 1, 0.5), 0.7, SFXTripleProjectile, SFXTripleImpact ),
   ProjectileInfo( .11f, Color(0,1,1), Color(1,1,0), Color(0,1,0.5), Color(0.5,1,0), Color(0.5, 1, 0), Color(0, 1, 0.5), 0.6, SFXTurretProjectile, SFXTurretImpact ),
};

void createWeaponProjectiles(U32 weapon, Point &dir, Point &shooterPos, Point &shooterVel, F32 shooterRadius, GameObject *shooter)
{
   GameObject *proj = NULL;
   ShipWeaponInfo *wi = gWeapons + weapon;
   Point projVel = dir * F32(wi->projVelocity) + dir * shooterVel.dot(dir);
   Point firePos = shooterPos + dir * shooterRadius;

   switch(weapon)
   {
      case WeaponTriple:
         {
            Point velPerp(projVel.y, -projVel.x);
            velPerp.normalize(50.0f);
            (new Projectile(wi->projectileType, firePos, projVel + velPerp, wi->projLiveTime, shooter))->addToGame(shooter->getGame());
            (new Projectile(wi->projectileType, firePos, projVel - velPerp, wi->projLiveTime, shooter))->addToGame(shooter->getGame());
         }
      case WeaponPhaser:
      case WeaponBounce:
      case WeaponTurretBlaster:
         (new Projectile(wi->projectileType, firePos, projVel, wi->projLiveTime, shooter))->addToGame(shooter->getGame());
         break;
      case WeaponBurst:
         (new GrenadeProjectile(firePos, projVel, wi->projLiveTime, shooter))->addToGame(shooter->getGame());
         break;
      case WeaponMineLayer:
         (new Mine(firePos, dynamic_cast<Ship*>(shooter)))->addToGame(shooter->getGame());
         break;
   }
}

};