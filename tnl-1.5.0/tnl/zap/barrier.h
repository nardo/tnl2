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

/// The Barrier class represents rectangular barriers that player controlled
/// Ship instances cannot pass through.  Barrier objects, once created, never
/// change state, simplifying the pack/unpack update methods.  Barriers are
/// constructed as an expanded line segment.
class Barrier : public GameObject
{
public:
   Point start; ///< The start point of the barrier
   Point end; ///< The end point of the barrier 
   F32 mWidth;

   enum {
      BarrierWidth = 50, ///< The width, in game units of the barrier.
   };

   static U32 mBarrierChangeIndex; ///< Global counter that is incremented every time a new barrier is added on the client.
   U32 mLastBarrierChangeIndex; ///< Index to check against the global counter - if it is different, then this barrier's polygon outline will be clipped against all adjacent barriers.

   Vector<Point> mRenderLineSegments; ///< The clipped line segments representing this barrier.

   /// Barrier constructor.
   Barrier(Point st = Point(), Point e = Point(), F32 width = BarrierWidth);

   /// Adds the server object to the net interface's scope always list
   void onAddedToGame(Game *theGame);

   /// renders this barrier by drawing the render line segments,
   void render(U32 layer);

   /// returns a sorting key for the object.  Barriers should sort behind other objects
   S32 getRenderSortValue() { return -1; }

   /// returns the collision polygon of this barrier, which is the boundary extruded from the start,end line segment.
   bool getCollisionPoly(Vector<Point> &polyPoints);

   /// collide always returns true for Barrier objects.
   bool collide(GameObject *otherObject) { return true; }

   /// clips the current set of render lines against the polygon passed as polyPoints.
   void clipRenderLinesToPoly(Vector<Point> &polyPoints);

   TNL_DECLARE_CLASS(Barrier);
};

};

