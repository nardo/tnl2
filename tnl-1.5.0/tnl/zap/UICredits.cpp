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

#include "../tnl/tnlRandom.h"
#include "UICredits.h"
#include "glutInclude.h"
#include <stdio.h>

namespace Zap
{

const char *gGameCredits[] = {
   "Z A P",
   "(c) 2004 GarageGames.com",
   "Lead Programmer and Level Designer",
   "Mark Frohnmayer",
   "Linux Programmer",
   "John Quigley",
   "Programmer and Sound Effects Engineer",
   "Ben Garney",
   "Programmer",
   "Robert Blanchet",
   "Internal Testers",
   "Alex Swanson, Brian Ramage",
   "External Testers",
   "Clark Fagot, Joe Maruschak, #garagegames",
   NULL,
};

CreditsUserInterface gCreditsUserInterface;

void CreditsUserInterface::onActivate()
{
   // construct the creditsfx objects here, they will
   // get properly deleted when the CreditsUI 
   // destructor is envoked

   // add credits scroller first and make it active
   CreditsScroller *scroller = new CreditsScroller;
   scroller->setActive(true);

   // add CreditsFX effects below here, dont activate:
   // 

   // choose randomly another CreditsFX effect
   // aside from CreditsScroller to activate
   if(fxList.size() > 1)
   {
      U32 rand = Random::readI(0, fxList.size() - 1);
      while(fxList[rand]->isActive())
      {
         rand = Random::readI(0, fxList.size() - 1);
      }
      fxList[rand]->setActive(true);
   }
}

void CreditsUserInterface::render()
{
   // loop through all the attached effects and 
   // call their render function
   for(S32 i = 0; i < fxList.size(); i++)
      if(fxList[i]->isActive())
         fxList[i]->render();
}

void CreditsUserInterface::quit()
{
#ifdef TNL_OS_XBOX
   extern void xboxexit();
   xboxexit();
#else
   exit(0);
#endif
}

//-----------------------------------------------------
// CreditsFX Objects
//-----------------------------------------------------
CreditsFX::CreditsFX()
{
   activated = false;
   gCreditsUserInterface.addFX(this);
}

CreditsScroller::CreditsScroller()
{
   mProjectInfo.titleBuf = gGameCredits[0];
   mProjectInfo.copyBuf = gGameCredits[1];

   mProjectInfo.currPos.x = (UserInterface::canvasHeight - 100) / 2;
   
   // loop through each line in the credits expecting
   // the credits to be listed in this order:
   //    1) job
   //    2) name
   S32 pos = (UserInterface::canvasHeight + CreditSpace);
   S32 index = 2;
   while(gGameCredits[index] && gGameCredits[index+1])
   {
      CreditsInfo c;

      // get job
      c.jobBuf = gGameCredits[index];
      c.nameBuf = gGameCredits[index + 1];
      index += 2;
   
      // place credit in cache
      c.currPos.x = pos;
      credits.push_back(c);

      pos += CreditSpace;
   }
   mTotalSize = pos;
}

void CreditsScroller::updateFX(U32 delta)
{
   // draw the project information
   S32 pos = S32( mProjectInfo.currPos.x - (delta / 10) );
   if(pos < -CreditSpace)
      pos += mTotalSize;
   mProjectInfo.currPos.x = pos;

   // scroll the credits text from bottom to top
   //  based on time
   for(S32 i = 0; i < credits.size(); i++)
   {
      pos = S32( credits[i].currPos.x - (delta / 10) );

      // reached the top, reset
      if(pos < -CreditSpace)
         pos += mTotalSize;
 
      credits[i].currPos.x = pos;
   }
}

void CreditsScroller::render()
{
   glColor3f(1,1,1);
   
   // draw the project information
   //glLineWidth(10);
   UserInterface::drawCenteredString(S32 (mProjectInfo.currPos.x), 100, mProjectInfo.titleBuf);
   //glLineWidth(1);
   UserInterface::drawCenteredString(S32(mProjectInfo.currPos.x) + 120, 20, mProjectInfo.copyBuf);

   // draw the credits text
   for(S32 i = 0; i < credits.size(); i++)
   {
      UserInterface::drawCenteredString(S32(credits[i].currPos.x), 25, credits[i].jobBuf);
      UserInterface::drawCenteredString(S32(credits[i].currPos.x) + 50, 25, credits[i].nameBuf);
   }
}

};

