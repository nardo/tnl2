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

#ifndef _SOCCERGAME_H_
#define _SOCCERGAME_H_

#include "gameType.h"
#include "item.h"

namespace Zap
{

class Ship;
class SoccerBallItem;

class SoccerGameType : public GameType
{
   typedef GameType Parent;
   enum Scores
   {
      KillScore    = 1,
      GoalScore    = 5,
   };
   Vector<GoalZone *> mGoals;
   SafePtr<SoccerBallItem> mBall;

public:
   void scoreGoal(StringTableEntry playerName, U32 goalTeamIndex);
   void addZone(GoalZone *theZone);
   void setBall(SoccerBallItem *theBall);
   void renderInterfaceOverlay(bool scoreboardVisible);

   enum {
      SoccerMsgScoreGoal,
      SoccerMsgScoreOwnGoal,
      SoccerMsgGameOverTeamWin,
      SoccerMsgGameOverTie,
   };
   const char *getGameTypeString() { return "Soccer"; }
   virtual const char *getInstructionString() { return "Push the ball into the opposing team's goal."; }

   TNL_DECLARE_RPC(s2cSoccerScoreMessage, (U32 msgIndex, StringTableEntry clientName, U32 teamIndex));
   TNL_DECLARE_CLASS(SoccerGameType);
};

class SoccerBallItem : public Item
{
   typedef Item Parent;
   Point initialPos;
   Timer mSendHomeTimer;
   StringTableEntry lastPlayerTouch;
public:
   SoccerBallItem(Point pos = Point());
   void renderItem(Point pos);
   void sendHome();
   void damageObject(DamageInfo *theInfo);
   void idle(GameObject::IdleCallPath path);
   void processArguments(S32 argc, const char **argv);
   void onAddedToGame(Game *theGame);

   bool collide(GameObject *hitObject);

   TNL_DECLARE_CLASS(SoccerBallItem);
};

};


#endif

