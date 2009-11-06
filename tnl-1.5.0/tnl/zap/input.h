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

#ifndef _INPUT_H_
#define _INPUT_H_
#include "tnl.h"
using namespace TNL;

namespace Zap
{

enum ControllerTypeType
{
   LogitechWingman,
   LogitechDualAction,
   SaitekDualAnalog,
   PS2DualShock,
   XBoxController,
   XBoxControllerOnXBox,
   ControllerTypeCount,
};

enum ButtonInfo {
   MaxJoystickAxes = 12,
   MaxJoystickButtons = 14,
   ControllerButton1 = 1 << 0,
   ControllerButton2 = 1 << 1,
   ControllerButton3 = 1 << 2,
   ControllerButton4 = 1 << 3,
   ControllerButton5 = 1 << 4,
   ControllerButton6 = 1 << 5,
   ControllerButtonLeftTrigger = 1 << 6,
   ControllerButtonRightTrigger = 1 << 7,
   ControllerGameButtonCount = 8,
   ControllerButtonStart = 1 << 8,
   ControllerButtonBack = 1 << 9,
   ControllerButtonDPadUp = 1 << 10,
   ControllerButtonDPadDown = 1 << 11,
   ControllerButtonDPadLeft = 1 << 12,
   ControllerButtonDPadRight = 1 << 13,
};

// the following functions are defined differently on each platform
// in the the platInput.cpp files.

void getModifierState( bool &shiftDown, bool &controlDown, bool &altDown );

void InitJoystick();
const char *GetJoystickName();
S32 autodetectJoystickType();
void ShutdownJoystick();
bool ReadJoystick(F32 axes[MaxJoystickAxes], U32 &buttonMask, U32 &hatMask);

};

#endif