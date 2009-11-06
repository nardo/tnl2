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

#include "gameType.h"
#include "ship.h"
#include "UIGame.h"
#include "UIMenus.h"
#include "UINameEntry.h"
#include "gameNetInterface.h"
#include "flagItem.h"
#include "glutInclude.h"
#include "engineeredObjects.h"
#include "gameObjectRender.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(GameType);

GameType::GameType()
   : mScoreboardUpdateTimer(1000)
   , mGameTimer(DefaultGameTime)
   , mGameTimeUpdateTimer(30000) 
{
   mNetFlags.set(Ghostable);
   mGameOver = false;
   mTeamScoreLimit = DefaultTeamScoreLimit;
}

void GameType::processArguments(S32 argc, const char **argv)
{
   if(argc > 0)
      mGameTimer.reset(U32(atof(argv[0]) * 60 * 1000));
   if(argc > 1)
      mTeamScoreLimit = atoi(argv[1]);
}

void GameType::idle(GameObject::IdleCallPath path)
{
   U32 deltaT = mCurrentMove.time;
   mLevelInfoDisplayTimer.update(deltaT);
   if(isGhost())
   {
      mGameTimer.update(deltaT);
      return;
   }
   queryItemsOfInterest();
   if(mScoreboardUpdateTimer.update(deltaT))
   {
      mScoreboardUpdateTimer.reset();
      for(S32 i = 0; i < mClientList.size(); i++)
      {
         if(mClientList[i]->clientConnection)
         {
            mClientList[i]->ping = (U32) mClientList[i]->clientConnection->getRoundTripTime();
            if(mClientList[i]->ping > MaxPing)
               mClientList[i]->ping = MaxPing;
         }
      }
      for(S32 i = 0; i < mClientList.size(); i++)
         if(mGameOver || mClientList[i]->wantsScoreboardUpdates)
            updateClientScoreboard(mClientList[i]);
   }

   if(mGameTimeUpdateTimer.update(deltaT))
   {
      mGameTimeUpdateTimer.reset();
      s2cSetTimeRemaining(mGameTimer.getCurrent());
   }

   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->respawnTimer.update(deltaT))
         spawnShip(mClientList[i]->clientConnection);
   }

   if(mGameTimer.update(deltaT))
   {
      gameOverManGameOver();
   }
}

void GameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   if(mLevelInfoDisplayTimer.getCurrent() != 0)
   {
      F32 alpha = 1;
      if(mLevelInfoDisplayTimer.getCurrent() < 1000)
         alpha = mLevelInfoDisplayTimer.getCurrent() * 0.001f;

      glEnable(GL_BLEND);
      glColor4f(1,1,1, alpha);
      UserInterface::drawCenteredStringf(UserInterface::vertMargin + 90, 30, "Level: %s", mLevelName.getString());
      UserInterface::drawCenteredStringf(UserInterface::vertMargin + 130, 30, "Game Type: %s", getGameTypeString());
      UserInterface::drawCenteredString(UserInterface::canvasHeight - UserInterface::vertMargin - 90, 20, getInstructionString());
      UserInterface::drawCenteredString(UserInterface::canvasHeight - UserInterface::vertMargin - 60, 20, mLevelDescription.getString());
      glDisable(GL_BLEND);
   }
   if((mGameOver || scoreboardVisible) && mTeams.size() > 0)
   {
      U32 totalWidth = UserInterface::canvasWidth - UserInterface::horizMargin * 2;
      U32 columnCount = mTeams.size();
      if(columnCount > 2)
         columnCount = 2;

      U32 teamWidth = totalWidth / columnCount;
      U32 maxTeamPlayers = 0;
      countTeamPlayers();

      for(S32 i = 0; i < mTeams.size(); i++)
         if(mTeams[i].numPlayers > maxTeamPlayers)
            maxTeamPlayers = mTeams[i].numPlayers;

      if(!maxTeamPlayers)
         return;

      U32 teamAreaHeight = 40;
      if(mTeams.size() < 2)
         teamAreaHeight = 0;

      U32 numTeamRows = (mTeams.size() + 1) >> 1;

      U32 totalHeight = (UserInterface::canvasHeight - UserInterface::vertMargin * 2) / numTeamRows - (numTeamRows - 1) * 2;
      U32 maxHeight = (totalHeight - teamAreaHeight) / maxTeamPlayers;
      if(maxHeight > 30)
         maxHeight = 30;

      U32 sectionHeight = (teamAreaHeight + maxHeight * maxTeamPlayers);
      totalHeight = sectionHeight * numTeamRows + (numTeamRows - 1) * 2;

      for(S32 i = 0; i < mTeams.size(); i++)
      {
         U32 yt = (UserInterface::canvasHeight - totalHeight) / 2 + (i >> 1) * (sectionHeight + 2);
         U32 yb = yt + sectionHeight;
         U32 xl = 10 + (i & 1) * teamWidth;
         U32 xr = xl + teamWidth - 2;

         Color c = mTeams[i].color;
         glEnable(GL_BLEND);

         glColor4f(c.r, c.g, c.b, 0.6);
         glBegin(GL_POLYGON);
         glVertex2f(xl, yt);
         glVertex2f(xr, yt);
         glVertex2f(xr, yb);
         glVertex2f(xl, yb);
         glEnd();

         glDisable(GL_BLEND);

         glColor3f(1,1,1);
         if(teamAreaHeight)
         {
            renderFlag(Point(xl + 20, yt + 18), c);
            renderFlag(Point(xr - 20, yt + 18), c);

            glColor3f(1,1,1);
            glBegin(GL_LINES);
            glVertex2f(xl, yt + teamAreaHeight);
            glVertex2f(xr, yt + teamAreaHeight);
            glEnd();

            UserInterface::drawString(xl + 40, yt + 2, 30, mTeams[i].name.getString());

            UserInterface::drawStringf(xr - 140, yt + 2, 30, "%d", mTeams[i].score);
         }

         U32 curRowY = yt + teamAreaHeight + 1;
         U32 fontSize = U32(maxHeight * 0.8f);
         for(S32 j = 0; j < mClientList.size(); j++)
         {
            if(mClientList[j]->teamId == i)
            {
               UserInterface::drawString(xl + 40, curRowY, fontSize, mClientList[j]->name.getString());

               static char buff[255] = "";
               dSprintf(buff, sizeof(buff), "%d", mClientList[j]->score);

               UserInterface::drawString(xr - (120 + UserInterface::getStringWidth(fontSize, buff)), curRowY, fontSize, buff);
               UserInterface::drawStringf(xr - 70, curRowY, fontSize, "%d", mClientList[j]->ping);
               curRowY += maxHeight;
            }
         }
      }
   }
   else if(mTeams.size() > 1)
   {
      for(S32 i = 0; i < mTeams.size(); i++)
      {
         Point pos(UserInterface::canvasWidth - UserInterface::horizMargin - 35, UserInterface::canvasHeight - UserInterface::vertMargin - 60 - i * 38);
         renderFlag(pos + Point(-20, 18), mTeams[i].color);
         glColor3f(1,1,1);
         UserInterface::drawStringf(U32(pos.x), U32(pos.y), 32, "%d", mTeams[i].score);
      }
   }
   renderTimeLeft();
   renderTalkingClients();
}

void GameType::renderObjectiveArrow(GameObject *target, Color c, F32 alphaMod)
{
   GameConnection *gc = gClientGame->getConnectionToServer();
   GameObject *co = NULL;
   if(gc)
      co = gc->getControlObject();
   if(!co)
      return;
   Rect r = target->getBounds(MoveObject::RenderState);
   Point nearestPoint = co->getRenderPos();

   if(r.max.x < nearestPoint.x)
      nearestPoint.x = r.max.x;
   if(r.min.x > nearestPoint.x)
      nearestPoint.x = r.min.x;
   if(r.max.y < nearestPoint.y)
      nearestPoint.y = r.max.y;
   if(r.min.y > nearestPoint.y)
      nearestPoint.y = r.min.y;

   renderObjectiveArrow(nearestPoint, c, alphaMod);
}

void GameType::renderObjectiveArrow(Point nearestPoint, Color c, F32 alphaMod)
{
   GameConnection *gc = gClientGame->getConnectionToServer();
   GameObject *co = NULL;
   if(gc)
      co = gc->getControlObject();
   if(!co)
      return;

   Point rp = gClientGame->worldToScreenPoint(nearestPoint);
   Point center(400, 300);
   Point arrowDir = rp - center;

   F32 er = arrowDir.x * arrowDir.x / (350 * 350) + arrowDir.y * arrowDir.y / (250 * 250);
   if(er < 1)
      return;
   Point np = rp;

   er = sqrt(er);
   rp.x = arrowDir.x / er;
   rp.y = arrowDir.y / er;
   rp += center;

   F32 dist = (np - rp).len();

   arrowDir.normalize();
   Point crossVec(arrowDir.y, -arrowDir.x);
   F32 alpha = (1 - gClientGame->getCommanderZoomFraction()) * 0.6 * alphaMod;
   if(!alpha)
      return;

   if(dist < 50)
      alpha *= dist * 0.02;

   Point p2 = rp - arrowDir * 23 + crossVec * 8;
   Point p3 = rp - arrowDir * 23 - crossVec * 8;

   glEnable(GL_BLEND);
   glColor(c * 0.7, alpha);
   glBegin(GL_POLYGON);
   glVertex(rp);
   glVertex(p2);
   glVertex(p3);
   glEnd();
   glColor(c, alpha);
   glBegin(GL_LINE_LOOP);
   glVertex(rp);
   glVertex(p2);
   glVertex(p3);
   glEnd();
   glDisable(GL_BLEND);
}

void GameType::renderTimeLeft()
{
   glColor3f(1,1,1);
   U32 timeLeft = mGameTimer.getCurrent();

   U32 minsRemaining = timeLeft / (60000);
   U32 secsRemaining = (timeLeft - (minsRemaining * 60000)) / 1000;
   UserInterface::drawStringf(UserInterface::canvasWidth - UserInterface::horizMargin - 65,
      UserInterface::canvasHeight - UserInterface::vertMargin - 20, 20, "%02d:%02d", minsRemaining, secsRemaining);
}

void GameType::renderTalkingClients()
{
   S32 y = 150;
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->voiceSFX->isPlaying())
      {
         Color teamColor = mTeams[mClientList[i]->teamId].color;
         glColor(teamColor);
         UserInterface::drawString(10, y, 20, mClientList[i]->name.getString());
         y += 25;
      }
   }
}

void GameType::gameOverManGameOver()
{
   // 17 days??? We won't last 17 hours!
   if (mGameOver)
      return;

   mGameOver = true;
   s2cSetGameOver(true);
   gServerGame->gameEnded();

   onGameOver();
}

void GameType::onGameOver()
{
   static StringTableEntry tieMessage("The game ended in a tie.");
   static StringTableEntry winMessage("%e0%e1 wins the game!");
   static StringTableEntry teamString("Team ");
   static StringTableEntry emptyString;

   bool tied = false;
   Vector<StringTableEntry> e;
   if(mTeams.size() > 1)
   {
      S32 teamWinner = 0;
      U32 winningScore = mTeams[0].score;
      for(S32 i = 1; i < mTeams.size(); i++)
      {
         if(mTeams[i].score == winningScore)
            tied = true;
         else if(mTeams[i].score > winningScore)
         {
            teamWinner = i;
            winningScore = mTeams[i].score;
            tied = false;
         }
      }
      if(!tied)
      {
         e.push_back(teamString);
         e.push_back(mTeams[teamWinner].name);
      }
   }
   else
   {
      if(mClientList.size())
      {
         ClientRef *winningClient = mClientList[0];
         for(S32 i = 1; i < mClientList.size(); i++)
         {
            if(mClientList[i]->score == winningClient->score)
               tied = true;
            else if(mClientList[i]->score > winningClient->score)
            {
               winningClient = mClientList[i];
               tied = false;
            }
         }
         if(!tied)
         {
            e.push_back(emptyString);
            e.push_back(winningClient->name);
         }
      }
   }
   if(tied)
   {
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, tieMessage);
   }
   else
   {
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagCapture, winMessage, e);
   }
}

TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cSetGameOver, (bool gameOver), (gameOver),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   mGameOver = gameOver;
}

void GameType::onAddedToGame(Game *theGame)
{
   theGame->setGameType(this);
}

extern void constructBarriers(Game *theGame, const Vector<F32> &barrier, F32 width);

bool GameType::processLevelItem(S32 argc, const char **argv)
{
   if(!stricmp(argv[0], "Team"))
   {
      if(argc < 5)
         return false;
      Team t;
      t.numPlayers = 0;

      t.name.set(argv[1]);
      t.color.read(argv + 2);
      mTeams.push_back(t);
   }
   else if(!stricmp(argv[0], "Spawn"))
   {
      if(argc < 4)
         return false;
      S32 teamIndex = atoi(argv[1]);
      Point p;
      p.read(argv + 2);
      p *= getGame()->getGridSize();
      if(teamIndex >= 0 && teamIndex < mTeams.size())
         mTeams[teamIndex].spawnPoints.push_back(p);
   }
   else if(!stricmp(argv[0], "BarrierMaker"))
   {
      BarrierRec barrier;
      if(argc < 2)
         return false;
      barrier.width = atof(argv[1]);
      for(S32 i = 2; i < argc; i++)
         barrier.verts.push_back(atof(argv[i]) * getGame()->getGridSize());
      if(barrier.verts.size() > 3)
      {
         mBarriers.push_back(barrier);
         constructBarriers(getGame(), barrier.verts, barrier.width);
      }
   }
   else if(!stricmp(argv[0], "LevelName") && argc > 1)
      mLevelName.set(argv[1]);
   else if(!stricmp(argv[0], "LevelDescription") && argc > 1)
      mLevelDescription.set(argv[1]);
   else
      return false;
   return true;
}

ClientRef *GameType::findClientRef(const StringTableEntry &name)
{
   for(S32 clientIndex = 0; clientIndex < mClientList.size(); clientIndex++)
      if(mClientList[clientIndex]->name == name)
         return mClientList[clientIndex];
   return NULL;
}

void GameType::spawnShip(GameConnection *theClient)
{
   ClientRef *cl = theClient->getClientRef();
   S32 teamIndex = cl->teamId;

   TNLAssert(mTeams[teamIndex].spawnPoints.size(), "No spawn points!");

   Point spawnPoint;
   S32 spawnIndex = Random::readI() % mTeams[teamIndex].spawnPoints.size();
   spawnPoint = mTeams[teamIndex].spawnPoints[spawnIndex];

   Ship *newShip = new Ship(cl->name, teamIndex, spawnPoint);
   newShip->addToGame(getGame());
   theClient->setControlObject(newShip);
   newShip->setOwner(theClient);

   //setClientShipLoadout(clientIndex, theClient->getLoadout());
}

void GameType::updateShipLoadout(GameObject *shipObject)
{
   GameConnection *gc = shipObject->getControllingClient();
   if(!gc)
      return;
   ClientRef *cl = gc->getClientRef();
   if(!cl)
      return;
   setClientShipLoadout(cl, gc->getLoadout());
}

void GameType::setClientShipLoadout(ClientRef *cl, const Vector<U32> &loadout)
{
   if(loadout.size() != 5)
      return;
   Ship *theShip = (Ship *) cl->clientConnection->getControlObject();

   if(theShip)
      theShip->setLoadout(loadout[0], loadout[1], loadout[2], loadout[3], loadout[4]);
}

void GameType::clientRequestLoadout(GameConnection *client, const Vector<U32> &loadout)
{
   //S32 clientIndex = findClientIndexByConnection(client);
   //if(clientIndex != -1)
   //   setClientShipLoadout(clientIndex, loadout);
}

void GameType::performScopeQuery(GhostConnection *connection)
{
   GameConnection *gc = (GameConnection *) connection;
   GameObject *co = gc->getControlObject();

   const Vector<SafePtr<GameObject> > &scopeAlwaysList = getGame()->getScopeAlwaysList();

   gc->objectInScope(this);

   for(S32 i = 0; i < scopeAlwaysList.size(); i++)
   {
      if(scopeAlwaysList[i].isNull())
         continue;
      gc->objectInScope(scopeAlwaysList[i]);
   }
   // readyForRegularGhosts is set once all the RPCs from the GameType
   // have been received and acknowledged by the client
   ClientRef *cl = gc->getClientRef();
   if(cl)
   {
      if(cl->readyForRegularGhosts && co)
      {
         performProxyScopeQuery(co, (GameConnection *) connection);
         gc->objectInScope(co);
      }
   }
}

void GameType::addItemOfInterest(Item *theItem)
{
   ItemOfInterest i;
   i.theItem = theItem;
   i.teamVisMask = 0;
   mItemsOfInterest.push_back(i);
}

void GameType::queryItemsOfInterest()
{
   static Vector<GameObject *> fillVector;
   for(S32 i = 0; i < mItemsOfInterest.size(); i++)
   {
      ItemOfInterest &ioi = mItemsOfInterest[i];
      ioi.teamVisMask = 0;
      Point pos = ioi.theItem->getActualPos();
      Point scopeRange(Game::PlayerSensorHorizVisDistance,
         Game::PlayerSensorVertVisDistance);
      Rect queryRect(pos, pos);

      queryRect.expand(scopeRange);
      findObjects(ShipType, fillVector, queryRect);
      for(S32 j = 0; j < fillVector.size(); j++)
      {
         Ship *theShip = (Ship *) fillVector[j];
         Point delta = theShip->getActualPos() - pos;
         delta.x = fabs(delta.x);
         delta.y = fabs(delta.y);

         if( (theShip->isSensorActive() && delta.x < Game::PlayerSensorHorizVisDistance &&
               delta.y < Game::PlayerSensorVertVisDistance) ||
               (delta.x < Game::PlayerHorizVisDistance &&
               delta.y < Game::PlayerVertVisDistance))
               ioi.teamVisMask |= (1 << theShip->getTeam());
      }
      fillVector.clear();
   }
}

void GameType::performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection)
{
   static Vector<GameObject *> fillVector;
   fillVector.clear();

   if(mTeams.size() > 1)
   {
      for(S32 i = 0; i < mItemsOfInterest.size(); i++)
      {
         if(mItemsOfInterest[i].teamVisMask & (1 << scopeObject->getTeam()))
         {
            Item *theItem = mItemsOfInterest[i].theItem;
            connection->objectInScope(theItem);
            if(theItem->isMounted())
               connection->objectInScope(theItem->getMount());
         }
      }
   }

   if(connection->isInCommanderMap() && mTeams.size() > 1)
   {
      S32 teamId = connection->getClientRef()->teamId;

      for(S32 i = 0; i < mClientList.size(); i++)
      {
         if(mClientList[i]->teamId == teamId)
         {
            if(!mClientList[i]->clientConnection)
               continue;

            Ship *co = (Ship *) mClientList[i]->clientConnection->getControlObject();
            if(!co)
               continue;

            Point pos = co->getActualPos();
            Point scopeRange;
            if(co->isSensorActive())
               scopeRange.set(Game::PlayerSensorHorizVisDistance + Game::PlayerScopeMargin,
                              Game::PlayerSensorVertVisDistance + Game::PlayerScopeMargin);
            else
               scopeRange.set(Game::PlayerHorizVisDistance + Game::PlayerScopeMargin,
                              Game::PlayerVertVisDistance + Game::PlayerScopeMargin);

            Rect queryRect(pos, pos);
            
            queryRect.expand(scopeRange);
            findObjects(scopeObject == co ? AllObjectTypes : CommandMapVisType, fillVector, queryRect);
         }
      }
   }
   else
   {
      Point pos = scopeObject->getActualPos();
      Ship *co = (Ship *) scopeObject;
      Point scopeRange;

      if(co->isSensorActive())
         scopeRange.set(Game::PlayerSensorHorizVisDistance + Game::PlayerScopeMargin,
                        Game::PlayerSensorVertVisDistance + Game::PlayerScopeMargin);
      else
         scopeRange.set(Game::PlayerHorizVisDistance + Game::PlayerScopeMargin,
                        Game::PlayerVertVisDistance + Game::PlayerScopeMargin);

      Rect queryRect(pos, pos);
      queryRect.expand(scopeRange);
      findObjects(AllObjectTypes, fillVector, queryRect);
   }

   for(S32 i = 0; i < fillVector.size(); i++)
      connection->objectInScope(fillVector[i]);
}

Color GameType::getTeamColor(S32 team)
{
   if(team == -1)
      return Color(0.8, 0.8, 0.8);
   else
      return mTeams[team].color;
}

Color GameType::getTeamColor(GameObject *theObject)
{
   return getTeamColor(theObject->getTeam());
}

Color GameType::getShipColor(Ship *s)
{
   return getTeamColor(s->getTeam());
}

void GameType::countTeamPlayers()
{
   for(S32 i = 0; i < mTeams.size(); i ++)
      mTeams[i].numPlayers = 0;

   for(S32 i = 0; i < mClientList.size(); i++)
      mTeams[mClientList[i]->teamId].numPlayers++;
}

void GameType::serverAddClient(GameConnection *theClient)
{
   theClient->setScopeObject(this);

   ClientRef *cref = allocClientRef();
   cref->name = theClient->getClientName();

   cref->clientConnection = theClient;
   countTeamPlayers();

   U32 minPlayers = mTeams[0].numPlayers;
   S32 minTeamIndex = 0;

   for(S32 i = 1; i < mTeams.size(); i++)
   {
      if(mTeams[i].numPlayers < minPlayers)
      {
         minTeamIndex = i;
         minPlayers = mTeams[i].numPlayers;
      }
   }
   cref->teamId = minTeamIndex;
   mClientList.push_back(cref);
   theClient->setClientRef(cref);

   s2cAddClient(cref->name, false);
   s2cClientJoinedTeam(cref->name, cref->teamId);
   spawnShip(theClient);
}


bool GameType::objectCanDamageObject(GameObject *damager, GameObject *victim)
{
   if(!damager)
      return true;

   GameConnection *damagerOwner = damager->getOwner();
   GameConnection *victimOwner = victim->getOwner();

   if(damagerOwner == victimOwner)
      return true;

   if(mTeams.size() <= 1)
      return true;

   return damager->getTeam() != victim->getTeam();
}

void GameType::controlObjectForClientKilled(GameConnection *theClient, GameObject *clientObject, GameObject *killerObject)
{
   GameConnection *killer = killerObject ? killerObject->getOwner() : NULL;
   ClientRef *killerRef = killer ? killer->getClientRef() : NULL;
   ClientRef *clientRef = theClient->getClientRef();

   if(killerRef)
   {
      // Punish team killers slightly
      if(mTeams.size() > 1 && killerRef->teamId == clientRef->teamId)
         killerRef->score -= 1;
      else
         killerRef->score += 1;

      s2cKillMessage(clientRef->name, killerRef->name);
   }
   clientRef->respawnTimer.reset(RespawnDelay);
}

void GameType::addClientGameMenuOptions(Vector<MenuItem> &menuOptions)
{
   if(mTeams.size() > 1)
      menuOptions.push_back(MenuItem("CHANGE TEAMS",1000));
}

void GameType::processClientGameMenuOption(U32 index)
{
   if(index == 1000)
      c2sChangeTeams();
}

void GameType::setTeamScore(S32 teamIndex, S32 newScore)
{
   mTeams[teamIndex].score = newScore;
   s2cSetTeamScore(teamIndex, newScore);
   if(newScore >= mTeamScoreLimit)
      gameOverManGameOver();
}

GAMETYPE_RPC_S2C(GameType, s2cSetLevelInfo, (StringTableEntry levelName, StringTableEntry levelDesc), (levelName, levelDesc))
{
   mLevelName = levelName;
   mLevelDescription = levelDesc;
   mLevelInfoDisplayTimer.reset(LevelInfoDisplayTime);
}

GAMETYPE_RPC_S2C(GameType, s2cSetTimeRemaining, (U32 timeLeft), (timeLeft))
{
   mGameTimer.reset(timeLeft);
}

GAMETYPE_RPC_C2S(GameType, c2sChangeTeams, (), ())
{
   GameConnection *source = (GameConnection *) NetObject::getRPCSourceConnection();
   changeClientTeam(source);
}

void GameType::changeClientTeam(GameConnection *source)
{
   if(mTeams.size() <= 1)
      return;

   ClientRef *cl = source->getClientRef();

   // destroy the old ship
   GameObject *co = source->getControlObject();

   if(co)
      ((Ship *) co)->kill();

   U32 newTeamId = (cl->teamId + 1) % mTeams.size();
   cl->teamId = newTeamId;
   s2cClientJoinedTeam(cl->name, newTeamId);
   spawnShip(source);
}

GAMETYPE_RPC_S2C(GameType, s2cAddClient, (StringTableEntry name, bool isMyClient), (name, isMyClient))
{
   ClientRef *cref = allocClientRef();
   cref->name = name;
   cref->teamId = 0;
   cref->decoder = new LPC10VoiceDecoder();

   cref->voiceSFX = new SFXObject(SFXVoice, NULL, 1, Point(), Point());

   mClientList.push_back(cref);
   gGameUserInterface.displayMessage(Color(0.6f, 0.6f, 0.8f), "%s joined the game.", name.getString());
}

void GameType::serverRemoveClient(GameConnection *theClient)
{
   ClientRef *cl = theClient->getClientRef();
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i] == cl)
      {
         mClientList.erase(i);
         break;
      }
   }
   GameObject *theControlObject = theClient->getControlObject();
   if(theControlObject)
      ((Ship *) theControlObject)->kill();

   s2cRemoveClient(theClient->getClientName());
}

GAMETYPE_RPC_S2C(GameType, s2cRemoveClient, (StringTableEntry name), (name))
{
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->name == name)
      {
         mClientList.erase(i);
         break;
      }
   }
   gGameUserInterface.displayMessage(Color(0.6f, 0.6f, 0.8f), "%s left the game.", name.getString());
}

GAMETYPE_RPC_S2C(GameType, s2cAddTeam, (StringTableEntry teamName, F32 r, F32 g, F32 b), (teamName, r, g, b))
{
   Team team;
   team.name = teamName;
   team.color.r = r;
   team.color.g = g;
   team.color.b = b;
   mTeams.push_back(team);
}

GAMETYPE_RPC_S2C(GameType, s2cSetTeamScore, (U32 teamIndex, U32 score), (teamIndex, score))
{
   mTeams[teamIndex].score = score;
}

GAMETYPE_RPC_S2C(GameType, s2cClientJoinedTeam, (StringTableEntry name, U32 teamIndex), (name, teamIndex))
{
   ClientRef *cl = findClientRef(name);
   cl->teamId = teamIndex;
   gGameUserInterface.displayMessage(Color(0.6f, 0.6f, 0.8f), "%s joined team %s.", name.getString(), mTeams[teamIndex].name.getString());
}

void GameType::onGhostAvailable(GhostConnection *theConnection)
{
   NetObject::setRPCDestConnection(theConnection);

   s2cSetLevelInfo(mLevelName, mLevelDescription);

   for(S32 i = 0; i < mTeams.size(); i++)
   {
      s2cAddTeam(mTeams[i].name, mTeams[i].color.r, mTeams[i].color.g, mTeams[i].color.b);
      s2cSetTeamScore(i, mTeams[i].score);
   }

   // add all the client and team information
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      s2cAddClient(mClientList[i]->name, mClientList[i]->clientConnection == theConnection);
      s2cClientJoinedTeam(mClientList[i]->name, mClientList[i]->teamId);
   }

   // an empty list clears the barriers
   Vector<F32> v;
   s2cAddBarriers(v, 0);

   for(S32 i = 0; i < mBarriers.size(); i++)
   {
      s2cAddBarriers(mBarriers[i].verts, mBarriers[i].width);
   }
   s2cSetTimeRemaining(mGameTimer.getCurrent());
   s2cSetGameOver(mGameOver);
   s2cSyncMessagesComplete(theConnection->getGhostingSequence());

   NetObject::setRPCDestConnection(NULL);
}

GAMETYPE_RPC_S2C(GameType, s2cSyncMessagesComplete, (U32 sequence), (sequence))
{
   c2sSyncMessagesComplete(sequence);
}

GAMETYPE_RPC_C2S(GameType, c2sSyncMessagesComplete, (U32 sequence), (sequence))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *cl = source->getClientRef();
   if(sequence != source->getGhostingSequence())
      return;
   cl->readyForRegularGhosts = true;
}

GAMETYPE_RPC_S2C(GameType, s2cAddBarriers, (Vector<F32> barrier, F32 width), (barrier, width))
{
   if(!barrier.size())
      getGame()->deleteObjects(BarrierType);
   else
      constructBarriers(getGame(), barrier, width);
}

GAMETYPE_RPC_C2S(GameType, c2sSendChat, (bool global, StringPtr message), (global, message))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *cl = source->getClientRef();

   RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this, 
      s2cDisplayChatMessage, (global, source->getClientName(), message));

   sendChatDisplayEvent(cl, global, theEvent);
}

GAMETYPE_RPC_C2S(GameType, c2sSendChatSTE, (bool global, StringTableEntry message), (global, message))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *cl = source->getClientRef();

   RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this, 
      s2cDisplayChatMessageSTE, (global, source->getClientName(), message));

   sendChatDisplayEvent(cl, global, theEvent);
}

void GameType::sendChatDisplayEvent(ClientRef *cl, bool global, NetEvent *theEvent)
{
   S32 teamId = 0;
   
   if(!global)
      teamId = cl->teamId;

   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(global || mClientList[i]->teamId == teamId)
         if(mClientList[i]->clientConnection)
            mClientList[i]->clientConnection->postNetEvent(theEvent);
   }
}

extern Color gGlobalChatColor;
extern Color gTeamChatColor;

GAMETYPE_RPC_S2C(GameType, s2cDisplayChatMessage, (bool global, StringTableEntry clientName, StringPtr message), (global, clientName, message))
{
   Color theColor = global ? gGlobalChatColor : gTeamChatColor;

   gGameUserInterface.displayMessage(theColor, "%s: %s", clientName.getString(), message.getString());
}

GAMETYPE_RPC_S2C(GameType, s2cDisplayChatMessageSTE, (bool global, StringTableEntry clientName, StringTableEntry message), (global, clientName, message))
{
   Color theColor = global ? gGlobalChatColor : gTeamChatColor;

   gGameUserInterface.displayMessage(theColor, "%s: %s", clientName.getString(), message.getString());
}

GAMETYPE_RPC_C2S(GameType, c2sRequestScoreboardUpdates, (bool updates), (updates))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *cl = source->getClientRef();
   cl->wantsScoreboardUpdates = updates;
   if(updates)
      updateClientScoreboard(cl);
}

GAMETYPE_RPC_C2S(GameType, c2sAdvanceWeapon, (), ())
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   Ship *s = dynamic_cast<Ship*>(source->getControlObject());
   if(s)
      s->selectWeapon();
}

Vector<RangedU32<0, GameType::MaxPing> > GameType::mPingTimes; ///< Static vector used for constructing update RPCs
Vector<SignedInt<24> > GameType::mScores;

void GameType::updateClientScoreboard(ClientRef *cl)
{
   mPingTimes.clear();
   mScores.clear();

   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->ping < MaxPing)
         mPingTimes.push_back(mClientList[i]->ping);
      else
         mPingTimes.push_back(MaxPing);
      mScores.push_back(mClientList[i]->score);
   }

   NetObject::setRPCDestConnection(cl->clientConnection);
   s2cScoreboardUpdate(mPingTimes, mScores);
   NetObject::setRPCDestConnection(NULL);
}

GAMETYPE_RPC_S2C(GameType, s2cScoreboardUpdate, (Vector<RangedU32<0, GameType::MaxPing> > pingTimes, Vector<SignedInt<24> > scores), (pingTimes, scores))
{
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(i >= pingTimes.size())
         break;

      mClientList[i]->ping = pingTimes[i];
      mClientList[i]->score = scores[i];
   }
}

GAMETYPE_RPC_S2C(GameType, s2cKillMessage, (StringTableEntry victim, StringTableEntry killer), (victim, killer))
{
   gGameUserInterface.displayMessage(Color(1.0f, 1.0f, 0.8f), 
            "%s zapped %s", killer.getString(), victim.getString());
}

TNL_IMPLEMENT_NETOBJECT_RPC(GameType, c2sVoiceChat, (bool echo, ByteBufferPtr voiceBuffer), (echo, voiceBuffer),
   NetClassGroupGameMask, RPCUnguaranteed, RPCToGhostParent, 0)
{
   // Broadcast this to all clients on the same team
   // Only send back to the source if echo is true.

   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *cl = source->getClientRef();
   if(cl)
   {
      RefPtr<NetEvent> event = TNL_RPC_CONSTRUCT_NETEVENT(this, s2cVoiceChat, (cl->name, voiceBuffer));
      for(S32 i = 0; i < mClientList.size(); i++)
      {
         if(mClientList[i]->teamId == cl->teamId && (mClientList[i] != cl || echo) && mClientList[i]->clientConnection)
            mClientList[i]->clientConnection->postNetEvent(event);
      }
   }
}

TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cVoiceChat, (StringTableEntry clientName, ByteBufferPtr voiceBuffer), (clientName, voiceBuffer),
   NetClassGroupGameMask, RPCUnguaranteed, RPCToGhost, 0)
{
   ClientRef *cl = findClientRef(clientName);
   if(cl)
   {
      ByteBufferPtr playBuffer = cl->decoder->decompressBuffer(*(voiceBuffer.getPointer()));

      //logprintf("Decoded buffer size %d", playBuffer->getBufferSize());
      cl->voiceSFX->queueBuffer(playBuffer);
   }
}

};

