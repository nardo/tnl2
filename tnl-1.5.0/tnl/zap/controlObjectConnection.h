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

#ifndef _CONTROLOBJECTCONNECTION_H_
#define _CONTROLOBJECTCONNECTION_H_

#include "tnl.h"
#include "tnlGhostConnection.h"
#include "gameObject.h"
#include "point.h"
#include "move.h"

using namespace TNL;

namespace Zap
{

class GameObject;

class ControlObjectConnection : public GhostConnection
{
   typedef GhostConnection Parent;
   // move management
   enum {
      MaxPendingMoves = 63,
      MaxMoveTimeCredit = 512,
   };
   Vector<Move> pendingMoves;
   SafePtr<GameObject> controlObject;

   U32 mLastClientControlCRC;
   Point mServerPosition;
   bool mCompressPointsRelative;

   U32 firstMoveIndex;
   U32 highSendIndex[3];
   U32 mMoveTimeCredit;

public:
   ControlObjectConnection();

   void setControlObject(GameObject *theObject);
   GameObject *getControlObject() { return controlObject; }
   U32 getControlCRC();

   void addPendingMove(Move *theMove)
   {
      if(pendingMoves.size() < MaxPendingMoves)
         pendingMoves.push_back(*theMove);
   }

   struct GamePacketNotify : public GhostConnection::GhostPacketNotify
   {
      U32 firstUnsentMoveIndex;
      Point lastControlObjectPosition;
      GamePacketNotify() { firstUnsentMoveIndex =  0; }
   };
   PacketNotify *allocNotify() { return new GamePacketNotify; }

   void writePacket(BitStream *bstream, PacketNotify *notify);
   void readPacket(BitStream *bstream);

   void packetReceived(PacketNotify *notify);
   void processMoveServer(Move *theMove);
   void addToTimeCredit(U32 timeAmount)
   { 
      mMoveTimeCredit += timeAmount; 
      if(mMoveTimeCredit > MaxMoveTimeCredit)
         mMoveTimeCredit = MaxMoveTimeCredit;
   }

   bool isDataToTransmit() { return true; }

   void writeCompressedPoint(Point &p, BitStream *stream);
   void readCompressedPoint(Point &p, BitStream *stream);
};


};

#endif

