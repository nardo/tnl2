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

#include "UIInstructions.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "game.h"
#include "glutInclude.h"
#include "gameObjectRender.h"
#include "ship.h"
#include "teleporter.h"
#include "engineeredObjects.h"
#include "input.h"

namespace Zap
{

InstructionsUserInterface gInstructionsUserInterface;

void InstructionsUserInterface::onActivate()
{
   mCurPage = 1;
}

enum {
   NumPages = 5,
};

const char *pageHeaders[] = {
   "CONTROLS",
   "LOADOUT SELECTION",
   "WEAPON PROJECTILES",
   "GAME OBJECTS",
   "GAME OBJECTS",
};

void InstructionsUserInterface::render()
{
   if(gClientGame->isConnectedToServer())
   {
      gGameUserInterface.render();
      glEnable(GL_BLEND);
      glBegin(GL_POLYGON);
      glColor4f(0, 0, 0, 0.7);
      glVertex2f(0,0);
      glVertex2f(canvasWidth, 0);
      glVertex2f(canvasWidth, canvasHeight);
      glVertex2f(0, canvasHeight);
      glEnd();
      glDisable(GL_BLEND);
   }

   glColor3f(1,1,1);
   drawStringf(3, 3, 25, "INSTRUCTIONS - %s", pageHeaders[mCurPage - 1]);
   drawStringf(650, 3, 25, "PAGE %d/%d", mCurPage, NumPages);
   drawCenteredString(571, 20, "LEFT - previous page  RIGHT, SPACE - next page  ESC exits");
   glColor3f(0.7, 0.7, 0.7);
   glBegin(GL_LINES);
   glVertex2f(0, 31);
   glVertex2f(800, 31);
   glVertex2f(0, 569);
   glVertex2f(800, 569);
   glEnd();
   switch(mCurPage)
   {
      case 1:
         renderPage1();
         break;
      case 2:
         renderPage2();
         break;
      case 3:
         renderPageObjectDesc(0);
         break;
      case 4:
         renderPageObjectDesc(1);
         break;
      case 5:
         renderPageObjectDesc(2);
         break;
   }
}

struct ControlString
{
   const char *controlString;
   const char *primaryControl;
};

static ControlString gControlsKeyboard[] = {
   { "Move ship up (forward)", "W" },
   { "Move ship down (backward)", "S" },
   { "Move ship left (strafe left)", "A" },
   { "Move ship right (strafe right)", "D" },
   { "Aim ship", "MOUSE" },
   { "Fire weapon", "MOUSE BUTTON 1" },
   { "Activate primary module", "SPACE" },
   { "Activate secondary module", "SHIFT, MOUSE BUTTON 2" },
   { "Cycle current weapon", "E" },
   { "Open loadout selection menu", "Q" },
   { "Toggle map view", "C" },
   { "Chat to team", "T" },
   { "Chat to everyone", "G" },
   { "Open QuickChat menu", "V" },
   { "Record voice chat", "R" },
   { "Show scoreboard", "TAB" },
   { NULL, NULL },
};

static ControlString gControlsGamepad[] = {
   { "Move Ship", "Left Stick" },
   { "Aim Ship/Fire Weapon", "Right Stick" },
   { "Activate primary module", "Left Trigger" },
   { "Activate secondary module", "Right Trigger" },
   { "Cycle current weapon", "" },
   { "Toggle map view", "" },
   { "Open QuickChat menu", "" },
   { "Open loadout selection menu", "" },
   { "Show scoreboard", "" },
   { "Record voice chat", "" },
   { "Chat to team", "Keyboard T" },
   { "Chat to everyone", "Keyboard G" },
   { NULL, NULL },
};

extern void renderControllerButton(F32 x, F32 y, U32 buttonIndex, U32 keyIndex);

void InstructionsUserInterface::renderPage1()
{
   U32 y = 75;
   U32 col1 = 50;
   U32 col2 = 400;
   glBegin(GL_LINES);
   glVertex2f(col1, y + 26); 
   glVertex2f(750, y + 26);
   glEnd();
   glColor3f(1,1,1);
   ControlString *controls;
   bool gamepad = false;

   if(OptionsMenuUserInterface::joystickType == -1)
      controls = gControlsKeyboard;
   else
   {
      gamepad = true;
      controls = gControlsGamepad;
   }

   drawString(col1, y, 20, "Action");
   drawString(col2, y, 20, "Control");
   y += 28;
   for(S32 i = 0; controls[i].controlString; i++)
   {
      glColor3f(1,1,1);
      drawString(col1, y, 20, controls[i].controlString);
      if((OptionsMenuUserInterface::joystickType == LogitechDualAction || OptionsMenuUserInterface::joystickType == PS2DualShock) && (i == 2 || i == 3))
         renderControllerButton(col2, y + 4, i + 4, i + 4);
      else if(gamepad && !controls[i].primaryControl[0])
         renderControllerButton(col2, y + 4, i - 4, i - 4);
      else
         drawString(col2, y, 20, controls[i].primaryControl);
      y += 26;
   }
}

static const char *loadoutInstructions[] = {
   "Players can outfit their ships with 3 weapons and 2 modules.",
   "Pressing the loadout selection menu key brings up a menu that",
   "allows the player to choose the next loadout for his or her ship.",
   "This loadout will not be active on the ship until the player",
   "flies over a Loadout Zone object.",
   "",
   "The available modules and their functions are described below:",
   NULL,
};

static const char *moduleDescriptions[] = {
   "Boost - Boosts movement speed",
   "Shield - Reflects incoming projectiles",
   "Repair - Repairs self and nearby damaged objects",
   "Sensor - Increases visible distance and reveals hidden objects",
   "Cloak - Turns the ship invisible",
};

void InstructionsUserInterface::renderPage2()
{
   S32 y = 75;
   glColor3f(1,1,1);
   for(S32 i = 0; loadoutInstructions[i]; i++)
   {
      drawCenteredString(y, 20, loadoutInstructions[i]);
      y += 26;
   }

   y += 30;
   for(S32 i = 0; i < 5; i++)
   {
      glColor3f(1,1,1);
      drawString(105, y, 20, moduleDescriptions[i]);
      glPushMatrix();
      glTranslatef(60, y + 10, 0);
      glScalef(0.7, 0.7, 1);
      glRotatef(-90, 0, 0, 1);
      static F32 thrusts[4] =  { 1, 0, 0, 0 };
      static F32 thrustsBoost[4] =  { 1.3, 0, 0, 0 };

      switch(i)
      {
         case 0:
            renderShip(Color(0,0,1), 1, thrustsBoost, 1, Ship::CollisionRadius, false, false);
            glBegin(GL_LINES);
            glColor3f(1,1,0);
            glVertex2f(-20, -17);
            glColor3f(0,0,0);
            glVertex2f(-20, -50);
            glColor3f(1,1,0);
            glVertex2f(20, -17);
            glColor3f(0,0,0);
            glVertex2f(20, -50);
            glEnd();
            break;
         case 1:
            renderShip(Color(0,0,1), 1, thrusts, 1, Ship::CollisionRadius, false, true);
            break;
         case 2:
            {
               F32 health = (gClientGame->getCurrentTime() & 0x7FF) * 0.0005f;

               renderShip(Color(0,0,1), 1, thrusts, health, Ship::CollisionRadius, false, false);
               glLineWidth(3);
               glColor3f(1,0,0);
               drawCircle(Point(0,0), Ship::RepairDisplayRadius);
               glLineWidth(DefaultLineWidth);
            }
            break;
         case 3:
            {
               renderShip(Color(0,0,1), 1, thrusts, 1, Ship::CollisionRadius, false, false);
               F32 radius = (gClientGame->getCurrentTime() & 0x1FF) * 0.002;
               drawCircle(Point(), radius * Ship::CollisionRadius + 4);
            }
            break;
         case 4:
            {
               U32 ct = gClientGame->getCurrentTime();
               F32 frac = ct & 0x3FF;
               F32 alpha;
               if((ct & 0x400) != 0)
                  alpha = frac * 0.001;
               else
                  alpha = 1 - (frac * 0.001);
               renderShip(Color(0,0,1), alpha, thrusts, 1, Ship::CollisionRadius, false, false);
            }
            break;
      }

      glPopMatrix();
      y += 60;
   }
}

enum {
   GameObjectCount = 14,
};

const char *gGameObjectInfo[] = {
 "Phaser","The default weapon",
 "Bouncer","Bounces off walls",
 "Triple","Fires three diverging shots",
 "Burst","Explosive projectile",
 "Friendly Mine","Team's mines show trigger radius",
 "Opponent Mine","These are much harder to see",

 "RepairItem","Repairs damage done to a ship", 
 "Loadout Zone","Updates ship loadout",
 "Neutral Turret","Repair to take team ownership", 
 "Active Turret","Fires at enemy team", 
 "Neutral Emitter", "Repair to take team ownership",
 "Force Field Emitter","Allows only one team to pass",
 "Teleporter","Warps ship to another location",
 "Flag","Objective item in some game types",
};

void InstructionsUserInterface::renderPageObjectDesc(U32 index)
{
   //Point start(105, 75);
   //Point objStart(60, 85);

   U32 objectsPerPage = 6;
   U32 startIndex = index * objectsPerPage;
   U32 endIndex = startIndex + objectsPerPage;
   if(endIndex > GameObjectCount)
      endIndex = GameObjectCount;

   for(U32 i = startIndex; i < endIndex; i++)
   {
      glColor3f(1,1,1);
      const char *text = gGameObjectInfo[i * 2];
      const char *desc = gGameObjectInfo[i * 2 + 1];
      U32 index = i - startIndex;

      Point objStart((index & 1) * 400, (index >> 1) * 165);
      objStart += Point(200, 90);
      Point start = objStart + Point(0, 55);

      renderCenteredString(start, 20, text);
      renderCenteredString(start + Point(0, 25), 20, desc);
      //drawString(start.x, start.y, 20, text);

      glPushMatrix();
      glTranslatef(objStart.x, objStart.y, 0);
      glScalef(0.7, 0.7, 1);

      switch(i)
      {
         case 0:
            renderProjectile(Point(0,0), 0, gClientGame->getCurrentTime());
            break;
         case 1:
            renderProjectile(Point(0,0), 1, gClientGame->getCurrentTime());
            break;
         case 2:
            renderProjectile(Point(0,0), 2, gClientGame->getCurrentTime());
            break;
         case 3:
            renderGrenade(Point(0,0));
            break;
         case 4:
            renderMine(Point(0,0), true, true);
            break;
         case 5:
            renderMine(Point(0,0), true, false);
            break;

         case 6:
            renderRepairItem(Point(0, 0));
            break;
         case 7:
            {
               Vector<Point> p;
               p.push_back(Point(-150, -30));
               p.push_back(Point(150, -30));
               p.push_back(Point(150, 30));
               p.push_back(Point(-150, 30));
               Rect ext(p[0], p[2]);
               renderLoadoutZone(Color(0,0,1), p, ext);
            }
            break;
         case 8:
            renderTurret(Color(1,1,1), Point(0, 15), Point(0, -1), false, 0, 0, Turret::TurretAimOffset);
            break;
         case 9:
            renderTurret(Color(0,0,1), Point(0, 15), Point(0, -1), true, 1, 0, Turret::TurretAimOffset);
            break;

         case 10:
            renderForceFieldProjector(Point(-7.5, 0), Point(1, 0), Color(1,1,1), false);
            break;
         case 11:
            renderForceFieldProjector(Point(-50, 0), Point(1, 0), Color(1,0,0), true);
            renderForceField(Point(-35, 0), Point(50, 0), Color(1,0,0), true);
            break;
         case 12:
            renderTeleporter(Point(0, 0), 0, true, gClientGame->getCurrentTime(), 1, Teleporter::TeleporterRadius, 1);
            break;
         case 13:
            renderFlag(Point(0,0), Color(1, 0, 0));
            break;
      }
      glPopMatrix();
      objStart.y += 75;
      start.y += 75;
   }
}

void InstructionsUserInterface::nextPage()
{
   mCurPage++;
   if(mCurPage > NumPages)
      mCurPage = 1;
}

void InstructionsUserInterface::prevPage()
{
   if(mCurPage > 1)
      mCurPage--;
   else
      mCurPage = NumPages;
}

void InstructionsUserInterface::exitInstructions()
{
   if(gClientGame->isConnectedToServer())
      gGameUserInterface.activate();
   else
      gMainMenuUserInterface.activate();
}

void InstructionsUserInterface::onSpecialKeyDown(U32 key)
{
   switch(key)
   {
      case GLUT_KEY_LEFT:
         prevPage();
         break;
      case GLUT_KEY_RIGHT:
         nextPage();
         break;
      case GLUT_KEY_F1:
         exitInstructions();
         break;
   }
}

void InstructionsUserInterface::onKeyDown(U32 key)
{
   switch(key)
   {
      case ' ':
         nextPage();
         break;
      case '\r':
      case 27:
         exitInstructions();
         break;
   }
}

};