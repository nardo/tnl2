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

#ifndef _GRIDDB_H_
#define _GRIDDB_H_

#include "tnlTypes.h"
#include "point.h"
#include "tnlDataChunker.h"
#include "tnlVector.h"

using namespace TNL;

namespace Zap
{

class GameObject;

class GridDatabase 
{
public:
   enum {
      BucketWidth = 256, ///< Width/height of each bucket in pixels
      BucketRowCount = 16, ///< Number of buckets per grid row, and number of rows
      BucketMask = BucketRowCount - 1,
   };
   struct BucketEntry
   {
      GameObject *theObject;
      BucketEntry *nextInBucket;
   };
   U32 mQueryId;
   BucketEntry *mBuckets[BucketRowCount][BucketRowCount];
   ClassChunker<BucketEntry> mChunker;

   GridDatabase();
   
   GameObject *findObjectLOS(U32 typeMask, U32 stateIndex, Point rayStart, Point rayEnd, float &collisionTime, Point &surfaceNormal);
   void findObjects(U32 typeMask, Vector<GameObject *> &fillVector, Rect &extents);

   void addToExtents(GameObject *theObject, Rect &extents);
   void removeFromExtents(GameObject *theObject, Rect &extents);
};

};

#endif

