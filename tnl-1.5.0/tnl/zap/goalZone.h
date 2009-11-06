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

#ifndef _GOALZONE_H_
#define _GOALZONE_H_

#include "gameObject.h"

namespace Zap
{

class GoalZone : public GameObject
{
   typedef GameObject Parent;
   Vector<Point> mPolyBounds;
   enum {
      MaxPoints = 10,

      FlashDelay = 500,
      FlashCount = 5,

      InitialMask = BIT(0),
      TeamMask = BIT(1),
   };
   S32 mFlashCount;
   Timer mFlashTimer;
public:
   GoalZone();
   void render();

   bool isFlashing() { return mFlashCount & 1; }
   bool didRecentlyChangeTeam() { return mFlashCount != 0; }
   S32 getRenderSortValue();
   void processArguments(S32 argc, const char **argv);

   void setTeam(S32 team);
   void onAddedToGame(Game *theGame);
   void computeExtent();
   bool getCollisionPoly(Vector<Point> &polyPoints);
   bool collide(GameObject *hitObject);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);
   void idle(GameObject::IdleCallPath path);
   TNL_DECLARE_CLASS(GoalZone);
};

};

#endif


