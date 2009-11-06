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

#include "loadoutSelect.h"
#include "UIGame.h"
#include "gameConnection.h"
#include "game.h"
#include "glutInclude.h"
#include "shipItems.h"
#include "gameWeapons.h"
#include <ctype.h>
#include "gameObjectRender.h"

namespace Zap
{

struct LoadoutItem
{
   U32 key;
   U32 button;
   U32 index;
   const char *text;
};

LoadoutItem gLoadoutModules[] = {
   { '1', 0, ModuleBoost, "Turbo Boost" },
   { '2', 1, ModuleShield, "Shield Generator" },
   { '3', 2, ModuleRepair, "Repair Module", },
   { '4', 3, ModuleSensor, "Enhanced Sensor" },
   { '5', 4, ModuleCloak, "Cloak Field Modulator" },
   { 0, 0, 0, NULL },
};

LoadoutItem gLoadoutWeapons[] = {
   { '1', 0, WeaponPhaser,    "Phaser" },
   { '2', 1, WeaponBounce,    "Bouncer" },
   { '3', 2, WeaponTriple,    "Three-Way" },
   { '4', 3, WeaponBurst,     "Burster" },
   { '5', 4, WeaponMineLayer, "Mine Dropper" },
   { 0, 0, 0, NULL },
};

const char *gLoadoutTitles[] = {
   "Choose Primary Module:",
   "Choose Secondary Module:",
   "Choose Primary Weapon:",
   "Choose Secondary Weapon:",
   "Choose Tertiary Weapon:",
};

LoadoutHelper::LoadoutHelper()
{
}

void LoadoutHelper::show(bool fromController)
{
   mFromController = fromController;
   mCurrentIndex = 0;
   mIdleTimer.reset(MenuTimeout);
}

extern void renderControllerButton(F32 x, F32 y, U32 buttonIndex, U32 keyIndex);

void LoadoutHelper::render()
{
   S32 curPos = 300;
   const int fontSize = 15;

   glColor3f(0.8, 1, 0.8);

   UserInterface::drawStringf(UserInterface::horizMargin, curPos, fontSize, "%s", gLoadoutTitles[mCurrentIndex]); 
   curPos += fontSize + 4;

   LoadoutItem *list;

   if(mCurrentIndex < 2)
      list = gLoadoutModules;
   else
      list = gLoadoutWeapons;

   for(S32 i = 0; list[i].text; i++)
   {
      Color c;
      if((mCurrentIndex == 1 && mModule[0] == i) ||
         (mCurrentIndex == 3 && mWeapon[0] == i) ||
         (mCurrentIndex == 4 && (mWeapon[0] == i || mWeapon[1] == i)))
         c.set(0.3, 0.7, 0.3);
      else
         c.set(0.1, 1.0, 0.1);

      glColor(c);
      renderControllerButton(UserInterface::horizMargin, curPos, list[i].button, list[i].key);

      glColor(c);
      UserInterface::drawStringf(UserInterface::horizMargin + 20, curPos, fontSize, "- %s", 
         list[i].text);
      curPos += fontSize + 4;
   }

}

void LoadoutHelper::idle(U32 delta)
{
   if(mIdleTimer.update(delta))
      gGameUserInterface.setPlayMode();
}

bool LoadoutHelper::processKey(U32 key)
{
   if(key == 27 || key == 8)
   {
      gGameUserInterface.setPlayMode();
      return true;
   }
   if(!mFromController)
      key = toupper(key);
   U32 index;
   LoadoutItem *list;
   if(mCurrentIndex < 2)
      list = gLoadoutModules;
   else
      list = gLoadoutWeapons;

   for(index = 0; list[index].text; index++)
   {
      if(key == list[index].key ||
         key == list[index].button)
      {
         break;
      }
   }
   if(!list[index].text)
      return false;
   mIdleTimer.reset(MenuTimeout);

   switch(mCurrentIndex)
   {
      case 0:
         mModule[0] = index;
         mCurrentIndex++;
         break;
      case 1:
         if(mModule[0] != index)
         {
            mModule[1] = index;
            mCurrentIndex++;
         }
         break;
      case 2:
         mWeapon[0] = index;
         mCurrentIndex++;
         break;
      case 3:
         if(mWeapon[0] != index)
         {
            mWeapon[1] = index;
            mCurrentIndex++;
         }
         break;
      case 4:
         if(mWeapon[1] != index && mWeapon[0] != index)
         {
            mWeapon[2] = index;
            mCurrentIndex++;
         }
         break;
   }
   if(mCurrentIndex == 5)
   {
      // do the processing thang
      Vector<U32> loadout;
      loadout.push_back(gLoadoutModules[mModule[0]].index);
      loadout.push_back(gLoadoutModules[mModule[1]].index);
      loadout.push_back(gLoadoutWeapons[mWeapon[0]].index);
      loadout.push_back(gLoadoutWeapons[mWeapon[1]].index);
      loadout.push_back(gLoadoutWeapons[mWeapon[2]].index);
      GameConnection *gc = gClientGame->getConnectionToServer();
      if(gc)
         gc->c2sRequestLoadout(loadout);
      gGameUserInterface.setPlayMode();
   }
   return true;
}

};

