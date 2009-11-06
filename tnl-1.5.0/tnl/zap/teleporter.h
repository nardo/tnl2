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

#include "point.h"
#include "gameConnection.h"
#include "gameObject.h"
#include "../tnl/tnlNetObject.h"

namespace Zap
{

class Teleporter : public GameObject
{
public:
   Point pos;
   Point dest;
   bool doSplash;
   U32 timeout;
   U32 mTime;

   enum {
      InitMask     = BIT(0),
      TeleportMask = BIT(1),

      TeleporterRadius        = 75,
      TeleporterTriggerRadius = 50,
      TeleporterDelay = 1500,
      TeleporterExpandTime = 1350,
      TeleportInExpandTime = 750,
      TeleportInRadius = 120,
   };

   Teleporter(Point center = Point(), Point exit = Point());

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void idle(GameObject::IdleCallPath path);
   void render();

   void onAddedToGame(Game *theGame);

   void processArguments(S32 argc, const char **argv);

   TNL_DECLARE_CLASS(Teleporter);
};

};
