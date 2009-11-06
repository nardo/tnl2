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

#include "UIMenus.h"
#include "UINameEntry.h"
#include "UIGame.h"
#include "UIQueryServers.h"
#include "UICredits.h"
#include "game.h"
#include "gameType.h"
#include "UIEditor.h"
#include "UIInstructions.h"
#include "input.h"

#include "glutInclude.h"

namespace Zap
{

MenuUserInterface::MenuUserInterface()
{
   menuTitle = "Menu";
   menuSubTitle = "";
   menuFooter = "UP, DOWN to choose  ENTER to select  ESC exits menu";
   selectionIndex = 0;
}

void MenuUserInterface::render()
{
   if(gClientGame->getConnectionToServer())
   {
      gGameUserInterface.render();
      glColor4f(0, 0, 0, 0.6);
      glEnable(GL_BLEND);
      glBegin(GL_POLYGON);
      glVertex2f(0, 0);
      glVertex2f(canvasWidth, 0);
      glVertex2f(canvasWidth, canvasHeight);
      glVertex2f(0, canvasHeight);
      glEnd();  
      glDisable(GL_BLEND); 
   }

   glColor3f(1,1,1);
   drawCenteredString( vertMargin, 30, menuTitle);
   drawCenteredString( vertMargin + 35, 18, menuSubTitle);
   drawCenteredString( canvasHeight - vertMargin - 20, 18, menuFooter);

   if(selectionIndex >= menuItems.size())
      selectionIndex = 0;

   S32 offset = 0;
   S32 count = menuItems.size();

   if(count > 7)
   {
      count = 7;
      offset = selectionIndex - 3;
      if(offset < 0)
         offset = 0;
      else if(offset + count >= menuItems.size())
         offset = menuItems.size() - count;
   }

   U32 yStart = (canvasHeight - count * 45) / 2;
   //glColor3f(0,1,0);

   for(S32 i = 0; i < count; i++)
   {
      U32 y = yStart + i * 45;

      if(selectionIndex == i + offset)
      {
         glColor3f(0,0,0.4);
         glBegin(GL_POLYGON);
         glVertex2f(0, y - 2);
         glVertex2f(800, y - 2);
         glVertex2f(800, y + 25 + 5);
         glVertex2f(0, y + 25 + 5);
         glEnd();
         glColor3f(0,0,1);
         glBegin(GL_LINES);
         glVertex2f(0, y - 2);
         glVertex2f(799, y - 2);
         glVertex2f(799, y + 25 + 5);
         glVertex2f(0, y + 25 + 5);
         glEnd();
      }      
      glColor3f(1,1,1);
      drawCenteredString(y, 25, menuItems[i+offset].mText);
   }
}

void MenuUserInterface::onSpecialKeyDown(U32 key)
{
   if(key == GLUT_KEY_UP)
   {
      selectionIndex--;
      if(selectionIndex < 0)
      {
         if(menuItems.size() > 7)
         {
            selectionIndex = 0;
            return;
         }
         else
            selectionIndex = menuItems.size() - 1;
      }
      UserInterface::playBoop();
   }
   else if(key == GLUT_KEY_DOWN)
   {
      selectionIndex++;
      if(selectionIndex >= menuItems.size())
      {
         if(menuItems.size() > 7)
         {
            selectionIndex = menuItems.size() - 1;
            return;
         }
         else
            selectionIndex = 0;
      }

      UserInterface::playBoop();
   }
}

void MenuUserInterface::onControllerButtonDown(U32 buttonIndex)
{
   if(buttonIndex == 0)
   {
      UserInterface::playBoop();
      processSelection(menuItems[selectionIndex].mIndex);
   }
   else if(buttonIndex == 1)
   {
      UserInterface::playBoop();
      onEscape();
   }
}

void MenuUserInterface::onKeyDown(U32 key)
{
   if(key == '\r')
   {
      UserInterface::playBoop();
      processSelection(menuItems[selectionIndex].mIndex);
   }
   else if(key == 27)
   {
      UserInterface::playBoop();
      onEscape();
   }
}

void MenuUserInterface::onEscape()
{

}

MainMenuUserInterface gMainMenuUserInterface;

MainMenuUserInterface::MainMenuUserInterface()
{
   dSprintf(titleBuffer, sizeof(titleBuffer), "%s:", ZAP_GAME_STRING);
   menuTitle = titleBuffer;
   motd[0] = 0;
   menuSubTitle = "A TORQUE NETWORK LIBRARY GAME - WWW.OPENTNL.ORG";
   menuFooter = "(C) 2004 GARAGEGAMES.COM, INC.";

   menuItems.push_back(MenuItem("JOIN LAN/INTERNET GAME", 0));
   menuItems.push_back(MenuItem("HOST GAME",1));
   menuItems.push_back(MenuItem("INSTRUCTIONS",2));
   menuItems.push_back(MenuItem("OPTIONS",3));
   menuItems.push_back(MenuItem("QUIT",4));
}

void MainMenuUserInterface::setMOTD(const char *motdString)
{
   strcpy(motd, motdString);
   motdArriveTime = gClientGame->getCurrentTime();
}

void MainMenuUserInterface::render()
{
   Parent::render();
   if(motd[0])
   {
      U32 width = getStringWidth(20, motd);
      glColor3f(1,1,1);
      U32 totalWidth = width + canvasWidth;
      U32 pixelsPerSec = 100;
      U32 delta = gClientGame->getCurrentTime() - motdArriveTime;
      delta = U32(delta * pixelsPerSec * 0.001) % totalWidth;
      
      drawString(canvasWidth - delta, 540, 20, motd);
   }
}

void MainMenuUserInterface::processSelection(U32 index)
{
   switch(index)
   {
      case 0:
         gQueryServersUserInterface.activate();
         break;
      case 1:
         hostGame(false, Address(IPProtocol, Address::Any, 28000));
         break;
      case 2:
         gInstructionsUserInterface.activate();
         break;
      case 3:
         gOptionsMenuUserInterface.activate();
         break;
      case 4:
         gCreditsUserInterface.activate();
         break;
   }
}

void MainMenuUserInterface::onEscape()
{
   gNameEntryUserInterface.activate();
}


OptionsMenuUserInterface gOptionsMenuUserInterface;

bool OptionsMenuUserInterface::controlsRelative = false;
bool OptionsMenuUserInterface::fullscreen = true;
S32 OptionsMenuUserInterface::joystickType = -1;
bool OptionsMenuUserInterface::echoVoice = false;

OptionsMenuUserInterface::OptionsMenuUserInterface()
{
   menuTitle = "OPTIONS MENU:";
}

void OptionsMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();
}

void OptionsMenuUserInterface::setupMenus()
{
   menuItems.clear();
   if(controlsRelative)
      menuItems.push_back(MenuItem("CONTROLS: RELATIVE",0));
   else
      menuItems.push_back(MenuItem("CONTROLS: ABSOLUTE",0));

   if(fullscreen)
      menuItems.push_back(MenuItem("VIDEO: FULLSCREEN",1));
   else
      menuItems.push_back(MenuItem("VIDEO: WINDOW",1));

   switch(joystickType)
   {
      case -1:
         menuItems.push_back(MenuItem("INPUT: KEYBOARD + MOUSE",2));
         break;
      case LogitechWingman:
         menuItems.push_back(MenuItem("INPUT: LOGITECH WINGMAN DUAL-ANALOG",2));
         break;
      case LogitechDualAction:
         menuItems.push_back(MenuItem("INPUT: LOGITECH DUAL ACTION",2));
         break;
      case SaitekDualAnalog:
         menuItems.push_back(MenuItem("INPUT: SAITEK P-880 DUAL-ANALOG",2));
         break;
      case PS2DualShock:
         menuItems.push_back(MenuItem("INPUT: PS2 DUALSHOCK USB",2));
         break;
      case XBoxController:
         menuItems.push_back(MenuItem("INPUT: XBOX CONTROLLER USB",2));
         break;
      case XBoxControllerOnXBox:
         menuItems.push_back(MenuItem("INPUT: XBOX CONTROLLER",2));
         break;
      default:
         menuItems.push_back(MenuItem("INPUT: UNKNOWN",2));
   }

   if(echoVoice)
      menuItems.push_back(MenuItem("VOICE ECHO: ENABLED",3));
   else
      menuItems.push_back(MenuItem("VOICE ECHO: DISABLED",3));
}

void OptionsMenuUserInterface::toggleFullscreen()
{
   if(fullscreen)
   {
      glutPositionWindow(100, 100);
      glutReshapeWindow(800, 600);
   }
   else
      glutFullScreen();
   fullscreen = !fullscreen;
}

void OptionsMenuUserInterface::processSelection(U32 index)
{
   switch(index)
   {
   case 0:
      controlsRelative = !controlsRelative;
      break;
   case 1:
      toggleFullscreen();
      break;
   case 2:
      joystickType++;
      if(joystickType > XBoxController)
         joystickType = -1;
      break;
   case 3:
      echoVoice = !echoVoice;
      break;
   };
   setupMenus();
}

void OptionsMenuUserInterface::onEscape()
{
   if(gClientGame->getConnectionToServer())
      gGameUserInterface.activate();   
   else
      gMainMenuUserInterface.activate();
}


GameMenuUserInterface gGameMenuUserInterface;

GameMenuUserInterface::GameMenuUserInterface()
{
   menuTitle = "GAME MENU:";
}

void GameMenuUserInterface::onActivate()
{
   Parent::onActivate();
   menuItems.clear();
   menuItems.push_back(MenuItem("OPTIONS",1));
   menuItems.push_back(MenuItem("INSTRUCTIONS",2));
   GameType *theGameType = gClientGame->getGameType();
   if(theGameType)
   {
      mGameType = theGameType;
      theGameType->addClientGameMenuOptions(menuItems);
   }
   GameConnection *gc = gClientGame->getConnectionToServer();
   if(gc)
   {
      if(gc->isAdmin())
         menuItems.push_back(MenuItem("ADMIN",4));
      else
         menuItems.push_back(MenuItem("ENTER ADMIN PASSWORD",5));
   }
   menuItems.push_back(MenuItem("LEAVE GAME",3));
}

void GameMenuUserInterface::processSelection(U32 index)
{
   switch(index)
   {
      case 1:
         gOptionsMenuUserInterface.activate();
         break;
      case 2:
         gInstructionsUserInterface.activate();
         break;
      case 3:
         endGame();
         if(EditorUserInterface::editorEnabled)
            gEditorUserInterface.activate();
         else
            gMainMenuUserInterface.activate();
         break;
      case 4:
         gAdminMenuUserInterface.activate();
         break;
      case 5:
         gAdminPasswordEntryUserInterface.activate();
         break;
      default:
         gGameUserInterface.activate();
         if(mGameType.isValid())
            mGameType->processClientGameMenuOption(index);
         break;
   }
}

void GameMenuUserInterface::onEscape()
{
   gGameUserInterface.activate();
}

LevelMenuUserInterface gLevelMenuUserInterface;

void LevelMenuUserInterface::onActivate()
{
   Parent::onActivate();
   mTypeSelectDone = false;
   menuTitle = "CHOOSE LEVEL TYPE:";

   GameConnection *gc = gClientGame->getConnectionToServer();
   if(!gc || !gc->mLevelTypes.size())
      return;
   
   menuItems.clear();
   menuItems.push_back(MenuItem(gc->mLevelTypes[0].getString(),0));

   for(S32 i = 1; i < gc->mLevelTypes.size();i++)
   {
      S32 j;
      for(j = 0;j < menuItems.size();j++)
         if(!stricmp(gc->mLevelTypes[i].getString(),menuItems[j].mText))
            break;
      if(j == menuItems.size())
      {
         menuItems.push_back(MenuItem(gc->mLevelTypes[i].getString(), i));
      }
   }
}

void LevelMenuUserInterface::processSelection(U32 index)
{
   Parent::onActivate();
   GameConnection *gc = gClientGame->getConnectionToServer();
   if(mTypeSelectDone)
   {
      // The selection index is the level to load.
      logprintf("load level %s", gc->mLevelNames[index].getString());
      gc->c2sRequestLevelChange(index);
      gGameUserInterface.activate();
   }
   else
   {
      mTypeSelectDone = true;
      StringTableEntry s = gc->mLevelTypes[index];
      menuItems.clear();
      for(S32 i = 0; i < gc->mLevelTypes.size();i++)
      {
         if(gc->mLevelTypes[i] == s)
            menuItems.push_back(MenuItem(gc->mLevelNames[i].getString(), i));
      }
   }
}

void LevelMenuUserInterface::onEscape()
{
   gGameUserInterface.activate();
}

AdminMenuUserInterface gAdminMenuUserInterface;

void AdminMenuUserInterface::onActivate()
{
   menuTitle = "ADMINISTRATOR OPTIONS:";
   menuItems.clear();
   menuItems.push_back(MenuItem("CHANGE LEVEL",0));
   menuItems.push_back(MenuItem("CHANGE A PLAYER'S TEAM",1));
   menuItems.push_back(MenuItem("KICK A PLAYER",2));
}

void AdminMenuUserInterface::onEscape()
{
   gGameUserInterface.activate();
}

void AdminMenuUserInterface::processSelection(U32 index)
{
   switch(index)
   {
   case 0:
      gLevelMenuUserInterface.activate();
      break;
   case 1:
      gPlayerMenuUserInterface.action = PlayerMenuUserInterface::ChangeTeam;
      gPlayerMenuUserInterface.activate();
      break;
   case 2:
      gPlayerMenuUserInterface.action = PlayerMenuUserInterface::Kick;
      gPlayerMenuUserInterface.activate();
      break;
   }
}

PlayerMenuUserInterface gPlayerMenuUserInterface;

void PlayerMenuUserInterface::render()
{
   menuItems.clear();
   GameType *gt = gClientGame->getGameType();
   if(gt)
   {
      for(S32 i = 0; i < gt->mClientList.size(); i++)
         menuItems.push_back(MenuItem(gt->mClientList[i]->name.getString(), i));
   }
   if(action == Kick)
      menuTitle = "CHOOSE PLAYER TO KICK:";
   else if(action == ChangeTeam)
      menuTitle = "CHOOSE PLAYER WHOSE TEAM TO CHANGE:";
   Parent::render();
}

void PlayerMenuUserInterface::onEscape()
{
   gGameUserInterface.activate();
}

void PlayerMenuUserInterface::processSelection(U32 index)
{
   StringTableEntry e(menuItems[index].mText);
   GameConnection *gc = gClientGame->getConnectionToServer();
   if(gc)
      gc->c2sAdminPlayerAction(e, action);
   gGameUserInterface.activate();
}

};