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

#ifndef _POINT_H_
#define _POINT_H_
#include <math.h>
#include <stdlib.h>

namespace Zap
{

struct Point
{
   typedef float member_type;
   member_type x;
   member_type y;

   Point() { x = 0; y = 0; }
   Point(const Point& pt) { x = pt.x; y = pt.y; }
   Point(member_type in_x, member_type in_y) { x = in_x; y = in_y; }
   void set(member_type ix, member_type iy) { x = ix; y = iy; }
   void set(const Point &pt) { x = pt.x; y = pt.y; }

   Point operator+(const Point &pt) const { return Point (x + pt.x, y + pt.y); }
   Point operator-(const Point &pt) const { return Point (x - pt.x, y - pt.y); }
   Point operator-() const { return Point(-x, -y); }
   Point &operator+=(const Point &pt) { x += pt.x; y += pt.y; return *this; }
   Point &operator-=(const Point &pt) { x -= pt.x; y -= pt.y; return *this; }

   Point operator*(const member_type f) { return Point (x * f, y * f); }
   Point &operator*=(const member_type f) { x *= f; y *= f; return *this; }

   Point operator*(const Point &pt) { return Point(x * pt.x, y * pt.y); }

   Point &operator=(const Point &pt) { x = pt.x; y = pt.y; return *this; }
   bool operator==(const Point &pt) const { return x == pt.x && y == pt.y; }

   member_type len() const { return (member_type) sqrt(x * x + y * y); }
   member_type lenSquared() const { return x * x + y * y; }
   void normalize() { member_type l = len(); if(l == 0) { x = 1; y = 0; } else { l = 1 / l; x *= l; y *= l; } }
   void normalize(float newLen) { member_type l = len(); if(l == 0) { x = newLen; y = 0; } else { l = newLen / l; x *= l; y *= l; } }

   void scaleFloorDiv(float scaleFactor, float divFactor)
   {
      x = (member_type) floor(x * scaleFactor + 0.5) * divFactor;
      y = (member_type) floor(y * scaleFactor + 0.5) * divFactor;
   }
   member_type dot(const Point &p) const { return x * p.x + y * p.y; }
   void read(const char **argv) { x = (member_type) atof(argv[0]); y = (member_type) atof(argv[1]); }
};

struct Color
{
   float r, g, b;

   Color(const Color &c) { r = c.r; g = c.g; b = c.b; }
   Color(float red = 1, float green = 1, float blue = 1) { r = red; g = green; b = blue; }
   void read(const char **argv) { r = (float) atof(argv[0]); g = (float) atof(argv[1]); b = (float) atof(argv[2]); }

   void interp(float t, const Color &c1, const Color &c2)
   {
      float oneMinusT = 1.0f - t;
      r = c1.r * t + c2.r * oneMinusT;
      g = c1.g * t + c2.g * oneMinusT;
      b = c1.b * t + c2.b * oneMinusT;
   }
   void set(float _r, float _g, float _b) { r = _r; g = _g; b = _b; }
   void set(const Color &c) { r = c.r; g = c.g; b = c.b; }

   Color operator+(const Color &c) const { return Color (r + c.r, g + c.g, b + c.b); }
   Color operator-(const Color &c) const { return Color (r - c.r, g - c.g, b - c.b); }
   Color operator-() const { return Color(-r, -g, -b); }
   Color &operator+=(const Color &c) { r += c.r; g += c.g; b += c.b; return *this; }
   Color &operator-=(const Color &c) { r -= c.r; g -= c.g; b -= c.b; return *this; }

   Color operator*(const float f) { return Color (r * f, g * f, b * f); }
   Color &operator*=(const float f) { r *= f; g *= f; b *= f; return *this; }
};

struct Rect
{
   Point min, max;

   Rect() {}
   Rect(Point p1, Point p2) { set(p1, p2); }
   Point getCenter() { return (max + min) * 0.5; }
   void set(Point p1, Point p2)
   {
      if(p1.x < p2.x)
      {
         min.x = p1.x;
         max.x = p2.x;
      }
      else
      {
         min.x = p2.x;
         max.x = p1.x;
      }
      if(p1.y < p2.y)
      {
         min.y = p1.y;
         max.y = p2.y;
      }
      else
      {
         min.y = p2.y;
         max.y = p1.y;
      }
   }
   bool contains(const Point &p)
   {
      return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
   }

   void unionPoint(const Point &p)
   {
      if(p.x < min.x)
         min.x = p.x;
      else if(p.x > max.x)
         max.x = p.x;
      if(p.y < min.y)
         min.y = p.y;
      else if(p.y > max.y)
         max.y = p.y;
   }

   void unionRect(const Rect &r)
   {
      if(r.min.x < min.x)
         min.x = r.min.x;
      if(r.max.x > max.x)
         max.x = r.max.x;
      if(r.min.y < min.y)
         min.y = r.min.y;
      if(r.max.y > max.y)
         max.y = r.max.y;
   }

   bool intersects(const Rect &r)
   {
      return min.x < r.max.x && min.y < r.max.y &&
             max.x > r.min.x && max.y > r.min.y;
   }
   void expand(Point delta) { min -= delta; max += delta; }

   Point getExtents()
   {
      return max - min;
   }
};

};

#endif
