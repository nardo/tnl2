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

#include "soccerGame.h"
#include "glutInclude.h"
#include "UIGame.h"
#include "sfx.h"
#include "gameNetInterface.h"
#include "ship.h"
#include "gameObjectRender.h"
#include "goalZone.h"

namespace Zap
{

class Ship;
class SoccerBallItem;

TNL_IMPLEMENT_NETOBJECT(SoccerGameType);

TNL_IMPLEMENT_NETOBJECT_RPC(SoccerGameType, s2cSoccerScoreMessage, 
   (U32 msgIndex, StringTableEntry clientName, U32 teamIndex), (msgIndex, clientName, teamIndex),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   if(msgIndex == SoccerMsgScoreGoal)
   {
      SFXObject::play(SFXFlagCapture);
      gGameUserInterface.displayMessage(Color(0.6f, 1.0f, 0.8f), 
                  "%s scored a goal on team %s!", 
                  clientName.getString(),
                  mTeams[teamIndex].name.getString());
   }
   else if(msgIndex == SoccerMsgScoreOwnGoal)
   {
      SFXObject::play(SFXFlagCapture);
      if(clientName.isNull())
         gGameUserInterface.displayMessage(Color(0.6f, 1.0f, 0.8f), 
                     "A goal was scored on team %s!", 
                     mTeams[teamIndex].name.getString());
      else
         gGameUserInterface.displayMessage(Color(0.6f, 1.0f, 0.8f), 
                     "%s scored a goal for the other team%s!", 
                     clientName.getString(),
                     mTeams.size() > 2 ? "s" : "");
   }
}

void SoccerGameType::addZone(GoalZone *theZone)
{
   mGoals.push_back(theZone);
}

void SoccerGameType::setBall(SoccerBallItem *theBall)
{
   mBall = theBall;
}

void SoccerGameType::scoreGoal(StringTableEntry playerName, U32 goalTeamIndex)
{
   ClientRef *cl = findClientRef(playerName);
   S32 scoringTeam = -1;
   if(cl)
      scoringTeam = cl->teamId;

   if(scoringTeam == -1 || scoringTeam == goalTeamIndex)
   {
      // give all the other teams a point.
      for(S32 i = 0; i < mTeams.size(); i++)
      {
         if(i != goalTeamIndex)
            setTeamScore(i, mTeams[i].score + 1);
      }
      s2cSoccerScoreMessage(SoccerMsgScoreOwnGoal, playerName, goalTeamIndex);
   }
   else
   {
      cl->score += GoalScore;
      setTeamScore(scoringTeam, mTeams[scoringTeam].score + 1);
      s2cSoccerScoreMessage(SoccerMsgScoreGoal, playerName, goalTeamIndex);
   }
}

void SoccerGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   Parent::renderInterfaceOverlay(scoreboardVisible);
   Ship *u = (Ship *) gClientGame->getConnectionToServer()->getControlObject();
   if(!u)
      return;

   for(S32 i = 0; i < mGoals.size(); i++)
   {
      if(mGoals[i]->getTeam() != u->getTeam())
         renderObjectiveArrow(mGoals[i], getTeamColor(mGoals[i]->getTeam()));
   }
   if(mBall.isValid())
      renderObjectiveArrow(mBall, getTeamColor(-1));
}

TNL_IMPLEMENT_NETOBJECT(SoccerBallItem);

SoccerBallItem::SoccerBallItem(Point pos) : Item(pos, true, 30, 4)
{
   mObjectTypeMask |= CommandMapVisType | TurretTargetType;
   mNetFlags.set(Ghostable);
   initialPos = pos;
}

void SoccerBallItem::processArguments(S32 argc, const char **argv)
{
   Parent::processArguments(argc, argv);
   initialPos = mMoveState[ActualState].pos;
}

void SoccerBallItem::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      theGame->getGameType()->addItemOfInterest(this);
   ((SoccerGameType *) theGame->getGameType())->setBall(this);
}

void SoccerBallItem::renderItem(Point pos)
{
   glColor3f(1, 1, 1);
   drawCircle(pos, mRadius);
}

void SoccerBallItem::idle(GameObject::IdleCallPath path)
{
   if(mSendHomeTimer.update(mCurrentMove.time))
      sendHome();
   else if(mSendHomeTimer.getCurrent())
   {
      F32 accelFraction = 1 - (0.98 * mCurrentMove.time * 0.001f);

      mMoveState[ActualState].vel *= accelFraction;
      mMoveState[RenderState].vel *= accelFraction;
   }
   Parent::idle(path);
}

void SoccerBallItem::damageObject(DamageInfo *theInfo)
{
   // compute impulse direction
   Point dv = theInfo->impulseVector - mMoveState[ActualState].vel;
   Point iv = mMoveState[ActualState].pos - theInfo->collisionPoint;
   iv.normalize();
   mMoveState[ActualState].vel += iv * dv.dot(iv) * 0.3;
   if(theInfo->damagingObject && (theInfo->damagingObject->getObjectTypeMask() & ShipType))
   {
      lastPlayerTouch = ((Ship *) theInfo->damagingObject)->mPlayerName;
   }
}

void SoccerBallItem::sendHome()
{
   mMoveState[ActualState].vel = mMoveState[RenderState].vel = Point();
   mMoveState[ActualState].pos = mMoveState[RenderState].pos = initialPos;
   setMaskBits(PositionMask);
   updateExtent();
}

bool SoccerBallItem::collide(GameObject *hitObject)
{
   if(isGhost())
      return true;

   if(hitObject->getObjectTypeMask() & ShipType)
   {
      lastPlayerTouch = ((Ship *) hitObject)->mPlayerName;
   }
   else
   {
      GoalZone *goal = dynamic_cast<GoalZone *>(hitObject);

      if(goal && !mSendHomeTimer.getCurrent())
      {
         SoccerGameType *g = (SoccerGameType *) getGame()->getGameType();
         g->scoreGoal(lastPlayerTouch, goal->getTeam());
         mSendHomeTimer.reset(1500);
      }
   }
   return true;
}

};
