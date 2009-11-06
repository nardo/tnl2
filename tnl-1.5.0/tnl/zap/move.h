//----------------------------------------------------------------
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

#ifndef _MOVE_H_
#define _MOVE_H_

#include "tnlTypes.h"
#include "tnlBitStream.h"

using namespace TNL;

namespace Zap
{

// some angle conversion functions:
const F32 radiansToDegreesConversion = 360.0f * FloatInverse2Pi;

inline F32 radiansToDegrees(F32 angle)
{
   return angle * radiansToDegreesConversion;
}

inline F32 radiansToUnit(F32 angle)
{
   return angle * FloatInverse2Pi;
}

inline F32 unitToRadians(F32 angle)
{
   return angle * Float2Pi;
}

struct Move
{
   float left;
   float right;
   float up;
   float down;
   float angle;
   bool fire;
   bool module[2];
   U32 time;

   enum {
      MaxMoveTime = 127,
   };

   Move() { left = right = up = down = angle = 0; module[0] = module[1] = fire = false; time = 32; }

   bool isEqualMove(Move *prev)
   {
      return   prev->left == left &&
               prev->right == right &&
               prev->up == up &&
               prev->down == down &&
               prev->angle == angle &&
               prev->fire == fire &&
               prev->module[0] == module[0] &&
               prev->module[1] == module[1];
   }

   void pack(BitStream *stream, Move *prev, bool packTime)
   {
      if(!stream->writeFlag(prev && isEqualMove(prev)))
      {
         stream->writeFloat(left, 4);
         stream->writeFloat(right, 4);
         stream->writeFloat(up, 4);
         stream->writeFloat(down, 4);
         U32 writeAngle = U32(radiansToUnit(angle) * 0xFFF);

         stream->writeInt(writeAngle, 12);
         stream->writeFlag(fire);
         stream->writeFlag(module[0]);
         stream->writeFlag(module[1]);
      }
      if(packTime)
         stream->writeRangedU32(time, 0, MaxMoveTime);
   }
   void unpack(BitStream *stream, bool unpackTime)
   {
      if(!stream->readFlag())
      {
         left = stream->readFloat(4);
         right = stream->readFloat(4);
         up = stream->readFloat(4);
         down = stream->readFloat(4);
         angle = unitToRadians(stream->readInt(12) / F32(0xFFF));
         fire = stream->readFlag();
         module[0] = stream->readFlag();
         module[1] = stream->readFlag();
      }
      if(unpackTime)
         time = stream->readRangedU32(0, MaxMoveTime);
   }
   void prepare()
   {
      PacketStream stream;
      pack(&stream, NULL, false);
      stream.setBytePosition(0);
      unpack(&stream, false);
   }
};

};

#endif

