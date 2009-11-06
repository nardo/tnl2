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

#include "input.h"
#include "move.h"
#include "UIMenus.h"
#include "point.h"
#include "tnlJournal.h"
#include <math.h>
#include "glutInclude.h"
#include "gameObjectRender.h"

namespace Zap
{

extern void drawCircle(Point pos, F32 radius);
void renderControllerButton(F32 x, F32 y, U32 buttonIndex, U32 keyIndex)
{
   S32 joy = OptionsMenuUserInterface::joystickType;

   if(joy == -1)
      UserInterface::drawStringf(x, y, 15, "%c", keyIndex);
   else if(joy == LogitechWingman)
   {
      glColor3f(1,1,1);
      const char buttons[] = "ABCXYZ";
      drawCircle(Point(x + 8, y + 8), 8);
      UserInterface::drawStringf(x + 4, y + 2, 12, "%c", buttons[buttonIndex]);
   }
   else if(joy == LogitechDualAction)
   {
      glColor3f(1,1,1);
      const char buttons[] = "12347856";
      drawCircle(Point(x + 8, y + 8), 8);
      UserInterface::drawStringf(x + 4, y + 2, 12, "%c", buttons[buttonIndex]);
   }
   else if(joy == SaitekDualAnalog)
   {
      glColor3f(1,1,1);
      const char buttons[] = "123456";
      drawCircle(Point(x + 8, y + 8), 8);
      UserInterface::drawStringf(x + 4, y + 2, 12, "%c", buttons[buttonIndex]);
   }
   else if(joy == PS2DualShock)
   {
      static F32 color[6][3] = { { 0.5, 0.5, 1 },
      { 1, 0.5, 0.5 },
      { 1, 0.5, 1 },
      { 0.5, 1, 0.5 },
      { 0.7, 0.7, 0.7 },
      { 0.7, 0.7, 0.7 }
      };
      Color c(color[buttonIndex][0], color[buttonIndex][1], color[buttonIndex][2]);
      glColor3f(1.0, 1.0, 1.0);
      Point center(x + 8, y + 8);
      if(buttonIndex < 4)
      {
         drawCircle(center, 8);
         glColor(c);
         switch(buttonIndex)
         {
         case 0:
            glBegin(GL_LINES);
            glVertex(center + Point(-5, -5));
            glVertex(center + Point(5, 5));
            glVertex(center + Point(-5, 5));
            glVertex(center + Point(5, -5));
            glEnd();
            break;
         case 1:
            drawCircle(center, 5);
            break;
         case 2:
            glBegin(GL_LINE_LOOP);
            glVertex(center + Point(-5, -5));
            glVertex(center + Point(-5, 5));
            glVertex(center + Point(5, 5));
            glVertex(center + Point(5, -5));
            glEnd();
            break;
         case 3:
            glBegin(GL_LINE_LOOP);
            glVertex(center + Point(0, -5));
            glVertex(center + Point(5, 3));
            glVertex(center + Point(-5, 3));
            glEnd();
            break;
         }
      }
      else
      {
         glBegin(GL_LINE_LOOP);
         glVertex(center + Point(-8, -8));
         glVertex(center + Point(-8, 8));
         glVertex(center + Point(10, 8));
         glVertex(center + Point(10, -8));
         glEnd();
         static const char *buttonIndexString[4] = { 
            "L2", "R2", "L1", "R1"
         };
         UserInterface::drawString(x + 2, y + 2, 11, buttonIndexString[buttonIndex - 4]);
      }
   }
   else if(joy == XBoxController || joy == XBoxControllerOnXBox)
   {
      static F32 color[6][3] = { { 0, 1, 0 }, 
      { 1, 0, 0 }, 
      { 0, 0, 1 }, 
      { 1, 1, 0 }, 
      { 1, 1, 1 },
      { 0, 0, 0} };
      Color c(color[buttonIndex][0], color[buttonIndex][1], color[buttonIndex][2]);

      glColor(c * 0.8f);
      fillCircle(Point(x + 8, y + 8), 8);
      const char buttons[] = "ABXY  ";
      glColor3f(1,1,1);
      drawCircle(Point(x + 8, y + 8), 8);
      glColor(c);
      UserInterface::drawStringf(x + 4, y + 2, 12, "%c", buttons[buttonIndex]);
   }
}

static U32 gShootAxisRemaps[ControllerTypeCount][2] =
{
   { 5, 6 }, { 2, 5 }, { 5, 2 }, { 2, 5 }, { 3, 4 }, { 3, 4 },
};

static U32 gControllerButtonRemaps[ControllerTypeCount][MaxJoystickButtons] =
{
   { // LogitechWingman
      ControllerButton1,
      ControllerButton2,
      ControllerButton3,
      ControllerButton4,
      ControllerButton5,
      ControllerButton6,
      ControllerButtonLeftTrigger,
      ControllerButtonRightTrigger,
      ControllerButtonBack,
      0,
      0,
      0,
      0,
      0,
   },
   { // LogitechDualAction
      ControllerButton1,
      ControllerButton2,
      ControllerButton3,
      ControllerButton4,
      ControllerButtonLeftTrigger,
      ControllerButtonRightTrigger,
      ControllerButton5,
      ControllerButton6,
      ControllerButtonBack,
      ControllerButtonStart,
      0,
      0,
      0,
      0,
   },
   { // SaitekDualAnalog
      ControllerButton1,
      ControllerButton2,
      ControllerButton3,
      ControllerButton4,
      ControllerButton5,
      ControllerButton6,
      ControllerButtonLeftTrigger,
      ControllerButtonRightTrigger,
      0,
      0,
      ControllerButtonBack,
      0,
      0,
      0,
   },
   { // PS2DualShock
      ControllerButton4,
      ControllerButton2,
      ControllerButton1,
      ControllerButton3,
      ControllerButton5,
      ControllerButton6,
      ControllerButtonLeftTrigger,
      ControllerButtonRightTrigger,
      ControllerButtonBack,
      0,
      0,
      ControllerButtonStart,
      0,
      0,
   },
   { // XBoxController
      ControllerButton1,
      ControllerButton2,
      ControllerButton3,
      ControllerButton4,
      ControllerButton6,
      ControllerButton5,
      ControllerButtonStart,
      ControllerButtonBack,
      0,
      0,
      ControllerButtonLeftTrigger,
      ControllerButtonRightTrigger,
      0,
      0,
   },
   { // XBoxControllerOnXBox
      1 << 0,
      1 << 1,
      1 << 2,
      1 << 3,
      1 << 4,
      1 << 5,
      1 << 6,
      1 << 7,
      1 << 8,
      1 << 9,
      1 << 10,
      1 << 11,
      1 << 12,
      1 << 13,
   }
};

static bool updateMoveInternal( Move *theMove, U32 &buttonMask )
{
   F32 axes[MaxJoystickAxes];
   static F32 minValues[2] = { - 0.5, -0.5 };
   static F32 maxValues[2] = { 0.5, 0.5 };
   U32 hatMask = 0;
   if(!ReadJoystick(axes, buttonMask, hatMask))
      return false;

   // All axes return -1 to 1
   // now we map the controls:

   F32 controls[4];
   controls[0] = axes[0];
   controls[1] = axes[1];

   if(controls[0] < minValues[0])
      minValues[0] = controls[0];
   if(controls[0] > maxValues[0])
      maxValues[0] = controls[0];
   if(controls[1] < minValues[1])
      minValues[1] = controls[1];
   if(controls[1] > maxValues[1])
      maxValues[1] = controls[1];

   if(controls[0] < 0)
      controls[0] = - (controls[0] / minValues[0]);
   else if(controls[0] > 0)
      controls[0] = (controls[0] / maxValues[0]);

   if(controls[1] < 0)
      controls[1] = - (controls[1] / minValues[1]);
   else if(controls[1] > 0)
      controls[1] = (controls[1] / maxValues[1]);

   // xbox control inputs are in a circle, not a square, which makes
   // diagonal movement inputs "slower"
   if(OptionsMenuUserInterface::joystickType == XBoxController ||
      OptionsMenuUserInterface::joystickType == XBoxControllerOnXBox)
   {
      Point dir(controls[0], controls[1]);
      F32 absX = fabs(dir.x);
      F32 absY = fabs(dir.y);

      // push out to the edge of the square (-1,-1 -> 1,1 )

      F32 dirLen = dir.len() * 1.25;
      if(dirLen > 1)
         dirLen = 1;

      if(absX > absY)
         dir *= F32(dirLen / absX);
      else
         dir *= F32(dirLen / absY);
      controls[0] = dir.x;
      controls[1] = dir.y;
   }
   controls[2] = axes[gShootAxisRemaps[OptionsMenuUserInterface::joystickType][0]];
   controls[3] = axes[gShootAxisRemaps[OptionsMenuUserInterface::joystickType][1]];

   for(U32 i = 0; i < 4; i++)
   {
      F32 deadZone = i < 2 ? 0.25f : 0.03125f;
      if(controls[i] < -deadZone)
         controls[i] = -(-controls[i] - deadZone) / F32(1 - deadZone);
      else if(controls[i] > deadZone)
         controls[i] = (controls[i] - deadZone) / F32(1 - deadZone);
      else
         controls[i] = 0;
   }
   if(controls[0] < 0)
   {
      theMove->left = -controls[0];
      theMove->right = 0;
   }
   else
   {
      theMove->left = 0;
      theMove->right = controls[0];
   }

   if(controls[1] < 0)
   {
      theMove->up = -controls[1];
      theMove->down = 0;
   }
   else
   {
      theMove->down = controls[1];
      theMove->up = 0;
   }

   Point p(controls[2], controls[3]);
   F32 plen = p.len();
   if(plen > 0.3)
   {
      theMove->angle = atan2(p.y, p.x);
      theMove->fire = (plen > 0.5);
   }
   else
      theMove->fire = false;

   // remap button inputs
   U32 retMask = 0;
   for(S32 i = 0; i < MaxJoystickButtons; i++)
      if(buttonMask & (1 << i))
         retMask |= gControllerButtonRemaps[OptionsMenuUserInterface::joystickType][i];
   buttonMask = retMask | hatMask;
   return true;
}

S32 autodetectJoystickType()
{
   S32 ret = -1;
   TNL_JOURNAL_READ_BLOCK(JoystickAutodetect, 
      TNL_JOURNAL_READ((&ret));
      return ret;
      )
   const char *joystickName = GetJoystickName();
   if(!strncmp(joystickName, "WingMan", 7))
      ret = LogitechWingman;
   else if(!strcmp(joystickName, "XBoxOnXBox"))
      ret = XBoxControllerOnXBox;
   else if(strstr(joystickName, "XBox"))
      ret = XBoxController;
   else if(!strcmp(joystickName, "4 axis 16 button joystick"))
      ret = PS2DualShock;
   else if(strstr(joystickName, "P880"))
      ret = SaitekDualAnalog;
   else if(strstr(joystickName, "Logitech Dual Action"))
      ret = LogitechDualAction;
   TNL_JOURNAL_WRITE_BLOCK(JoystickAutodetect,
      TNL_JOURNAL_WRITE((ret));
   )
   return ret;
}

static bool updateMoveJournaled( Move *theMove, U32 &buttonMask )
{
   TNL_JOURNAL_READ_BLOCK(JoystickUpdate,
      BitStream *readStream = Journal::getReadStream();
      if(!readStream->readFlag())
         return false;

      Move aMove;
      aMove.unpack(readStream, false);
      *theMove = aMove;
      buttonMask = readStream->readInt(MaxJoystickButtons);
      return true;
   )

   bool ret = updateMoveInternal(theMove, buttonMask);

   TNL_JOURNAL_WRITE_BLOCK(JoystickUpdate,
      BitStream *writeStream = Journal::getWriteStream();
      if(writeStream->writeFlag(ret))
      {
         Move dummy;
         theMove->pack(writeStream, &dummy, false);
         writeStream->writeInt(buttonMask, MaxJoystickButtons);
      }
   )
   return ret;
}



void JoystickUpdateMove(Move *theMove)
{
   static U32 lastButtonsPressed = 0;
   U32 buttonsPressed;
   if(!updateMoveJournaled(theMove, buttonsPressed))
      return;

   U32 buttonDown = buttonsPressed & ~lastButtonsPressed;
   U32 buttonUp = ~buttonsPressed & lastButtonsPressed;
   lastButtonsPressed = buttonsPressed;

   for(U32 i = 0; i < ControllerGameButtonCount; i++)
   {
      U32 mask = 1 << i;
      if(buttonDown & mask)
         UserInterface::current->onControllerButtonDown(i);
      else if(buttonUp & mask)
         UserInterface::current->onControllerButtonUp(i);
   }
   if(buttonDown & ControllerButtonStart)
      UserInterface::current->onKeyDown('\r');
   if(buttonDown & ControllerButtonBack)
      UserInterface::current->onKeyDown(27);
   if(buttonDown & ControllerButtonDPadUp)
      UserInterface::current->onSpecialKeyDown(GLUT_KEY_UP);
   if(buttonDown & ControllerButtonDPadDown)
      UserInterface::current->onSpecialKeyDown(GLUT_KEY_DOWN);
   if(buttonDown & ControllerButtonDPadLeft)
      UserInterface::current->onSpecialKeyDown(GLUT_KEY_LEFT);
   if(buttonDown & ControllerButtonDPadRight)
      UserInterface::current->onSpecialKeyDown(GLUT_KEY_RIGHT);

   if(buttonUp & ControllerButtonStart)
      UserInterface::current->onKeyUp('\r');
   if(buttonUp & ControllerButtonBack)
      UserInterface::current->onKeyUp(27);
   if(buttonUp & ControllerButtonDPadUp)
      UserInterface::current->onSpecialKeyUp(GLUT_KEY_UP);
   if(buttonUp & ControllerButtonDPadDown)
      UserInterface::current->onSpecialKeyUp(GLUT_KEY_DOWN);
   if(buttonUp & ControllerButtonDPadLeft)
      UserInterface::current->onSpecialKeyUp(GLUT_KEY_LEFT);
   if(buttonUp & ControllerButtonDPadRight)
      UserInterface::current->onSpecialKeyUp(GLUT_KEY_RIGHT);
}


};