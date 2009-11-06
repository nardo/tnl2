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

#ifndef _UICREDITS_H_
#define _UICREDITS_H_

#include "UI.h"
#include "game.h"

namespace Zap
{

class CreditsFX
{
protected:
   bool activated;

public:
   CreditsFX();
   void setActive(bool active) { activated = active; }
   bool isActive() { return activated; }
   virtual void updateFX(U32 delta) = 0;
   virtual void render() = 0;
};

class CreditsScroller : public CreditsFX
{
   //typedef CreditsFX Parent;
public:
   enum Credits {
      MaxCreditLen = 32,
      CreditSpace  = 200,
   };
private:
   struct ProjectInfo {
      const char *titleBuf;
      const char *copyBuf;
      Point currPos;
   } mProjectInfo;
   struct CreditsInfo {
      const char *nameBuf;
      const char *jobBuf;
      Point currPos;
   };
   Vector<CreditsInfo> credits;
   void readCredits(const char *file);
   S32 mTotalSize;

public:
   CreditsScroller();
   void updateFX(U32 delta);
   void render();
};

// Credits UI
class CreditsUserInterface : public UserInterface
{
   Vector<CreditsFX *> fxList;

public:
   ~CreditsUserInterface()
   {
      for(S32 i = 0; i < fxList.size(); i++)
      {
         delete fxList[i]; fxList[i] = NULL;
         fxList.erase(i);
      }
      fxList.clear();
   }
  
   void onActivate();
   void addFX(CreditsFX *fx) { fxList.push_back(fx); }
   void idle(U32 t)
   {
      for(S32 i = 0; i < fxList.size(); i++)
         if(fxList[i]->isActive())
            fxList[i]->updateFX(t);
   }
   void render();
   void quit();
   void onControllerButtonDown(U32 button) { quit(); }
   void onKeyDown(U32 key) { quit(); }   // exit for now
};
extern CreditsUserInterface gCreditsUserInterface;

}

#endif
