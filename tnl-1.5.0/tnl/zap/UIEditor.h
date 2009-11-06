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

#ifndef _UIEDITOR_H_
#define _UIEDITOR_H_

#include "UIMenus.h"
#include "gameLoader.h"
#include "point.h"

namespace Zap
{

class EditorUserInterface : public UserInterface, public LevelLoader
{
   struct WorldItem
   {
      U32 index;
      S32 team;
      F32 width;
      Vector<Point> verts;
      bool selected;
      Vector<bool> vertSelected;
   };
   struct Team
   {
      char name[256];
      Color color;
   };
   char mGameType[256];
   Vector<const char *> mGameTypeArgs;

   Vector<WorldItem> mItems;
   Vector<WorldItem> mOriginalItems;

   Vector<Team> mTeams;

   Vector<Vector<const char *> > mUnknownItems;

   char mEditFileName[255];

   WorldItem mNewItem;
   F32 mCurrentScale;
   Point mCurrentOffset;
   Point mMousePos;
   Point mMouseDownPos;
   S32 mCurrentTeam;
   F32 mGridSize;

   bool mCreatingPoly;
   bool mDragSelecting;

   bool mUp, mDown, mLeft, mRight, mIn, mOut;

   void clearSelection();
   S32 countSelectedItems();
   S32 countSelectedVerts();
   void findHitVertex(Point canvasPos, S32 &hitItem, S32 &hitVertex);
   void findHitItemAndEdge(Point canvasPos, S32 &hitItem, S32 &hitEdge);
   void computeSelectionMinMax(Point &min, Point &max);

   void processLevelLoadLine(int argc, const char **argv);
   void constructItem(U32 itemIndex);
public:
   static bool editorEnabled;
   void setEditName(const char *name);

   void render();
   void renderPoly(WorldItem &p);
   void renderItem(S32 i);
   void onMouseDown(S32 x, S32 y);
   void onMouseUp(S32 x, S32 y);
   void onMouseDragged(S32 x, S32 y);
   void onRightMouseDown(S32 x, S32 y);
   void onMouseMoved(S32 x, S32 y);
   void onKeyDown(U32 key);
   void onKeyUp(U32 key);
   void onActivate();
   void idle(U32 timeDelta);
   void deleteSelection();
   void duplicateSelection();
   void setCurrentTeam(S32 currentTeam);
   void flipSelectionVertical();
   void flipSelectionHorizontal();
   void rotateSelection(F32 angle);

   void incBarrierWidth();
   void decBarrierWidth();

   void saveLevel();
   void testLevel();

   Point convertWindowToCanvasCoord(Point p) { return Point(p.x * canvasWidth / windowWidth, p.y * canvasHeight / windowHeight); }
   Point convertCanvasToLevelCoord(Point p) { return (p - mCurrentOffset) * (1 / mCurrentScale); }
   Point convertLevelToCanvasCoord(Point p) { return p * mCurrentScale + mCurrentOffset; }
   Point snapToLevelGrid(Point p);
};

class EditorMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   EditorMenuUserInterface();
   void render();
   void onActivate();
   void setupMenus();
   void processSelection(U32 index);
   void onEscape();
};

extern EditorUserInterface gEditorUserInterface;
extern EditorMenuUserInterface gEditorMenuUserInterface;
};

#endif

