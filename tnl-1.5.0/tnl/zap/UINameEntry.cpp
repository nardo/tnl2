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

#include "UINameEntry.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "game.h"
#include "gameConnection.h"

#include "glutInclude.h"

namespace Zap
{

void TextEntryUserInterface::onActivate()
{
   if(resetOnActivate)
   {
      buffer[0] = 0;
      cursorPos = 0;
   }
}

void TextEntryUserInterface::render()
{
   glColor3f(1,1,1);

   U32 y = (canvasHeight / 2) - 20;

   drawCenteredString(y, 20, title);
   y += 45;

   char astbuffer[MaxTextLen+1];
   const char *renderBuffer=buffer;
   if(secret)
   {

      S32 i;
      for(i = 0; i < MaxTextLen;i++)
      {
         if(!buffer[i])
            break;
         astbuffer[i] = '*';
      } 
      astbuffer[i] = 0;
      renderBuffer = astbuffer;
   }
   drawCenteredString(y, 30, renderBuffer);

   U32 width = getStringWidth(30, renderBuffer);
   U32 x = (canvasWidth - width) / 2;

   if(blink)
      drawString(x + getStringWidth(30, renderBuffer, cursorPos), y, 30, "_");
}

void TextEntryUserInterface::idle(U32 t)
{
   if(mBlinkTimer.update(t))
   {
      mBlinkTimer.reset(BlinkTime);
      blink = !blink;
   }
}

void TextEntryUserInterface::onKeyDown(U32 key)
{
   if(key == '\r')
      onAccept(buffer);
   else if(key == 8 || key == 127)
   {
      // backspace key
      if(cursorPos > 0)
      {
         cursorPos--;
         for(U32 i = cursorPos; buffer[i]; i++)
            buffer[i] = buffer[i+1];
      }
   }
   else if(key == 27)
   {
      onEscape();
   }
   else
   {
      for(U32 i = MaxTextLen - 1; i > cursorPos; i--)
         buffer[i] = buffer[i-1];
      if(cursorPos < MaxTextLen-1)
      {
         buffer[cursorPos] = key;
         cursorPos++;
      }
   }
}

void TextEntryUserInterface::onKeyUp(U32 key)
{

}

void TextEntryUserInterface::setText(const char *text)
{
   if(strlen(text) > MaxTextLen)
   {
      strncpy(buffer, text, MaxTextLen);
      buffer[MaxTextLen] = 0;
   }
   else
      strcpy(buffer, text);
}


NameEntryUserInterface gNameEntryUserInterface;

void NameEntryUserInterface::onEscape()
{
}

void NameEntryUserInterface::onAccept(const char *text)
{
   gMainMenuUserInterface.activate();
}

PasswordEntryUserInterface gPasswordEntryUserInterface;

void PasswordEntryUserInterface::onEscape()
{
   gMainMenuUserInterface.activate();
}

void PasswordEntryUserInterface::onAccept(const char *text)
{
   joinGame(connectAddress, false, false);
}

AdminPasswordEntryUserInterface gAdminPasswordEntryUserInterface;

void AdminPasswordEntryUserInterface::render()
{
   gGameUserInterface.render();
   glColor4f(0, 0, 0, 0.5);
   glEnable(GL_BLEND);
   glBegin(GL_POLYGON);
   glVertex2f(0, 0);
   glVertex2f(canvasWidth, 0);
   glVertex2f(canvasWidth, canvasHeight);
   glVertex2f(0, canvasHeight);
   glEnd();  
   glDisable(GL_BLEND); 
   Parent::render();
}

void AdminPasswordEntryUserInterface::onEscape()
{
   gGameUserInterface.activate();
}

void AdminPasswordEntryUserInterface::onAccept(const char *text)
{
   gGameUserInterface.activate();
   GameConnection *gc = gClientGame->getConnectionToServer();
   if(gc)
      gc->c2sAdminPassword(text);
}



};

