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

#include "UIEditor.h"
#include "glutInclude.h"
#include "UICredits.h"
#include <stdio.h>
#include <ctype.h>
#include "gameObjectRender.h"
#include "barrier.h"

namespace Zap
{

bool EditorUserInterface::editorEnabled = false;

EditorUserInterface gEditorUserInterface;

void EditorUserInterface::setEditName(const char *name)
{
   mGridSize = Game::DefaultGridSize;
   mGameType[0] = 0;
   char fileBuffer[256];
   dSprintf(fileBuffer, sizeof(fileBuffer), "levels/%s", name);
   initLevelFromFile(fileBuffer);
   if(mTeams.size() == 0)
   {
      Team t;
      t.color = Color(0, 0, 1);
      strcpy(t.name, "Blue");
      mTeams.push_back(t);
   }
   mCurrentOffset.set(0,0);
   mCurrentScale = 100;
   mDragSelecting = false;
   mUp = mDown = mLeft = mRight = mIn = mOut = false;
   mCreatingPoly = false;
   mCurrentTeam = 0;

   strcpy(mEditFileName, name);
   mOriginalItems = mItems;
}

struct GameItemRec
{
   const char *name;
   bool hasWidth;
   bool hasTeam;
   bool canHaveNoTeam;
   bool isPoly;
   char letter;
};

enum GameItems
{
   ItemSpawn,
   ItemSoccerBall,
   ItemCTFFlag,
   ItemBarrierMaker,
   ItemTeleporter,
   ItemRepair,
   ItemTest,
   ItemResource,
   ItemLoadoutZone,
   ItemTurret,
   ItemForceField,
   ItemGoalZone,
};

GameItemRec gGameItemRecs[] = {
   { "Spawn", false, true, false, false, 'S' },
   { "SoccerBallItem", false, false, false, false, 'B' },
   { "FlagItem", false, true, true, false, 0 },
   { "BarrierMaker", true, false, false, true, 0 },
   { "Teleporter", false, false, false, true, 'T' },
   { "RepairItem", false, false, false, false, 'R' },
   { "TestItem", false, false, false, false, 't' },
   { "ResourceItem", false, false, false, false, 'r' },
   { "LoadoutZone", false, true, true, true, 0 },
   { "Turret", false, true, true, false, 'N' },
   { "ForceFieldProjector", false, true, true, false, 'P' },
   { "GoalZone", false, true, false, true, 0 },
   { NULL, false, false, false, false, 0 },
};

void EditorUserInterface::processLevelLoadLine(int argc, const char **argv)
{
   U32 index;
   U32 strlenCmd = (U32) strlen(argv[0]);
   for(index = 0; gGameItemRecs[index].name != NULL; index++)
   {
      if(!strcmp(argv[0], gGameItemRecs[index].name))
      {
         S32 minArgs = 3;
         if(gGameItemRecs[index].isPoly)
            minArgs += 2;
         if(gGameItemRecs[index].hasTeam)
            minArgs++;
         if(argc >= minArgs)
            break;
      }
   }

   if(gGameItemRecs[index].name)
   {
      WorldItem i;
      i.index = index;
      S32 arg = 1;
      i.team = -1;
      i.selected = false;
      i.width = 0;
      if(gGameItemRecs[index].hasWidth)
      {
         i.width = atof(argv[arg]);
         arg++;
      }
      if(gGameItemRecs[index].hasTeam)
      {
         i.team = atoi(argv[arg]);
         arg++;
      }
      for(;arg < argc; arg += 2)
      {
         Point p;
         if(arg != argc - 1)
         {
            p.read(argv + arg);
            i.verts.push_back(p);
            i.vertSelected.push_back(false);
         }
      }
      mItems.push_back(i);
   }
   else if(strlenCmd >= 8 && !strcmp(argv[0] + strlenCmd - 8, "GameType"))
   {
      strcpy(mGameType, argv[0]);
      mGameType[strlenCmd - 8] = 0;
      for(S32 i = 1; i < argc; i++)
         mGameTypeArgs.push_back(strdup(argv[i]));
   }
   else if(!strcmp(argv[0], "Team") && argc == 5)
   {
      Team t;
      strcpy(t.name, argv[1]);
      t.color.read(argv + 2);
      mTeams.push_back(t);
   }
   else
   {
      if(!strcmp(argv[0], "GridSize") && argc == 2)
         mGridSize = atof(argv[1]);

      Vector<const char *> item;
      for(S32 i = 0; i < argc; i++)
         item.push_back(strdup(argv[i]));
      mUnknownItems.push_back(item);
   }
}

void EditorUserInterface::onActivate()
{
   editorEnabled = true;
}

Point EditorUserInterface::snapToLevelGrid(Point p)
{
   F32 mulFactor, divFactor;
   if(mCurrentScale >= 100)
   {
      mulFactor = 10;
      divFactor = 0.1;
   }
   else
   {
      mulFactor = 2;
      divFactor = 0.5;
   }

   return Point(floor(p.x * mulFactor + 0.5) * divFactor, 
                floor(p.y * mulFactor + 0.5) * divFactor);
}

void EditorUserInterface::render()
{
   glColor3f(0.0, 0.0, 0.0);
   if(mCurrentScale >= 100)
   {
      F32 gridScale = mCurrentScale * 0.1;
      F32 yStart = fmod(mCurrentOffset.y, gridScale);
      F32 xStart = fmod(mCurrentOffset.x, gridScale);
      glColor3f(0.2, 0.2, 0.2);
      glBegin(GL_LINES);
      while(yStart < canvasHeight)
      {
         glVertex2f(0, yStart);
         glVertex2f(canvasWidth, yStart);
         yStart += gridScale;
      }
      while(xStart < canvasWidth)
      {
         glVertex2f(xStart, 0);
         glVertex2f(xStart, canvasHeight);
         xStart += gridScale;
      }
      glEnd();
   }

   if(mCurrentScale >= 10)
   {
      F32 yStart = fmod(mCurrentOffset.y, mCurrentScale);
      F32 xStart = fmod(mCurrentOffset.x, mCurrentScale);
      glColor3f(0.4, 0.4, 0.4);
      glBegin(GL_LINES);
      while(yStart < canvasHeight)
      {
         glVertex2f(0, yStart);
         glVertex2f(canvasWidth, yStart);
         yStart += mCurrentScale;
      }
      while(xStart < canvasWidth)
      {
         glVertex2f(xStart, 0);
         glVertex2f(xStart, canvasHeight);
         xStart += mCurrentScale;
      }
      glEnd();
   }
   glColor3f(0.7, 0.7, 0.7);
   glLineWidth(3);
   Point origin = convertLevelToCanvasCoord(Point(0,0));
   glBegin(GL_LINES );
   glVertex2f(0, origin.y);
   glVertex2f(canvasWidth, origin.y);
   glVertex2f(origin.x, 0);
   glVertex2f(origin.x, canvasHeight);
   glEnd();
   glLineWidth(1);

   for(S32 i = 0; i < mItems.size(); i++)
      renderItem(i);

   if(mCreatingPoly)
   {
      Point mouseVertex = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));
      mNewItem.verts.push_back(mouseVertex);
      glLineWidth(3);
      glColor3f(1, 1, 0);
      renderPoly(mNewItem);
      glLineWidth(1);
      mNewItem.verts.erase_fast(mNewItem.verts.size() - 1);
   }
   else
   {
      for(S32 i = 0; i < mItems.size(); i++)
      {
         if(mItems[i].selected && mItems[i].index == ItemBarrierMaker)
         {
            glColor3f(1,1,1);
            drawStringf(680, 5, 20, "Width: %g", mItems[i].width);
            break;
         }
      }
   }
   if(mDragSelecting)
   {
      glColor3f(1,1,1);
      glBegin(GL_LINE_LOOP);
      Point downPos = convertLevelToCanvasCoord(mMouseDownPos);
      glVertex2f(downPos.x, downPos.y);
      glVertex2f(mMousePos.x, downPos.y);
      glVertex2f(mMousePos.x, mMousePos.y);
      glVertex2f(downPos.x, mMousePos.y);
      glEnd();
   }
}

void EditorUserInterface::renderPoly(WorldItem &p)
{
   glBegin(GL_LINE_STRIP);
   for(S32 j = 0; j < p.verts.size(); j++)
   {
      Point v = convertLevelToCanvasCoord(p.verts[j]);
      glVertex2f(v.x, v.y);
   }
   glEnd();
}

extern void constructBarrierPoints(const Vector<Point> &vec, F32 width, Vector<Point> &barrierEnds );

void EditorUserInterface::renderItem(S32 index)
{
   EditorUserInterface::WorldItem &i = mItems[index];
   Point pos = convertLevelToCanvasCoord(i.verts[0]);
   Color c;

   if(i.index == ItemTeleporter)
   {
      Point dest = convertLevelToCanvasCoord(i.verts[1]);
      glColor3f(0,1,0);

      if(i.selected)
         glLineWidth(3);

      glBegin(GL_POLYGON);
      glVertex2f(pos.x - 5, pos.y - 5);
      glVertex2f(pos.x + 5, pos.y - 5);
      glVertex2f(pos.x + 5, pos.y + 5);
      glVertex2f(pos.x - 5, pos.y + 5);
      glEnd();

      glBegin(GL_LINES);
      glVertex2f(pos.x, pos.y);
      glVertex2f(dest.x, dest.y);
      glVertex2f(dest.x - 5, dest.y - 5);
      glVertex2f(dest.x + 5, dest.y + 5);
      glVertex2f(dest.x + 5, dest.y - 5);
      glVertex2f(dest.x - 5, dest.y + 5);
      glEnd();

      glLineWidth(1);

      glColor3f(1,1,1);
      if(i.vertSelected[0])
      {
         glBegin(GL_LINE_LOOP);
         glVertex2f(pos.x - 5, pos.y - 5);
         glVertex2f(pos.x + 5, pos.y - 5);
         glVertex2f(pos.x + 5, pos.y + 5);
         glVertex2f(pos.x - 5, pos.y + 5);
         glEnd();
      }
      if(i.vertSelected[1])
      {
         glBegin(GL_LINE_LOOP);
         glVertex2f(dest.x - 5, dest.y - 5);
         glVertex2f(dest.x + 5, dest.y - 5);
         glVertex2f(dest.x + 5, dest.y + 5);
         glVertex2f(dest.x - 5, dest.y + 5);
         glEnd();
      }
   }
   else if(gGameItemRecs[i.index].isPoly)
   {
      Color theColor;
      if(i.selected)
         theColor.set(1,1,0);
      else if(i.index == ItemBarrierMaker)
         theColor.set(0,0,1);
      else if(i.team == -1)
         theColor.set(0.7, 0.7, 0.7);
      else
      {
         Color c = mTeams[i.team].color;
         theColor = c;
      }
      glColor(theColor);

      if(i.index != ItemBarrierMaker)
      {
         // render the team ones as GL_POLYGONs
         glBegin(GL_POLYGON);
         for(S32 j = 0; j < i.verts.size(); j++)
         {
            Point v = convertLevelToCanvasCoord(i.verts[j]);
            glVertex2f(v.x, v.y);
         }
         glEnd();
      }
      else
      {
         Vector<Point> barPoints;
         constructBarrierPoints(i.verts, i.width / mGridSize, barPoints );

         for(S32 j = 0; j < barPoints.size(); j += 2)
         {
            Point dir = barPoints[j+1] - barPoints[j];
            Point crossVec(dir.y, -dir.x);
            crossVec.normalize(i.width * 0.5 / mGridSize);
            glBegin(GL_POLYGON);
            glColor3f(0.5, 0.5, 1);
            glVertex(convertLevelToCanvasCoord(barPoints[j] + crossVec));
            glVertex(convertLevelToCanvasCoord(barPoints[j] - crossVec));
            glVertex(convertLevelToCanvasCoord(barPoints[j+1] - crossVec));
            glVertex(convertLevelToCanvasCoord(barPoints[j+1] + crossVec));
            glEnd();
         }
         glColor(theColor);
         glLineWidth(3);
         renderPoly(i);
         glLineWidth(1);
      }
      for(S32 j = 0; j < i.verts.size(); j++)
      {
         Point v = convertLevelToCanvasCoord(i.verts[j]);
         if(i.vertSelected[j])
            glColor3f(1,1,0);
         else
            glColor3f(1,0,0);
         glBegin(GL_LINE_LOOP);
         glVertex2f(v.x - 5, v.y - 5);
         glVertex2f(v.x + 5, v.y - 5);
         glVertex2f(v.x + 5, v.y + 5);
         glVertex2f(v.x - 5, v.y + 5);
         glEnd();
      }
   }
   else
   {
      char letter = gGameItemRecs[i.index].letter;
      if(i.team == -1)
         c = Color(0.5, 0.5, 0.5);
      else
         c = mTeams[i.team].color;
      if(i.index == ItemCTFFlag)
      {
         glPushMatrix();
         glTranslatef(pos.x, pos.y, 0);
         glScalef(0.6, 0.6, 1);
         renderFlag(Point(0,0), c);
         glPopMatrix();
      }
      else
      {
         glColor(c);
         glBegin(GL_POLYGON);
         glVertex2f(pos.x - 8, pos.y - 8);
         glVertex2f(pos.x + 8, pos.y - 8);
         glVertex2f(pos.x + 8, pos.y + 8);
         glVertex2f(pos.x - 8, pos.y + 8);
         glEnd();
      }
      if(i.selected)
      {
         Point pos = convertLevelToCanvasCoord(i.verts[0]);
         glColor3f(1,1,1);
         glBegin(GL_LINE_LOOP);
         glVertex2f(pos.x - 10, pos.y - 10);
         glVertex2f(pos.x + 10, pos.y - 10);
         glVertex2f(pos.x + 10, pos.y + 10);
         glVertex2f(pos.x - 10, pos.y + 10);
         glEnd();
      }
      if(letter)
      {
         glColor3f(1,1,1);
         drawStringf(pos.x - 5, pos.y - 8, 15, "%c", letter);
      }
   }
}

void EditorUserInterface::clearSelection()
{
   for(S32 i = 0; i < mItems.size(); i++)
   {
      WorldItem &itm = mItems[i];
      itm.selected = false;
      for(S32 j = 0; j < itm.vertSelected.size(); j++)
         itm.vertSelected[j] = false;
   }
}

S32 EditorUserInterface::countSelectedItems()
{
   S32 count = 0;
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected)
         count++;
   return count;
}

S32 EditorUserInterface::countSelectedVerts()
{
   S32 count = 0;
   for(S32 i = 0; i < mItems.size(); i++)
      for(S32 j = 0; j < mItems[i].vertSelected.size(); j++)
         if(mItems[i].vertSelected[j])
            count++;
   return count;
}

void EditorUserInterface::duplicateSelection()
{
   mOriginalItems = mItems;

   S32 itemCount = mItems.size();
   for(S32 i = 0; i < itemCount; i++)
   {
      if(mItems[i].selected)
      {
         WorldItem newItem = mItems[i];
         mItems[i].selected = false;
         for(S32 j = 0; j < newItem.verts.size(); j++)
            newItem.verts[j] += Point(0.5, 0.5);
         mItems.push_back(newItem);
      }
   }
}

void EditorUserInterface::rotateSelection(F32 angle)
{
   mOriginalItems = mItems;
   Point min, max;
   computeSelectionMinMax(min,max);
   //Point ctr = (min + max)*0.5;
   //ctr.x = floor(ctr.x * 10) * 0.1f;
   //ctr.y = floor(ctr.y * 10) * 0.1f;
   F32 sinTheta = sin(angle * Float2Pi / 360.0f);
   F32 cosTheta = cos(angle * Float2Pi / 360.0f);
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(!mItems[i].selected)
         continue;
      WorldItem &itm = mItems[i];
      for(S32 j = 0; j < itm.verts.size(); j++)
      {
         Point v = itm.verts[j];// - ctr;
         Point n(v.x * cosTheta + v.y * sinTheta, v.y * cosTheta - v.x * sinTheta);
         itm.verts[j] = n;// + ctr;
      }
   }
}


void EditorUserInterface::computeSelectionMinMax(Point &min, Point &max)
{
   min.set(1000000, 1000000);
   max.set(-1000000, -1000000);

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(!mItems[i].selected)
         continue;
      WorldItem &itm = mItems[i];
      for(S32 j = 0; j < itm.verts.size(); j++)
      {
         Point v = itm.verts[j];

         if(v.x < min.x)
            min.x = v.x;
         if(v.x > max.x)
            max.x = v.x;
         if(v.y < min.y)
            min.y = v.y;
         if(v.y > max.y)
            max.y = v.y;
      }
   }
}

void EditorUserInterface::setCurrentTeam(S32 currentTeam)
{
   mCurrentTeam = currentTeam;

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(!mItems[i].selected)
         continue;
      if(!gGameItemRecs[mItems[i].index].hasTeam)
         continue;
      if(currentTeam == -1 && !gGameItemRecs[mItems[i].index].canHaveNoTeam)
         continue;
      mItems[i].team = mCurrentTeam;
   }
}

void EditorUserInterface::flipSelectionHorizontal()
{
   mOriginalItems = mItems;

   Point min, max;
   computeSelectionMinMax(min, max);
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(!mItems[i].selected)
         continue;

      for(S32 j = 0; j < mItems[i].verts.size(); j++)
         mItems[i].verts[j].x = min.x + (max.x - mItems[i].verts[j].x);
   }
}

void EditorUserInterface::flipSelectionVertical()
{
   mOriginalItems = mItems;

   Point min, max;
   computeSelectionMinMax(min, max);
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(!mItems[i].selected)
         continue;

      for(S32 j = 0; j < mItems[i].verts.size(); j++)
         mItems[i].verts[j].y = min.y + (max.y - mItems[i].verts[j].y);
   }
}

void EditorUserInterface::findHitVertex(Point canvasPos, S32 &hitItem, S32 &hitVertex)
{
   hitItem = -1;
   hitVertex = -1;
   for(S32 i = mItems.size() - 1; i >= 0; i--)
   {
      WorldItem &p = mItems[i];
      if(!gGameItemRecs[p.index].isPoly)
         continue;

      for(S32 j = p.verts.size() - 1; j >= 0; j--)
      {
         Point v = convertLevelToCanvasCoord(p.verts[j]);
         if(fabs(v.x - canvasPos.x) < 5 && fabs(v.y - canvasPos.y) < 5)
         {
            hitItem = i;
            hitVertex = j;
            return;
         }
      }
   }
}

void EditorUserInterface::findHitItemAndEdge(Point canvasPos, S32 &hitItem, S32 &hitEdge)
{
   hitItem = -1;
   hitEdge = -1;

   for(S32 i = mItems.size() - 1; i >= 0; i--)
   {
      WorldItem &p = mItems[i];

      if(!gGameItemRecs[mItems[i].index].isPoly)
      {
         Point pos = convertLevelToCanvasCoord(p.verts[0]);
         if(fabs(canvasPos.x - pos.x) < 8 && fabs(canvasPos.y - pos.y) < 8)
         {
            hitItem = i;
            return;
         }
      }

      Point p1 = convertLevelToCanvasCoord(p.verts[0]);
      for(S32 j = 0; j < p.verts.size() - 1; j++)
      {
         Point p2 = convertLevelToCanvasCoord(p.verts[j+1]);

         Point edgeDelta = p2 - p1;
         Point clickDelta = canvasPos - p1;
         float fraction = clickDelta.dot(edgeDelta);
         float lenSquared = edgeDelta.dot(edgeDelta);
         if(fraction > 0 && fraction < lenSquared)
         {
            // compute the closest point:
            Point closest = p1 + edgeDelta * (fraction / lenSquared);
            float distance = (canvasPos - closest).len();
            if(distance < 5)
            {
               hitEdge = j;
               hitItem = i;
               return;
            }
         }
         p1 = p2;
      }
   }
}

void EditorUserInterface::onMouseDown(S32 x, S32 y)
{
   mMousePos = convertWindowToCanvasCoord(Point(x,y));
   if(mCreatingPoly)
   {
      mOriginalItems = mItems;
      if(mNewItem.verts.size() > 1)
         mItems.push_back(mNewItem);
      mNewItem.verts.clear();
      mCreatingPoly = false;
   }

   mMouseDownPos = convertCanvasToLevelCoord(mMousePos);

   // rules for mouse down:
   // if the click has no shift- modifier, then
   //   if the click was on something that was selected
   //     do nothing
   //   else
   //     clear the selection
   //     add what was clicked to the selection
   //  else
   //    toggle the selection of what was clicked

   bool shiftKeyDown = glutGetModifiers() & GLUT_ACTIVE_SHIFT;

   S32 vertexHit, vertexHitPoly;
   S32 edgeHit, itemHit;
   
   findHitVertex(mMousePos, vertexHitPoly, vertexHit);
   findHitItemAndEdge(mMousePos, itemHit, edgeHit);

   if(!shiftKeyDown)
   {
      if(vertexHit != -1 && mItems[vertexHitPoly].selected)
      {
         vertexHit = -1;
         itemHit = vertexHitPoly;
      }
      if(vertexHit != -1 && (itemHit == -1 || !mItems[itemHit].selected))
      {
         if(!mItems[vertexHitPoly].vertSelected[vertexHit])
         {
            clearSelection();
            mItems[vertexHitPoly].vertSelected[vertexHit] = true;
         }
      }
      else if(itemHit != -1)
      {
         if(!mItems[itemHit].selected)
         {
            clearSelection();
            mItems[itemHit].selected = true;
         }
      }
      else
      {
         mDragSelecting = true;
         clearSelection();
      }
   }
   else
   {
      if(vertexHit != -1)
      {
         mItems[vertexHitPoly].vertSelected[vertexHit] = 
            !mItems[vertexHitPoly].vertSelected[vertexHit];
      }
      else if(itemHit != -1)
         mItems[itemHit].selected = !mItems[itemHit].selected;
      else
         mDragSelecting = true;
   }
   mOriginalItems = mItems;
}

void EditorUserInterface::onMouseUp(S32 x, S32 y)
{
   mMousePos = convertWindowToCanvasCoord(Point(x,y));
   if(mDragSelecting)
   {
      Rect r(convertCanvasToLevelCoord(mMousePos),
             mMouseDownPos);
      for(S32 i = 0; i < mItems.size(); i++)
      {
         S32 j;
         for(j = 0; j < mItems[i].verts.size(); j++)
         {
            if(!r.contains(mItems[i].verts[j]))
               break;
         }
         if(j == mItems[i].verts.size())
            mItems[i].selected = true;
      }
      mDragSelecting = false;
   }
}

void EditorUserInterface::onMouseDragged(S32 x, S32 y)
{
   mMousePos = convertWindowToCanvasCoord(Point(x,y));
   if(mCreatingPoly || mDragSelecting)
      return;
   Point delta = convertCanvasToLevelCoord(mMousePos);
   delta = snapToLevelGrid(delta - mMouseDownPos);

   for(S32 i = 0; i < mItems.size(); i++)
   {
      for(S32 j = 0; j < mItems[i].verts.size(); j++)
      {
         if(mItems[i].selected || mItems[i].vertSelected[j])
            mItems[i].verts[j] = mOriginalItems[i].verts[j] + delta;
      }
   }
}

void EditorUserInterface::onRightMouseDown(S32 x, S32 y)
{
   mMousePos = convertWindowToCanvasCoord(Point(x,y));
   if(mCreatingPoly)
   {
      Point newVertex = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));
      mNewItem.verts.push_back(newVertex);
      mNewItem.vertSelected.push_back(false);
      return;
   }

   S32 edgeHit, itemHit;
   findHitItemAndEdge(mMousePos, itemHit, edgeHit);

   if(itemHit != -1 && gGameItemRecs[mItems[itemHit].index].isPoly &&
      mItems[itemHit].index != ItemTeleporter)
   {
      clearSelection();

      Point newVertex = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));
      // insert an extra vertex at the mouse clicked point,
      // and then select it.
      mItems[itemHit].verts.insert(edgeHit + 1);
      mItems[itemHit].verts[edgeHit + 1] = newVertex;
      mItems[itemHit].vertSelected.insert(edgeHit + 1);
      mItems[itemHit].vertSelected[edgeHit + 1] = true;
      mMouseDownPos = newVertex;
      mOriginalItems = mItems;
   }
   else
   {
      //london chikara kelly markling
      mCreatingPoly = true;
      mNewItem.verts.clear();
      mNewItem.index = ItemBarrierMaker;
      mNewItem.width = Barrier::BarrierWidth;
      mNewItem.team = -1;
      mNewItem.selected = false;
      mNewItem.vertSelected.clear();
      Point newVertex = snapToLevelGrid(convertCanvasToLevelCoord(mMousePos));
      mNewItem.verts.push_back(newVertex);
      mNewItem.vertSelected.push_back(false);
   }
}

void EditorUserInterface::onMouseMoved(S32 x, S32 y)
{
   glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
   mMousePos = convertWindowToCanvasCoord(Point(x,y));
   //bool shiftKeyDown = glutGetModifiers() & GLUT_ACTIVE_SHIFT;

   if(!mCreatingPoly)// && !shiftKeyDown)
   {
      S32 vertexHit, vertexHitPoly;
      S32 edgeHit, itemHit;
   
      findHitVertex(mMousePos, vertexHitPoly, vertexHit);
      findHitItemAndEdge(mMousePos, itemHit, edgeHit);
      if( (vertexHit != -1 && mItems[vertexHitPoly].vertSelected[vertexHit]) ||
          (itemHit != -1 && mItems[itemHit].selected))
         glutSetCursor(GLUT_CURSOR_SPRAY);
   }
}

void EditorUserInterface::deleteSelection()
{
   for(S32 i = 0; i < mItems.size(); )
   {
      if(mItems[i].selected)
      {
         mItems.erase(i);
      }
      else
      {
         for(S32 j = 0; j < mItems[i].verts.size(); )
         {
            if(mItems[i].vertSelected[j])
            {
               mItems[i].verts.erase(j);
               mItems[i].vertSelected.erase(j);
            }
            else
               j++;
         }

         if(mItems[i].verts.size() == 0)
            mItems.erase(i);
         else
            i++;
      }
   }
}

void EditorUserInterface::incBarrierWidth()
{
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].index == ItemBarrierMaker && mItems[i].selected)
         mItems[i].width += 5;
   }
}

void EditorUserInterface::decBarrierWidth()
{
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].index == ItemBarrierMaker && mItems[i].selected && mItems[i].width >= 9)
         mItems[i].width -= 5;
   }
}

void EditorUserInterface::constructItem(U32 itemIndex)
{
   clearSelection();
   WorldItem w;
   w.index = itemIndex;
   if(gGameItemRecs[itemIndex].hasTeam)
      w.team = mCurrentTeam;
   else
      w.team = -1;
   if(w.team == -1 && gGameItemRecs[itemIndex].hasTeam && !gGameItemRecs[itemIndex].canHaveNoTeam)
      w.team = 0;

   w.selected = false;
   Point pos = convertCanvasToLevelCoord(mMousePos);
   w.verts.push_back(snapToLevelGrid(pos));
   w.vertSelected.push_back(false);
   if(itemIndex == ItemTeleporter)
   {
      w.verts.push_back(w.verts[0] + Point(1,1));
      w.vertSelected.push_back(false);
   }
   mItems.push_back(w);
}

void EditorUserInterface::onKeyDown(U32 key)
{
   bool ctrlActive = glutGetModifiers() & GLUT_ACTIVE_CTRL;

   if(key >= '0' && key < U32('1' + mTeams.size()))
   {
      setCurrentTeam(S32(key) - '1');
      return;
   }

   switch(tolower(key))
   {
      case 0x4: // control-d
         duplicateSelection();
         break;
      case 'f':
         flipSelectionHorizontal();
         break;
      case 26:
         {
            Vector<WorldItem> temp = mItems;
            mItems = mOriginalItems;
            mOriginalItems = temp;
            break;
         }
      case 'v':
         flipSelectionVertical();
         break;
      case 'r':
         if (key == 'R')
            rotateSelection(15.0f);
         else
            rotateSelection(-15.0f);
         break;
      case 'z':
         mCurrentOffset.set(0,0);
         break;
      case 'w':
         mUp = true;
         break;
      case 's':
         mDown = true;
         break;
      case 'a':
         mLeft = true;
         break;
      case 'd':
         mRight = true;
         break;
      case 'e':
         mIn = true;
         break;
      case 'c':
         mOut = true;
         break;
      case 't':
         constructItem(ItemTeleporter);
         break;
      case 'g':
         constructItem(ItemSpawn);
         break;
      case 'b':
         constructItem(ItemRepair);
         break;
      case 'y':
         constructItem(ItemTurret);
         break;
      case 'h':
         constructItem(ItemForceField);
         break;
      case 8:
      case 127:
         deleteSelection();
         break;
      case '+':
      case '=':
         incBarrierWidth();
         break;
      case '-':
      case '_':
         decBarrierWidth();
         break;
      case 27:
         gEditorMenuUserInterface.activate();
         break;
   }
}

void EditorUserInterface::onKeyUp(U32 key)
{
   switch(tolower(key))
   {
      case 'w':
         mUp = false;
         break;
      case 's':
         mDown = false;
         break;
      case 'a':
         mLeft = false;
         break;
      case 'd':
         mRight = false;
         break;
      case 'e':
         mIn = false;
         break;
      case 'c':
         mOut = false;
         break;
   }
}

void EditorUserInterface::idle(U32 timeDelta)
{
   F32 pixelsToScroll = timeDelta * 0.5f;

   if(mLeft && !mRight)
      mCurrentOffset.x += pixelsToScroll;
   else if(mRight && !mLeft)
      mCurrentOffset.x -= pixelsToScroll;
   if(mUp && !mDown)
      mCurrentOffset.y += pixelsToScroll;
   else if(mDown && !mUp)
      mCurrentOffset.y -= pixelsToScroll;

   Point mouseLevelPoint = convertCanvasToLevelCoord(mMousePos);
   if(mIn && !mOut)
      mCurrentScale *= 1 + timeDelta * 0.002;
   if(mOut && !mIn)
      mCurrentScale *= 1 - timeDelta * 0.002;
   if(mCurrentScale > 200)
      mCurrentScale = 200;
   else if(mCurrentScale < 10)
      mCurrentScale = 10;
   Point newMousePoint = convertLevelToCanvasCoord(mouseLevelPoint);
   mCurrentOffset += mMousePos - newMousePoint;   
}

static void escapeString(const char *src, char dest[1024])
{
   S32 i;
   for(i = 0; src[i]; i++)
      if(src[i] == '\"' || src[i] == ' ' || src[i] == '\n' || src[i] == '\t')
         break;

   if(!src[i])
   {
      strcpy(dest, src);
      return;
   }
   char *dptr = dest;
   *dptr++ = '\"';
   char c;
   while((c = *src++) != 0)
   {
      switch(c)
      {
         case '\"':
            *dptr++ = '\\';
            *dptr++ = '\"';
            break;
         case '\n':
            *dptr++ = '\\';
            *dptr++ = 'n';
            break;
         case '\t':
            *dptr++ = '\\';
            *dptr++ = 't';
            break;
         default:
            *dptr++ = c;
      }
   }
   *dptr++ = '\"';
   *dptr++ = 0;
}

void EditorUserInterface::saveLevel()
{
   char fileNameBuffer[256];
   dSprintf(fileNameBuffer, sizeof(fileNameBuffer), "levels/%s", mEditFileName);
   FILE *f = fopen(fileNameBuffer, "w");
   fprintf(f, "%sGameType", mGameType);
   for(S32 i = 0; i < mGameTypeArgs.size(); i++)
      fprintf(f, " %s", mGameTypeArgs[i]);
   fprintf(f, "\n");
   for(S32 i = 0; i < mTeams.size(); i++)
   {
      fprintf(f, "Team %s %g %g %g\n", mTeams[i].name,
         mTeams[i].color.r, mTeams[i].color.g, mTeams[i].color.b);
   }
   for(S32 i = 0; i < mUnknownItems.size(); i++)
   {
      Vector<const char *> &v = mUnknownItems[i];
      for(S32 j = 0; j < v.size(); j++)
      {
         char outBuffer[1024];
         escapeString(v[j], outBuffer);
         fputs(outBuffer, f);
         if(j == v.size() - 1)
            fputs("\n", f);
         else
            fputs(" ", f);
      }
   }
   for(S32 i = 0; i < mItems.size(); i++)
   {
      WorldItem &p = mItems[i];
      fprintf(f, "%s ", gGameItemRecs[mItems[i].index].name);
      if(gGameItemRecs[mItems[i].index].hasWidth)
         fprintf(f, "%g ", mItems[i].width);
      if(gGameItemRecs[mItems[i].index].hasTeam)
         fprintf(f, "%d ", mItems[i].team);
      for(S32 j = 0; j < p.verts.size(); j++)
         fprintf(f, "%g %g%c", p.verts[j].x, p.verts[j].y, (j == p.verts.size() - 1) ? '\n' : ' ');
   }

   fclose(f);
}

extern void hostGame(bool dedicated, Address bindAddress);
extern const char *gLevelList;

void EditorUserInterface::testLevel()
{
   char tmpFileName[256];
   strcpy(tmpFileName, mEditFileName);
   strcpy(mEditFileName, "editor.tmp");
   saveLevel();
   strcpy(mEditFileName, tmpFileName);
   const char *gLevelSave = gLevelList;
   gLevelList = "editor.tmp";
   hostGame(false, Address(IPProtocol, Address::Any, 28000));
   gLevelList = gLevelSave;
}

EditorMenuUserInterface gEditorMenuUserInterface;

EditorMenuUserInterface::EditorMenuUserInterface()
{
   menuTitle = "EDITOR MENU:";
}

void EditorMenuUserInterface::onActivate()
{
   setupMenus();
}

void EditorMenuUserInterface::setupMenus()
{
   Parent::onActivate();
   menuItems.clear();
   menuItems.push_back(MenuItem("RETURN TO EDITOR", 0));
   if(OptionsMenuUserInterface::fullscreen)
      menuItems.push_back(MenuItem("SET WINDOWED MODE", 1));
   else
      menuItems.push_back(MenuItem("SET FULLSCREEN MODE", 1));
   menuItems.push_back(MenuItem("TEST LEVEL",2));
   menuItems.push_back(MenuItem("SAVE LEVEL",3));
   menuItems.push_back(MenuItem("QUIT",4));
}

void EditorMenuUserInterface::processSelection(U32 index)
{
   switch(index)
   {
      case 0:
         gEditorUserInterface.activate();
         break;
      case 1:
         gOptionsMenuUserInterface.processSelection(1);
         setupMenus();
         break;
      case 2:
         gEditorUserInterface.testLevel();
         break;
      case 3:
         gEditorUserInterface.activate();
         gEditorUserInterface.saveLevel();
         break;
      case 4:
         gCreditsUserInterface.activate();
         break;
   }
}

void EditorMenuUserInterface::onEscape()
{
   gEditorUserInterface.activate();
}

void EditorMenuUserInterface::render()
{
   gEditorUserInterface.render();
   glColor4f(0, 0, 0, 0.5);
   glEnable(GL_BLEND);
   glBegin(GL_POLYGON);
   glVertex2f(0, 0);
   glVertex2f(canvasWidth, 0);
   glVertex2f(canvasWidth, canvasHeight);
   glVertex2f(0, canvasHeight);
   glEnd();  
   glDisable(GL_BLEND); 
   Parent::render();
}

};
