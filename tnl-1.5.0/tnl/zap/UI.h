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

#ifndef _UI_H_
#define _UI_H_

#include "../tnl/tnl.h"

using namespace TNL;

namespace Zap
{

static const float DefaultLineWidth = 2.0f;

class UserInterface
{
public:
   static UserInterface *current;

   static S32 windowWidth, windowHeight, canvasWidth, canvasHeight;
   static S32 vertMargin, horizMargin;

   static void renderCurrent();

   virtual void render();
   virtual void idle(U32 timeDelta);
   virtual void onActivate();

   void activate();

   virtual void onMouseDown(S32 x, S32 y);
   virtual void onMouseUp(S32 x, S32 y);
   virtual void onRightMouseDown(S32 x, S32 y);
   virtual void onRightMouseUp(S32 x, S32 y);

   virtual void onMouseMoved(S32 x, S32 y);
   virtual void onMouseDragged(S32 x, S32 y);
   virtual void onKeyDown(U32 key);
   virtual void onKeyUp(U32 key);
   virtual void onSpecialKeyDown(U32 key);
   virtual void onSpecialKeyUp(U32 key);
   virtual void onModifierKeyDown(U32 key);
   virtual void onModifierKeyUp(U32 key);
   virtual void onControllerButtonDown(U32 buttonIndex) {}
   virtual void onControllerButtonUp(U32 buttonIndex) {}

   static void drawString(S32 x, S32 y, S32 size, const char *string);
   static void drawCenteredString(S32 y, S32 size, const char *string);
   static void drawStringf(S32 x, S32 y, S32 size, const char *fmt, ...);
   static void drawCenteredStringf(S32 y, S32 size, const char *fmt, ...);
   static U32 getStringWidth(S32 size, const char *string, U32 len = 0);
   static void playBoop();
};

};

#endif
