//-----------------------------------------------------------------------------------
//
//   Torque Network Library
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

#ifndef _TNL_PACKETSTREAM_H_
#define _TNL_PACKETSTREAM_H_

#include "core/coreBitStream.h"
#include "tnl/tnlUDP.h"

namespace TNL
{

class SymmetricCipher;
/// Hashes the BitStream, writing the hash digest into the end of the buffer, and then encrypts with the given cipher
void BitStreamHashAndEncrypt(BitStream *theStream, U32 hashDigestSize, U32 encryptStartOffset, SymmetricCipher *theCipher);

/// Decrypts the BitStream, then checks the hash digest at the end of the buffer to validate the contents
bool BitStreamDecryptAndCheckHash(BitStream *theStream, U32 hashDigestSize, U32 decryptStartOffset, SymmetricCipher *theCipher);

/// PacketStream provides a network interface to the BitStream for easy construction of data packets.
class PacketStream : public BitStream
{
   typedef BitStream Parent;
   U8 buffer[MaxPacketDataSize]; ///< internal buffer for packet data, sized to the maximum UDP packet size.
public:
   /// Constructor assigns the internal buffer to the BitStream.
   PacketStream(U32 targetPacketSize = MaxPacketDataSize) : BitStream(buffer, targetPacketSize, MaxPacketDataSize) {}
   /// Sends this packet to the specified address through the specified socket.
   NetError sendto(Socket &outgoingSocket, const Address &theAddress);
   /// Reads a packet into the stream from the specified socket.
   NetError recvfrom(Socket &incomingSocket, Address *recvAddress);
};

};

#endif