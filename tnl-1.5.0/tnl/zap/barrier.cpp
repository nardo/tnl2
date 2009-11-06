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

#include "barrier.h"
#include "glutInclude.h"
#include "gameLoader.h"
#include "gameNetInterface.h"
#include <math.h>
#include "gameObjectRender.h"

using namespace TNL;

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(Barrier);

U32 Barrier::mBarrierChangeIndex = 1;

void constructBarrierPoints(const Vector<Point> &vec, F32 width, Vector<Point> &barrierEnds )
{
   bool loop = vec[0] == vec[vec.size() - 1];

   Vector<Point> edgeVector;
   for(S32 i = 0; i < vec.size() - 1; i++)
   {
      Point e = vec[i+1] - vec[i];
      e.normalize();
      edgeVector.push_back(e);
   }

   Point lastEdge = edgeVector[edgeVector.size() - 1];
   Vector<F32> extend;
   for(S32 i = 0; i < edgeVector.size(); i++)
   {
      Point curEdge = edgeVector[i];
      double cosTheta = curEdge.dot(lastEdge);

      if(cosTheta >= -0.01)
      {
         F32 extendAmt = width * 0.5 * tan( acos(cosTheta) / 2 );
         if(extendAmt > 0.01)
            extendAmt -= 0.01;
         extend.push_back(extendAmt);
      }
      else
         extend.push_back(0);
      lastEdge = curEdge;
   }
   F32 first = extend[0];
   extend.push_back(first);

   for(S32 i = 0; i < edgeVector.size(); i++)
   {
      F32 extendBack = extend[i];
      F32 extendForward = extend[i+1];
      if(i == 0 && !loop)
         extendBack = 0;
      if(i == edgeVector.size() - 1 && !loop)
         extendForward = 0;

      Point start = vec[i] - edgeVector[i] * extendBack;
      Point end = vec[i+1] + edgeVector[i] * extendForward;
      barrierEnds.push_back(start);
      barrierEnds.push_back(end);
   }
}

void constructBarriers(Game *theGame, const Vector<F32> &barrier, F32 width)
{
   Vector<Point> vec;

   for(S32 i = 1; i < barrier.size(); i += 2)
   {
      float x = barrier[i-1];
      float y = barrier[i];
      vec.push_back(Point(x,y));
   }
   if(vec.size() <= 1)
      return;

   Vector<Point> barrierEnds;
   constructBarrierPoints(vec, width, barrierEnds);
   for(S32 i = 0; i < barrierEnds.size(); i += 2)
   {
      Barrier *b = new Barrier(barrierEnds[i], barrierEnds[i+1], width);
      b->addToGame(theGame);
   }
}

Barrier::Barrier(Point st, Point e, F32 width)
{
   mObjectTypeMask = BarrierType | CommandMapVisType;
   start = st;
   end = e;
   Rect r(start, end);
   mWidth = width;
   r.expand(Point(width, width));
   setExtent(r);
   mLastBarrierChangeIndex = 0;
}

void Barrier::onAddedToGame(Game *theGame)
{
}

bool Barrier::getCollisionPoly(Vector<Point> &polyPoints)
{
   Point vec = end - start;
   Point crossVec(vec.y, -vec.x);
   crossVec.normalize(mWidth * 0.5);
   
   polyPoints.push_back(Point(start.x + crossVec.x, start.y + crossVec.y));
   polyPoints.push_back(Point(end.x + crossVec.x, end.y + crossVec.y));
   polyPoints.push_back(Point(end.x - crossVec.x, end.y - crossVec.y));
   polyPoints.push_back(Point(start.x - crossVec.x, start.y - crossVec.y));
   return true;
}

void Barrier::clipRenderLinesToPoly(Vector<Point> &polyPoints)
{
   //return;
   Vector<Point> clippedSegments;
   

   for(S32 i = 0; i < mRenderLineSegments.size(); i+= 2)
   {
      Point rp1 = mRenderLineSegments[i];
      Point rp2 = mRenderLineSegments[i + 1];

      Point cp1 = polyPoints[polyPoints.size() - 1];
      for(S32 j = 0; j < polyPoints.size(); j++)
      {
         Point cp2 = polyPoints[j];
         Point ce = cp2 - cp1;
         Point n(-ce.y, ce.x);

         n.normalize();
         F32 distToZero = n.dot(cp1);

         F32 d1 = n.dot(rp1);
         F32 d2 = n.dot(rp2);

         bool d1in = d1 >= distToZero;
         bool d2in = d2 >= distToZero;

         if(!d1in && !d2in) // both points are outside this edge of the poly, so...
         {
            // add them to the render poly
            clippedSegments.push_back(rp1);
            clippedSegments.push_back(rp2);
            break;
         }
         else if((d1in && !d2in) || (d2in && !d1in))
         {
            // find the clip intersection point:
            F32 t = (distToZero - d1) / (d2 - d1);
            Point clipPoint = rp1 + (rp2 - rp1) * t;

            if(d1in)
            {
               clippedSegments.push_back(clipPoint);
               clippedSegments.push_back(rp2);
               rp2 = clipPoint;
            }
            else
            {
               clippedSegments.push_back(rp1);
               clippedSegments.push_back(clipPoint);
               rp1 = clipPoint;
            }
         }

         // if both are in, just go to the next edge.
         cp1 = cp2;
      }
   }
   mRenderLineSegments = clippedSegments;
}

void Barrier::render(U32 layerIndex)
{
   Color b(0,0,0.15), f(0,0,1);
   //Color b(0.0,0.0,0.075), f(.3 ,0.3,0.8);

   if(layerIndex == 0)
   {
      Vector<Point> colPoly;
      getCollisionPoly(colPoly);
      glColor(b);
      glBegin(GL_POLYGON);
      for(S32 i = 0; i < colPoly.size(); i++)
         glVertex2f(colPoly[i].x, colPoly[i].y);
      glEnd();
   }
   else if(layerIndex == 1)
   {
      if(mLastBarrierChangeIndex != mBarrierChangeIndex)
      {
         mLastBarrierChangeIndex = mBarrierChangeIndex;
         mRenderLineSegments.clear();

         Vector<Point> colPoly;
         getCollisionPoly(colPoly);
         S32 last = colPoly.size() - 1;
         for(S32 i = 0; i < colPoly.size(); i++)
         {
            mRenderLineSegments.push_back(colPoly[last]);
            mRenderLineSegments.push_back(colPoly[i]);
            last = i;
         }
         static Vector<GameObject *> fillObjects;
         fillObjects.clear();

         Rect bounds(start, end);
         bounds.expand(Point(mWidth, mWidth));
         findObjects(BarrierType, fillObjects, bounds);

         for(S32 i = 0; i < fillObjects.size(); i++)
         {
            colPoly.clear();
            if(fillObjects[i] != this && fillObjects[i]->getCollisionPoly(colPoly))
               clipRenderLinesToPoly(colPoly);
         }
      }
      glColor(f);
      glBegin(GL_LINES);
      for(S32 i = 0; i < mRenderLineSegments.size(); i++)
         glVertex2f(mRenderLineSegments[i].x, mRenderLineSegments[i].y);
      glEnd();
   }
}

};

