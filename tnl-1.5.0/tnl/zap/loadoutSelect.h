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

#ifndef _LOADOUTSELECT_H_
#define _LOADOUTSELECT_H_

#include "tnlTypes.h"
using namespace TNL;
#include "timer.h"

namespace Zap
{

class LoadoutHelper
{
   bool mFromController;
   U32 mModule[2];
   U32 mWeapon[3];
   U32 mCurrentIndex;
   Timer mIdleTimer;
   enum {
      MenuTimeout = 3500,
   };
public:
   LoadoutHelper();

   void render();
   void idle(U32 delta);
   void show(bool fromController);
   bool processKey(U32 key);
};

};

#endif

