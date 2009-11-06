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

#include "gameConnection.h"
#include "game.h"
#include "gameType.h"
#include "gameNetInterface.h"

#include "UIGame.h"
#include "UIMenus.h"
#include "UINameEntry.h"

namespace Zap
{
// Global list of clients (if we're a server).
GameConnection GameConnection::gClientList;

extern const char *gServerPassword;
extern const char *gAdminPassword;

TNL_IMPLEMENT_NETCONNECTION(GameConnection, NetClassGroupGame, true);

GameConnection::GameConnection()
{
   mNext = mPrev = this;
   setTranslatesStrings();
   mInCommanderMap = false;
   mIsAdmin = false;
}

GameConnection::~GameConnection()
{
   // unlink ourselves if we're in the client list
   mPrev->mNext = mNext;
   mNext->mPrev = mPrev;

   // Tell the user...
   logprintf("%s disconnected", getNetAddress().toString());
}

/// Adds this connection to the doubly linked list of clients.
void GameConnection::linkToClientList()
{
   mNext = gClientList.mNext;
   mPrev = gClientList.mNext->mPrev;
   mNext->mPrev = this;
   mPrev->mNext = this;
}

GameConnection *GameConnection::getClientList()
{
   return gClientList.getNextClient();
}

GameConnection *GameConnection::getNextClient()
{
   if(mNext == &gClientList)
      return NULL;
   return mNext;
}

void GameConnection::setClientRef(ClientRef *theRef)
{
   mClientRef = theRef;
}

ClientRef *GameConnection::getClientRef()
{
   return mClientRef;
}

TNL_IMPLEMENT_RPC(GameConnection, c2sAdminPassword, (StringPtr pass), (pass), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   if(gAdminPassword && !strcmp(gAdminPassword, pass))
   {
      setIsAdmin(true);
      static StringTableEntry msg("Administrator access granted.");
      s2cDisplayMessage(ColorAqua, SFXIncomingMessage, msg);
      s2cSetIsAdmin();
   }
}

TNL_IMPLEMENT_RPC(GameConnection, c2sAdminPlayerAction, 
   (StringTableEntry playerName, U32 actionIndex), (playerName, actionIndex), 
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   if(isAdmin())
   {
      GameConnection *theClient;
      for(theClient = getClientList(); theClient; theClient = theClient->getNextClient())
         if(theClient->getClientName() == playerName)
            break;

      if(!theClient)
         return;

      StringTableEntry msg;
      static StringTableEntry kickMessage("%e0 was kicked from the game by %e1.");
      static StringTableEntry changeTeamMessage("%e0 was team changed by %e1.");
      Vector<StringTableEntry> e;
      e.push_back(theClient->getClientName());
      e.push_back(getClientName());

      switch(actionIndex)
      {
      case PlayerMenuUserInterface::ChangeTeam:
         msg = changeTeamMessage;
         {
            GameType *gt = gServerGame->getGameType();
            gt->changeClientTeam(theClient);
         }
         break;
      case PlayerMenuUserInterface::Kick:
         {
            msg = kickMessage;
            if(theClient->isAdmin())
            {
               static StringTableEntry nokick("Administrators cannot be kicked.");
               s2cDisplayMessage(ColorAqua, SFXIncomingMessage, nokick);
               return;
            }
            ConnectionParameters &p = theClient->getConnectionParameters();
            if(p.mIsArranged)
               gServerGame->getNetInterface()->banHost(p.mPossibleAddresses[0], 30000);
            gServerGame->getNetInterface()->banHost(theClient->getNetAddress(), 30000);

            theClient->disconnect("You were kicked from the game.");
            break;
         }
      default:
         return;
      }
      for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
         walk->s2cDisplayMessageE(ColorAqua, SFXIncomingMessage, msg, e);
   }
}

TNL_IMPLEMENT_RPC(GameConnection, s2cSetIsAdmin, (), (),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   setIsAdmin(true);
}

TNL_IMPLEMENT_RPC(GameConnection, c2sRequestCommanderMap, (), (),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   mInCommanderMap = true;
}

TNL_IMPLEMENT_RPC(GameConnection, c2sReleaseCommanderMap, (), (),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   mInCommanderMap = false;
}

TNL_IMPLEMENT_RPC(GameConnection, c2sRequestLoadout, (Vector<U32> loadout), (loadout), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   mLoadout = loadout;
   GameType *gt = gServerGame->getGameType();
   if(gt)
      gt->clientRequestLoadout(this, mLoadout);
}

static void displayMessage(U32 colorIndex, U32 sfxEnum, const char *message)
{
   static Color colors[] = 
   {
      Color(1,1,1),
      Color(1,0,0),
      Color(0,1,0),
      Color(0,0,1),
      Color(0,1,1),
      Color(1,1,0),
      Color(0.6f, 1, 0.8f),
   };
   gGameUserInterface.displayMessage(colors[colorIndex], "%s", message);
   if(sfxEnum != SFXNone)
      SFXObject::play(sfxEnum);
}

TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayMessageESI, 
                  (RangedU32<0, GameConnection::ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, StringTableEntry formatString,
                  Vector<StringTableEntry> e, Vector<StringPtr> s, Vector<S32> i),
                  (color, sfx, formatString, e, s, i),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   char outputBuffer[256];
   S32 pos = 0;
   const char *src = formatString.getString();
   while(*src)
   {
      if(src[0] == '%' && (src[1] == 'e' || src[1] == 's' || src[1] == 'i') && (src[2] >= '0' && src[2] <= '9'))
      {
         S32 index = src[2] - '0';
         switch(src[1])
         {
            case 'e':
               if(index < e.size())
                  pos += dSprintf(outputBuffer + pos, 256 - pos, "%s", e[index].getString());
               break;
            case 's':
               if(index < s.size())
                  pos += dSprintf(outputBuffer + pos, 256 - pos, "%s", s[index].getString());
               break;
            case 'i':
               if(index < i.size())
                  pos += dSprintf(outputBuffer + pos, 256 - pos, "%d", i[index]);
               break;
         }
         src += 3;
      }
      else
         outputBuffer[pos++] = *src++;

      if(pos >= 255)
         break;
   }
   outputBuffer[pos] = 0;
   displayMessage(color, sfx, outputBuffer);
}                 

TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayMessageE, 
                  (RangedU32<0, GameConnection::ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, StringTableEntry formatString,
                  Vector<StringTableEntry> e), (color, sfx, formatString, e),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   char outputBuffer[256];
   S32 pos = 0;
   const char *src = formatString.getString();
   while(*src)
   {
      if(src[0] == '%' && (src[1] == 'e') && (src[2] >= '0' && src[2] <= '9'))
      {
         S32 index = src[2] - '0';
         switch(src[1])
         {
            case 'e':
               if(index < e.size())
                  pos += dSprintf(outputBuffer + pos, 256 - pos, "%s", e[index].getString());
               break;
         }
         src += 3;
      }
      else
         outputBuffer[pos++] = *src++;

      if(pos >= 255)
         break;
   }
   outputBuffer[pos] = 0;
   displayMessage(color, sfx, outputBuffer);
}                 

TNL_IMPLEMENT_RPC(GameConnection, s2cDisplayMessage, 
                  (RangedU32<0, GameConnection::ColorCount> color, RangedU32<0, NumSFXBuffers> sfx, StringTableEntry formatString),
                  (color, sfx, formatString),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   char outputBuffer[256];
   S32 pos = 0;
   const char *src = formatString.getString();
   while(*src)
   {
      outputBuffer[pos++] = *src++;

      if(pos >= 255)
         break;
   }
   outputBuffer[pos] = 0;
   displayMessage(color, sfx, outputBuffer);
}                 


TNL_IMPLEMENT_RPC(GameConnection, s2cAddLevel, (StringTableEntry name, StringTableEntry type), (name, type),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 1)
{
   mLevelNames.push_back(name);
   mLevelTypes.push_back(type);
}

TNL_IMPLEMENT_RPC(GameConnection, c2sRequestLevelChange, (S32 newLevelIndex), (newLevelIndex), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   if(mIsAdmin)
   {
      static StringTableEntry msg("%e0 changed the level to %e1.");
      Vector<StringTableEntry> e;
      e.push_back(getClientName());
      e.push_back(gServerGame->getLevelName(newLevelIndex));

      gServerGame->cycleLevel(newLevelIndex);
      for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
         walk->s2cDisplayMessageE(ColorYellow, SFXNone, msg, e);

   }
}

void GameConnection::writeConnectRequest(BitStream *stream)
{
   Parent::writeConnectRequest(stream);

   stream->writeString(gPasswordEntryUserInterface.getText());
   stream->writeString(mClientName.getString());
}

bool GameConnection::readConnectRequest(BitStream *stream, const char **errorString)
{
   if(!Parent::readConnectRequest(stream, errorString))
      return false;

   if(gServerGame->isFull())
   {
      *errorString = "Server Full.";
      return false;
   }

   // first read out the password.
   char buf[256];
   
   stream->readString(buf);
   if(gServerPassword && stricmp(buf, gServerPassword))
   {
      *errorString = "PASSWORD";
      return false;
   }

   //now read the player name
   stream->readString(buf);
   size_t len = strlen(buf);

   if(len > 30)
      len = 30;

   // strip leading and trailing spaces...
   char *name = buf;
   while(len && *name == ' ')
   {
      name++;
      len--;
   }
   while(len && name[len-1] == ' ')
      len--;

   // remove invisible chars
   for(size_t i = 0; i < len; i++)
      if(name[i] < ' ' || name[i] > 127)
         name[i] = 'X';

   name[len] = 0;

   U32 index = 0;

checkPlayerName:
   for(GameConnection *walk = getClientList(); walk; walk = walk->getNextClient())
   {
      if(!strcmp(walk->mClientName.getString(), name))
      {
         dSprintf(name + len, 3, ".%d", index);
         index++;
         goto checkPlayerName;
      }
   }

   mClientName = name;
   return true;
}

void GameConnection::onConnectionEstablished()
{
   Parent::onConnectionEstablished();

   if(isInitiator())
   {
      setGhostFrom(false);
      setGhostTo(true);
      logprintf("%s - connected to server.", getNetAddressString());
      setFixedRateParameters(50, 50, 2000, 2000);
   }
   else
   {
      linkToClientList();
      gServerGame->addClient(this);
      setGhostFrom(true);
      setGhostTo(false);
      activateGhosting();
      logprintf("%s - client \"%s\" connected.", getNetAddressString(), mClientName.getString());
      setFixedRateParameters(50, 50, 2000, 2000);
   }
}

void GameConnection::onConnectionTerminated(NetConnection::TerminationReason r, const char *reason)
{
   if(isInitiator())
   {
      gMainMenuUserInterface.activate();
   }
   else
   {
      gServerGame->removeClient(this);
   }
}

void GameConnection::onConnectTerminated(TerminationReason r, const char *string)
{
   if(isInitiator())
   {
      if(!strcmp(string, "PASSWORD"))
      {
         gPasswordEntryUserInterface.setConnectServer(getNetAddress());
         gPasswordEntryUserInterface.activate();
      }
      else
         gMainMenuUserInterface.activate();
   }
}

};

