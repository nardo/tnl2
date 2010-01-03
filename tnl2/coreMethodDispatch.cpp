//-----------------------------------------------------------------------------------
//
//   Torque Core Library
//   Copyright (C) 2005 GarageGames.com, Inc.
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

#include "core/coreMethodDispatch.h"
#include "core/coreStringTable.h"
#include "core/coreThread.h"

namespace TNL
{
void read(BitStream &s, StringPtr *val)
{
   char buffer[256];
   s.readString(buffer);
   *val = buffer;
}

void write(BitStream &s, const StringPtr &val)
{
   s.writeString(val.getString());
}

void read(BitStream &s, ByteBufferPtr *val)
{
   U32 size = s.readInt(ByteBufferSizeBitSize);
   *val = new ByteBuffer(size);
   s.read(size, (*val)->getBuffer());
}
void write(BitStream &s, const ByteBufferPtr &val)
{
   s.writeInt(val->getBufferSize(), ByteBufferSizeBitSize);
   s.write(val->getBufferSize(), val->getBuffer());
}

};