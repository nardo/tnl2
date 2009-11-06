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

#include "quickChat.h"
#include "UIGame.h"
#include "glutInclude.h"
#include "gameType.h"
#include "UIMenus.h"
#include "gameObjectRender.h"
#include "input.h"
#include <ctype.h>
namespace Zap
{
void renderControllerButton(F32 x, F32 y, U32 buttonIndex, U32 keyIndex);

VChatHelper::VChatNode VChatHelper::mChatTree[] =
{
   // Root node
   {0, ' ', -1, true, "", ""},
      {1, 'G', 5, true, "Global", ""},
         {2, 'Z', 5, false, "Doh", "Doh!"},
         {2, 'S', 4, false, "Shazbot", "Shazbot!"},
         {2, 'D', 3, false, "Damnit", "Dammit!"},
         {2, 'C', -1, false, "Crap", "Ah Crap!"},
         {2, 'E', 2, false, "Duh", "Duh."},
         {2, 'X', -1, false, "You idiot!", "You idiot!"},
         {2, 'T', 1, false, "Thanks", "Thanks."},
         {2, 'A', 0, false, "No Problem", "No Problemo."},
      {1, 'D', 4, true, "Defense", ""},
         {2, 'A', -1, true, "Attacked", "We are being attacked."},
         {2, 'E', 5, true, "Enemy Attacking Base", "The enemy is attacking our base."},
         {2, 'N', 4, true, "Need More Defense", "We need more defense."},
         {2, 'T', 3, true, "Base Taken", "Base is taken."},
         {2, 'C', 2, true, "Base Clear", "Base is secured."},
         {2, 'Q', 1, true, "Is Base Clear?", "Is our base clear?"},
         {2, 'D', 0, true, "Defending Base", "Defending our base."},
         {2, 'G', -1, true, "Defend Our Base", "Defend our base."},
      {1, 'F', 3, true, "Flag", ""},
         {2, 'G', 5, true, "Flag gone", "Our flag is not in the base!"},
         {2, 'E', 4, true, "Enemy has flag", "The enemy has our flag!"},
         {2, 'H', 3, true, "Have enemy flag", "I have the enemy flag."},
         {2, 'S', 2, true, "Flag secure", "Our flag is secure."},
         {2, 'R', 1, true, "Return our flag", "Return our flag to base."},
         {2, 'F', 0, true, "Get enemy flag", "Get the enemy flag."},
      {1, 'S', -1, true, "Incoming Enemies - Direction", ""},
         {2, 'V', -1, true, "Incoming Enemies",  "Incoming enemies!"},
         {2, 'W', -1, true, "Incoming North",    "*** INCOMING NORTH ***"},
         {2, 'D', -1, true, "Incoming West",     "*** INCOMING WEST  ***"},
         {2, 'A', -1, true, "Incoming East",     "*** INCOMING EAST  ***"},
         {2, 'S', -1, true, "Incoming South",    "*** INCOMING SOUTH ***"},
      {1, 'V', 2, true, "Quick", ""},
         {2, 'Z', 5, true, "Move out", "Move out."},
         {2, 'G', 4, true, "Going offense", "Going offense."},
         {2, 'E', 3, true, "Regroup", "Regroup."},
         {2, 'V', 2, true, "Help!", "Help!" },
         {2, 'W', 1, true, "Wait for signal", "Wait for my signal to attack."},
         {2, 'A', 0, true, "Attack!", "Attack!"},
         {2, 'O', -1, true, "Go on the offensive", "Go on the offensive."},
         {2, 'J', -1, true, "Capture the objective", "Capture the objective."},
      {1, 'R', 1, true, "Reponses", ""},
         {2, 'D', 5, true, "Dont know", "I don't know."},
         {2, 'T', 4, true, "Thanks", "Thanks."},
         {2, 'S', 3, true, "Sorry", "Sorry."},
         {2, 'Y', 2, true, "Yes", "Yes."},
         {2, 'N', 1, true, "No", "No."},
         {2, 'A', 0, true, "Acknowledge", "Acknowledged."},
      {1, 'T', 0, true, "Taunts", ""},
         {2, 'E', 5, false, "Yoohoo!", "Yoohoo!"},
         {2, 'Q', 4, false, "How'd THAT feel?", "How'd THAT feel?"},
         {2, 'W', 3, false, "I've had worse...", "I've had worse..."},
         {2, 'X', 2, false, "Missed me!", "Missed me!"},
         {2, 'D', 1, false, "Dance!", "Dance!"},
         {2, 'C', 0, false, "Come get some!", "Come get some!"},
         {2, 'R', -1, false, "Rawr", "RAWR!"},

   // Terminate
   {0, ' ', false, "", ""}
};

VChatHelper::VChatHelper()
{
   mCurNode = &mChatTree[0];
}

void VChatHelper::render()
{
   Vector<VChatNode *> renderNodes;

   VChatNode *walk = mCurNode;
   U32 matchLevel = walk->depth + 1;
   walk++;

   // First get to the end...
   while(walk->depth >= matchLevel)
      walk++;

   // Then draw bottom up...
   while(walk != mCurNode)
   {
      if(walk->depth == matchLevel && (!mFromController || (mFromController && walk->buttonIndex != -1)))
         renderNodes.push_back(walk);
      walk--;
   }

   S32 curPos = 300;
   const int fontSize = 15;

   for(S32 i = 0; i < renderNodes.size(); i++)
   {
      glColor3f(0.3, 1.0, 0.3);
      renderControllerButton(UserInterface::horizMargin, curPos, renderNodes[i]->buttonIndex, renderNodes[i]->trigger);

      glColor3f(0.3, 1.0, 0.3);
      UserInterface::drawStringf(UserInterface::horizMargin + 20, curPos, fontSize, "- %s", 
         renderNodes[i]->caption);
      curPos += fontSize + 4;
   }
}

void VChatHelper::show(bool fromController)
{
   mCurNode = &mChatTree[0];
   mFromController = OptionsMenuUserInterface::joystickType != -1;
   mIdleTimer.reset(MenuTimeout);
}

void VChatHelper::idle(U32 delta)
{
   if(mIdleTimer.update(delta))
      gGameUserInterface.setPlayMode();
}

void VChatHelper::processKey(U32 key)
{
   // Escape...
   if(key == 8 || key == 27)
   {
      UserInterface::playBoop();
      gGameUserInterface.setPlayMode();
      return;
   }

   // Try to find a match if we can...

   // work in upper case...
   key = toupper(key);

   // Set up walk...
   VChatNode *walk = mCurNode;
   U32 matchLevel = walk->depth+1;
   walk++;

   // Iterate over anything at our desired depth or lower...
   while(walk->depth >= matchLevel)
   {

      // If it has the same key and depth...
      bool match = (!mFromController && toupper(walk->trigger) == key) ||
                   (mFromController && key == walk->buttonIndex);
      if(match && walk->depth == matchLevel)
      {
         mIdleTimer.reset(MenuTimeout);
         // select it...
         mCurNode = walk;

         UserInterface::playBoop();

         // If we're at a leaf (ie, next child down is higher or equal to us), then issue the chat and call it good...
         walk++;
         if(mCurNode->depth >= walk->depth)
         {
            GameType *gt = gClientGame->getGameType();
            gGameUserInterface.setPlayMode();

            StringTableEntry entry(mCurNode->msg);
            if(gt)
               gt->c2sSendChatSTE(!mCurNode->teamOnly, entry);

            return;
         }
      }

      walk++;
   }

}


};
