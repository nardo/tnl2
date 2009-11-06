// Example code for: Collision Detection with Swept Spheres and Ellipsoids
// See: http://www.three14.demon.nl/sweptellipsoid/SweptEllipsoid.pdf
//
// Copyright (C) 2003 Jorrit Rouwe
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// This is free software, you can do with it what you want.
//
// This file contains the main swept sphere / ellipsoid intersection tests.
//
// Please have a look at the notes. They indicate obvious places for optimization 
// if you are using a swept ellipsoid against a large number of polygons.

// this file has been modified by Mark Frohnmayer:
// removed swept ellipsoid routine
// removed swept sphere
// removed references to vector2 and replaced with Point
// added in prototypes for circle collisions

#ifndef _SWEPT_ELLIPSOID_H_
#define _SWEPT_ELLIPSOID_H_

namespace Zap {
//#include "Vector2.h"
//#include "Plane.h"

// Test between a polygon and a swept sphere with radius inRadius moving from inBegin to inBegin + inDelta
// If there is an intersection the intersection position is returned in outPoint and the center of the
// sphere is at inBegin + outFraction * inDelta when it collides
//bool PolygonSweptSphereIntersect(const Plane &inPlane, const Vector2 *inVertices, int inNumVertices, const Vector3 &inBegin, const Vector3 &inDelta, float inRadius, Vector3 &outPoint, float &outFraction);

// Test intersection with a swept ellipsoid with principal axis inAxis1, inAxis2, inAxis3 moving from inBegin to inBegin + inDelta
// If there is an intersection the intersection position is returned in outPoint and the center of the
// sphere is at inBegin + outFraction * inDelta when it collides
//bool PolygonSweptEllipsoidIntersect(const Plane &inPlane, const Vector2 *inVertices, int inNumVertices, const Vector3 &inBegin, const Vector3 &inDelta, const Vector3 &inAxis1, const Vector3 &inAxis2, const Vector3 &inAxis3, Vector3 &outPoint, float &outFraction);

bool PolygonSweptCircleIntersect(const Point *inVertices, int inNumVertices, const Point &inBegin, const Point &inDelta, Point::member_type inRadius, Point &outPoint, Point::member_type &outFraction);

};
#endif // _SWEPT_ELLIPSOID_H_
