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

#include "gameObjectRender.h"
#include "glutInclude.h"
#include "tnlRandom.h"
#include "UI.h"
#include "projectile.h"

namespace Zap
{

#ifdef TNL_OS_XBOX
const float gShapeLineWidth = 2.0f;
#else
const float gShapeLineWidth = 2.0f;
#endif

void glVertex(Point p)
{
   glVertex2f(p.x, p.y);
}

void glColor(Color c, float alpha)
{
   glColor4f(c.r, c.g, c.b, alpha);
}

void drawCircle(Point pos, F32 radius)
{
   glBegin(GL_LINE_LOOP);

   for(F32 theta = 0; theta < 2 * 3.1415; theta += 0.2)
      glVertex2f(pos.x + cos(theta) * radius, pos.y + sin(theta) * radius);

   glEnd();
}

void fillCircle(Point pos, F32 radius)
{
   glBegin(GL_POLYGON);

   for(F32 theta = 0; theta < 2 * 3.1415; theta += 0.2)
      glVertex2f(pos.x + cos(theta) * radius, pos.y + sin(theta) * radius);

   glEnd();
}

void renderShip(Color c, F32 alpha, F32 thrusts[], F32 health, F32 radius, bool cloakActive, bool shieldActive)
{
   if(alpha != 1.0)
      glEnable(GL_BLEND);
   
   if(cloakActive)
   {
      glColor4f(0,0,0, 1 - alpha);
      glBegin(GL_POLYGON);
      glVertex2f(-20, -15);
      glVertex2f(0, 25);
      glVertex2f(20, -15);
      glEnd();
   }

   // first render the thrusters

   if(thrusts[0] > 0) // forward thrust:
   {
      glColor4f(1, 0, 0, alpha);
      glBegin(GL_LINES);
      glVertex2f(-8, -15);
      glVertex2f(0, -15 - 20 * thrusts[0]);
      glVertex2f(0, -15 - 20 * thrusts[0]);
      glVertex2f(8, -15);
      glEnd();
      glColor4f(1, 0.5, 0, alpha);
      glBegin(GL_LINES);
      glVertex2f(-6, -15);
      glVertex2f(0, -15 - 15 * thrusts[0]);
      glVertex2f(0, -15 - 15 * thrusts[0]);
      glVertex2f(6, -15);
      glEnd();
      glColor4f(1, 1, 0, alpha);
      glBegin(GL_LINES);
      glVertex2f(-4, -15);
      glVertex2f(0, -15 - 8 * thrusts[0]);
      glVertex2f(0, -15 - 8 * thrusts[0]);
      glVertex2f(4, -15);
      glEnd();
   }
   if(thrusts[1] > 0) // back thrust
   {
      // two jets:
      // left and right side:
      // from 7.5, 10 -> 12.5, 10 and from -7.5, 10 to -12.5, 10
      glColor4f(1, 0.5, 0, alpha);
      glBegin(GL_LINES);
      glVertex2f(7.5, 10);
      glVertex2f(10, 10 + thrusts[1] * 15);
      glVertex2f(12.5, 10);
      glVertex2f(10, 10 + thrusts[1] * 15);
      glVertex2f(-7.5, 10);
      glVertex2f(-10, 10 + thrusts[1] * 15);
      glVertex2f(-12.5, 10);
      glVertex2f(-10, 10 + thrusts[1] * 15);
      glEnd();
      glColor4f(1,1,0, alpha);
      glBegin(GL_LINES);
      glVertex2f(9, 10);
      glVertex2f(10, 10 + thrusts[1] * 10);
      glVertex2f(11, 10);
      glVertex2f(10, 10 + thrusts[1] * 10);
      glVertex2f(-9, 10);
      glVertex2f(-10, 10 + thrusts[1] * 10);
      glVertex2f(-11, 10);
      glVertex2f(-10, 10 + thrusts[1] * 10);
      glEnd();

   }
   float xThrust = -12.5;
   if(thrusts[3] > 0)
   {
      xThrust = -xThrust;
      thrusts[2] = thrusts[3];
   }
   if(thrusts[2] > 0)
   {
      glColor4f(1, 0, 0, alpha);
      glBegin(GL_LINES);
      glVertex2f(xThrust, 10);
      glVertex2f(xThrust + thrusts[2] * xThrust * 1.5, 5);
      glVertex2f(xThrust, 0);
      glVertex2f(xThrust + thrusts[2] * xThrust * 1.5, 5);
      glEnd();
      glColor4f(1,0.5,0, alpha);
      glBegin(GL_LINES);
      glVertex2f(xThrust, 8);
      glVertex2f(xThrust + thrusts[2] * xThrust, 5);
      glVertex2f(xThrust, 2);
      glVertex2f(xThrust + thrusts[2] * xThrust, 5);
      glEnd();
      glColor4f(1,1,0, alpha);
      glBegin(GL_LINES);
      glVertex2f(xThrust, 6);
      glVertex2f(xThrust + thrusts[2] * xThrust * 0.5, 5);
      glVertex2f(xThrust, 4);
      glVertex2f(xThrust + thrusts[2] * xThrust * 0.5, 5);
      glEnd();
   }

   // then render the ship:
   glColor4f(0.5,0.5,0.5, alpha);
   glBegin(GL_LINES);
   glVertex2f(-12.5, 0);
   glVertex2f(-12.5, 10);
   glVertex2f(-12.5, 10);
   glVertex2f(-7.5, 10);
   glVertex2f(7.5, 10);
   glVertex2f(12.5, 10);
   glVertex2f(12.5, 10);
   glVertex2f(12.5, 0);
   glEnd();
   
   glColor4f(c.r,c.g,c.b, alpha);
   glBegin(GL_LINE_LOOP);
   glVertex2f(-12, -13);
   glVertex2f(0, 22);
   glVertex2f(12, -13);
   glEnd();

   U32 lineCount = U32(14 * health);
   glBegin(GL_LINES);
   for(U32 i = 0; i < lineCount; i++)
   {
      S32 yo = i * 2;
      glVertex2f(-2, -11 + yo);
      glVertex2f(2, -11 + yo);
   }
   glEnd();

   glColor4f(0.7,0.7,0.7, alpha);
   glBegin(GL_LINE_LOOP);
   glVertex2f(-20, -15);
   glVertex2f(0, 25);
   glVertex2f(20, -15);
   glEnd();

   // Render shield if appropriate
   if(shieldActive)
   {
      F32 shieldRadius = radius + 3;

      glColor4f(1,1,0, alpha);
      glBegin(GL_LINE_LOOP);
      for(F32 theta = 0; theta <= 2 * 3.1415; theta += 0.3)
         glVertex2f(cos(theta) * shieldRadius, sin(theta) * shieldRadius);
      
      glEnd();
   }


   if(alpha != 1.0)
      glDisable(GL_BLEND);
}

void renderTeleporter(Point pos, U32 type, bool in, S32 time, F32 radiusFraction, F32 radius, F32 alpha)
{
   enum {
      NumColors = 6,
      NumTypes = 2,
      NumParticles = 100,
   };

   static bool trackerInit = false;

   struct Tracker
   {
      F32 thetaI;
      F32 thetaP;
      F32 dI;
      F32 dP;
      U32 ci;
   };
   static Tracker particles[NumParticles];

   static float colors[NumTypes][NumColors][3] = {
      {
         { 0, 0.25, 0.8 },
         { 0, 0.5, 1 },
         { 0, 0, 1 },
         { 0, 1, 1 },
         { 0, 0.5, 0.5 },
         { 0, 0, 1 },
      },
      {
         { 1, 0, 0.5 },
         { 1, 0, 1 },
         { 0, 0, 1 },
         { 0.5, 0, 1 },
         { 0, 0, 0.5 },
         { 1, 0, 0 },
      }
   };
   if(!trackerInit)
   {
      trackerInit = true;
      for(S32 i = 0; i < NumParticles; i++)
      {
         Tracker &t = particles[i];
         t.thetaI = Random::readF() * Float2Pi;
         t.thetaP = Random::readF() * 2 + 0.5;
         t.dP = Random::readF() * 5 + 2.5;
         t.dI = Random::readF() * t.dP;
         t.ci = Random::readI(0, NumColors - 1);
      }
   }

   glPushMatrix();
   glTranslatef(pos.x, pos.y, 0);

   glEnable(GL_BLEND);
   F32 arcTime = 0.5 + (1 - radiusFraction) * 0.5;
   if(!in)
      arcTime = -arcTime;

   Color tpColors[NumColors];
   Color white(1,1,1);
   for(S32 i = 0; i < NumColors; i++)
   {
      Color c(colors[type][i][0], colors[type][i][1], colors[type][i][2]);
      tpColors[i].interp(radiusFraction, c, white);
   }
   F32 beamWidth = 4;

   for(S32 i = 0; i < NumParticles; i++)
   {
      Tracker &t = particles[i];
      //glColor3f(t.c.r, t.c.g, t.c.b);
      F32 d = (t.dP - fmod(float(t.dI + time * 0.001), (float) t.dP)) / t.dP;
      F32 alphaMod = 1;
      if(d > 0.9)
         alphaMod = (1 - d) * 10;

      F32 theta = fmod(float( t.thetaI + time * 0.001 * t.thetaP), (float) Float2Pi);
      F32 startRadius = radiusFraction * radius * d;

      Point start(cos(theta), sin(theta));
      Point n(-start.y, start.x);
 
      theta -= arcTime * t.thetaP * (alphaMod + 0.05);
      d += arcTime / t.dP;
      if(d < 0)
         d = 0;
      Point end(cos(theta), sin(theta));

      F32 endRadius = radiusFraction * radius * d;

      glBegin(GL_TRIANGLE_STRIP);
      glColor(tpColors[t.ci], alpha * alphaMod);

      F32 arcLength = (end * endRadius - start * startRadius).len();
      U32 vertexCount = floor(arcLength / 10) + 2;

      glVertex(start * (startRadius + beamWidth * 0.3) + n * 2);
      glVertex(start * (startRadius - beamWidth * 0.3) + n * 2);
      for(U32 j = 0; j <= vertexCount; j++)
      {
         F32 frac = j / F32(vertexCount);
         F32 width = beamWidth * (1 - frac) * 0.5;
         Point p = start * (1 - frac) + end * frac;
         p.normalize();
         F32 rad = startRadius * (1 - frac) + endRadius * frac;

         p.normalize();
         glColor(tpColors[t.ci], alpha * alphaMod * (1 - frac));
         glVertex(p * (rad + width));
         glVertex(p * (rad - width));
      }
      glEnd();
   }
   glDisable(GL_BLEND);
   glPopMatrix();
}

void renderTurret(Color c, Point anchor, Point normal, bool enabled, F32 health, F32 barrelAngle, F32 aimOffset)
{
   glColor(c);

   Point cross(normal.y, -normal.x);
   Point aimCenter = anchor + normal * aimOffset;

   glBegin(GL_LINE_STRIP);

   for(S32 x = -10; x <= 10; x++)
   {
      F32 theta = x * FloatHalfPi * 0.1;
      Point pos = normal * cos(theta) + cross * sin(theta);
      glVertex(aimCenter + pos * 15);
   }
   glEnd();

   glLineWidth(3);
   glBegin(GL_LINES);
   Point aimDelta(cos(barrelAngle), sin(barrelAngle));
   glVertex(aimCenter + aimDelta * 15);
   glVertex(aimCenter + aimDelta * 30);
   glEnd();
   glLineWidth(DefaultLineWidth);

   if(enabled)
      glColor3f(1,1,1);
   else
      glColor3f(0.6, 0.6, 0.6);
   glBegin(GL_LINE_LOOP);
   glVertex(anchor + cross * 18);
   glVertex(anchor + cross * 18 + normal * aimOffset);
   glVertex(anchor - cross * 18 + normal * aimOffset);
   glVertex(anchor - cross * 18);
   glEnd();

   glColor(c);
   S32 lineHeight = U32(28 * health);
   glBegin(GL_LINES);
   for(S32 i = 0; i < lineHeight; i += 2)
   {
      Point lsegStart = anchor - cross * (14 - i) + normal * 5;
      Point lsegEnd = lsegStart + normal * (aimOffset - 10);
      glVertex(lsegStart);
      glVertex(lsegEnd);
   }
   Point lsegStart = anchor - cross * 14 + normal * 3;
   Point lsegEnd = anchor + cross * 14 + normal * 3;
   Point n = normal * (aimOffset - 6);
   glVertex(lsegStart);
   glVertex(lsegEnd);
   glVertex(lsegStart + n);
   glVertex(lsegEnd + n);
   glEnd();
}

void renderFlag(Point pos, Color c)
{
   glPushMatrix();
   glTranslatef(pos.x, pos.y, 0);

   glColor3f(c.r, c.g, c.b);
   glBegin(GL_LINES);
   glVertex2f(-15, -15);
   glVertex2f(15, -5);

   glVertex2f(15, -5);
   glVertex2f(-15, 5);

   glVertex2f(-15, -10);
   glVertex2f(10, -5);

   glVertex2f(10, -5);
   glVertex2f(-15, 0);
   glColor3f(1,1,1);
   glVertex2f(-15, -15);
   glVertex2f(-15, 15);
   glEnd();

   glPopMatrix();
}

void renderCenteredString(Point pos, U32 size, const char *string)
{
   F32 width = UserInterface::getStringWidth(size, string);
   UserInterface::drawString(pos.x - width * 0.5, pos.y - size * 0.5, size, string);
}

void renderLoadoutZone(Color theColor, Vector<Point> &bounds, Rect extent)
{
   F32 alpha = 0.5;
   glColor(theColor * 0.5);
   glBegin(GL_POLYGON);
   for(S32 i = 0; i < bounds.size(); i++)
      glVertex2f(bounds[i].x, bounds[i].y);
   glEnd();
   glColor(theColor * 0.7);
   glBegin(GL_LINE_LOOP);
   for(S32 i = 0; i < bounds.size(); i++)
      glVertex2f(bounds[i].x, bounds[i].y);
   glEnd();

   Point extents = extent.getExtents();
   Point center = extent.getCenter();

   glPushMatrix();
   glTranslatef(center.x, center.y, 0);
   if(extents.x < extents.y)
      glRotatef(90, 0, 0, 1);
   glColor(theColor);
   renderCenteredString(Point(0,0), 25, "LOADOUT ZONE");
   glPopMatrix();
}

void renderProjectile(Point pos, U32 type, U32 time)
{                     
   ProjectileInfo *pi = gProjInfo + type;

   glColor(pi->projColors[0]);
   glPushMatrix();
   glTranslatef(pos.x, pos.y, 0);
   glScalef(pi->scaleFactor, pi->scaleFactor, 1);

   glPushMatrix();
   glRotatef((time % 720) * 0.5, 0, 0, 1);

   glBegin(GL_LINE_LOOP);
   glVertex2f(-2, 2);
   glVertex2f(0, 6);
   glVertex2f(2, 2);
   glVertex2f(6, 0);
   glVertex2f(2, -2);
   glVertex2f(0, -6);
   glVertex2f(-2, -2);
   glVertex2f(-6, 0);
   glEnd();

   glPopMatrix();

   glRotatef(180 - (time % 360), 0, 0, 1);
   glColor(pi->projColors[1]);
   glBegin(GL_LINE_LOOP);
   glVertex2f(-2, 2);
   glVertex2f(0, 8);
   glVertex2f(2, 2);
   glVertex2f(8, 0);
   glVertex2f(2, -2);
   glVertex2f(0, -8);
   glVertex2f(-2, -2);
   glVertex2f(-8, 0);
   glEnd();

   glPopMatrix();
}

void renderMine(Point pos, bool armed, bool visible)
{
   F32 mod = 0.3;
   if(visible)
   {
      glColor3f(0.5,0.5,0.5);
      drawCircle(pos, Mine::SensorRadius);
      mod = 1.0;
   }
   else
      glLineWidth(1);

   glColor3f(mod,mod,mod);
   drawCircle(pos, 10);

   if(armed)
   {
      glColor3f(mod,0,0);
      drawCircle(pos, 6);
   }
   glLineWidth(DefaultLineWidth);
}

void renderGrenade(Point pos)
{
   glColor3f(1,1,1);
   drawCircle(pos, 10);

   glColor3f(1,0,0);
   drawCircle(pos, 6);
}


void renderRepairItem(Point pos)
{
   glPushMatrix();
   glTranslatef(pos.x, pos.y, 0);

   glColor3f(1,1,1);
   glBegin(GL_LINE_LOOP);
   glVertex2f(-18, -18);
   glVertex2f(18, -18);
   glVertex2f(18, 18);
   glVertex2f(-18, 18);
   glEnd();

   glColor3f(1,0,0);
   glBegin(GL_LINE_LOOP);

   float crossWidth = 4;
   float crossLen = 14;

   glVertex2f(crossWidth, crossWidth);
   glVertex2f(crossLen, crossWidth);
   glVertex2f(crossLen, -crossWidth);
   glVertex2f(crossWidth, -crossWidth);
   glVertex2f(crossWidth, -crossLen);
   glVertex2f(-crossWidth, -crossLen);
   glVertex2f(-crossWidth, -crossWidth);
   glVertex2f(-crossLen, -crossWidth);
   glVertex2f(-crossLen, crossWidth);
   glVertex2f(-crossWidth, crossWidth);
   glVertex2f(-crossWidth, crossLen);
   glVertex2f(crossWidth, crossLen);
   glEnd();

   glPopMatrix();
}

void renderForceFieldProjector(Point pos, Point normal, Color c, bool enabled)
{
   Point cross(normal.y, -normal.x);
   if(c.r < 0.7)
      c.r = 0.7;
   if(c.g < 0.7)
      c.g = 0.7;
   if(c.b < 0.7)
      c.b = 0.7;
   if(enabled)
      glColor(c);
   else
      glColor(c * 0.6);

   glBegin(GL_LINE_LOOP);
   glVertex(pos + cross * 12);
   glVertex(pos + normal * 15);
   glVertex(pos - cross * 12);
   glEnd();
}

void renderForceField(Point start, Point end, Color c, bool fieldUp)
{
   if(c.r < 0.5)
      c.r = 0.5;
   if(c.g < 0.5)
      c.g = 0.5;
   if(c.b < 0.5)
      c.b = 0.5;

   Point normal(end.y - start.y, start.x - end.x);
   normal.normalize(2.5);

   if(fieldUp)
      glColor(c);
   else
      glColor(c * 0.5);
   glBegin(GL_LINE_LOOP);
   glVertex(start + normal);
   glVertex(end + normal);
   glVertex(end - normal);
   glVertex(start - normal);
   glEnd();
}

void renderGoalZone(Vector<Point> &bounds, Color c, bool isFlashing)
{
   F32 alpha = 0.5;
   glColor(c * 0.5f);
   glBegin(GL_POLYGON);
   for(S32 i = 0; i < bounds.size(); i++)
      glVertex(bounds[i]);
   glEnd();
   if(isFlashing)
      glColor(c);
   else
      glColor(c * 0.7);

   glBegin(GL_LINE_LOOP);
   for(S32 i = 0; i < bounds.size(); i++)
      glVertex(bounds[i]);
   glEnd();
}

};