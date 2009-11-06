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

#include "gameObject.h"
#include "gameType.h"
#include "glutInclude.h"
#include "gameNetInterface.h"
#include "UI.h"
#include "gameObjectRender.h"

namespace Zap
{

class LoadoutZone : public GameObject
{
   typedef GameObject Parent;
   Vector<Point> mPolyBounds;
   enum {
      MaxPoints = 10,
   };

public:
   LoadoutZone()
   {
      mTeam = 0;
      mNetFlags.set(Ghostable);
      mObjectTypeMask = LoadoutZoneType | CommandMapVisType;
   }

   void render()
   {
      renderLoadoutZone(getGame()->getGameType()->getTeamColor(getTeam()), mPolyBounds, getExtent());
   }

   S32 getRenderSortValue()
   {
      return -1;
   }

   void processArguments(S32 argc, const char **argv)
   {
      if(argc < 7)
         return;

      mTeam = atoi(argv[0]);
      for(S32 i = 1; i < argc; i += 2)
      {
         Point p;
         p.x = atof(argv[i]) * getGame()->getGridSize();
         p.y = atof(argv[i+1]) * getGame()->getGridSize();
         mPolyBounds.push_back(p);
      }
      computeExtent();
   }

   void onAddedToGame(Game *theGame)
   {
      if(!isGhost())      
         setScopeAlways();
   }

   void computeExtent()
   {
      Rect extent(mPolyBounds[0], mPolyBounds[0]);
      for(S32 i = 1; i < mPolyBounds.size(); i++)
         extent.unionPoint(mPolyBounds[i]);
      setExtent(extent);
   }

   bool getCollisionPoly(Vector<Point> &polyPoints)
   {
      for(S32 i = 0; i < mPolyBounds.size(); i++)
         polyPoints.push_back(mPolyBounds[i]);
      return true;
   }

   bool collide(GameObject *hitObject)
   {
      if(!isGhost() && (hitObject->getTeam() == getTeam()) && (hitObject->getObjectTypeMask() & ShipType))
         getGame()->getGameType()->updateShipLoadout(hitObject);

      return false;
   }

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
   {
      stream->write(mTeam);
      stream->writeEnum(mPolyBounds.size(), MaxPoints);
      for(S32 i = 0; i < mPolyBounds.size(); i++)
      {
         stream->write(mPolyBounds[i].x);
         stream->write(mPolyBounds[i].y);
      }
      return 0;
   }

   void unpackUpdate(GhostConnection *connection, BitStream *stream)
   {
      stream->read(&mTeam);
      S32 size = stream->readEnum(MaxPoints);
      for(S32 i = 0; i < size; i++)
      {
         Point p;
         stream->read(&p.x);
         stream->read(&p.y);
         mPolyBounds.push_back(p);
      }
      if(size)
         computeExtent();
   }

   TNL_DECLARE_CLASS(LoadoutZone);
};

TNL_IMPLEMENT_NETOBJECT(LoadoutZone);

};