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

#ifndef _UIVCHAT_H_
#define _UIVCHAT_H_

#include "tnlNetBase.h"
#include "UI.h"
#include "tnlNetStringTable.h"
#include "timer.h"

namespace Zap
{

class VChatHelper
{
private:
   struct VChatNode
   {
      U32 depth;
      U8  trigger;
      S32 buttonIndex;
      bool teamOnly;
      const char *caption;
      const char *msg;
   };

   static VChatNode mChatTree[];

   bool mFromController;
   VChatNode *mCurNode;
   Timer mIdleTimer;
   enum {
      MenuTimeout = 3500,
   };

public:
   VChatHelper();

   void idle(U32 delta);
   void render();
   void show(bool fromController);
   void processKey(U32 key);
};

};

#endif
