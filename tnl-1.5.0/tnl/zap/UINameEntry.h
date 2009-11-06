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

#ifndef _UINAMEENTRY_H_
#define _UINAMEENTRY_H_

#include "UI.h"
#include "timer.h"

namespace Zap
{

class TextEntryUserInterface : public UserInterface
{
   enum {
      MaxTextLen = 32,
      BlinkTime = 100,
   };
   char buffer[MaxTextLen+1];
   U32 cursorPos;
   bool blink;
   Timer mBlinkTimer;

protected:
   const char *title;
   bool secret;
   bool resetOnActivate;
public:
   TextEntryUserInterface() : mBlinkTimer(BlinkTime)
   { 
      title = "ENTER TEXT:"; 
      buffer[0] = 0; 
      memset(buffer, 0, sizeof(buffer));
      blink = false;
      secret = false;
      cursorPos = 0;
      resetOnActivate = true;
   }

   void onActivate();
   void render();
   void idle(U32 t);

   void onKeyDown(U32 key);
   void onKeyUp(U32 key);
   
   virtual void onAccept(const char *text) = 0;
   virtual void onEscape() = 0;
   const char *getText() { return buffer; }
   void setText(const char *text);
};

class NameEntryUserInterface : public TextEntryUserInterface
{
public:
   NameEntryUserInterface()
   { 
      title = "ENTER YOUR NAME:"; 
      resetOnActivate = false;
   }
   virtual void onAccept(const char *text);
   virtual void onEscape();
};

extern NameEntryUserInterface gNameEntryUserInterface;

class PasswordEntryUserInterface : public TextEntryUserInterface
{
   Address connectAddress;
public:
   PasswordEntryUserInterface()
   { 
      title = "ENTER SERVER PASSWORD:";
      secret = true;
   }
   void onAccept(const char *text);
   void onEscape();
   void setConnectServer(const Address &addr) { connectAddress = addr; }
};

extern PasswordEntryUserInterface gPasswordEntryUserInterface;

class AdminPasswordEntryUserInterface : public TextEntryUserInterface
{
   typedef TextEntryUserInterface Parent;
public:
   AdminPasswordEntryUserInterface()
   { 
      title = "ENTER ADMIN PASSWORD:";
      secret = true;
   }
   void onAccept(const char *text);
   void onEscape();
   void render();
};

extern AdminPasswordEntryUserInterface gAdminPasswordEntryUserInterface;

};

#endif
