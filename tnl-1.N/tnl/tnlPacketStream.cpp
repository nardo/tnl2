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

#include "tnl/tnlPacketStream.h"
#include "tnl/tnlSymmetricCipher.h"
#include <mycrypt.h>

namespace TNL
{

void BitStreamHashAndEncrypt(BitStream *theStream, U32 hashDigestSize, U32 encryptStartOffset, SymmetricCipher *theCipher)
{
   U32 digestStart = theStream->getBytePosition();
   theStream->setBytePosition(digestStart);
   hash_state hashState;

   U8 hash[32];

   // do a sha256 hash of the BitStream:
   sha256_init(&hashState);
   sha256_process(&hashState, theStream->getBuffer(), digestStart);
   sha256_done(&hashState, hash);

   // write the hash into the BitStream:
   theStream->write(hashDigestSize, hash);

   theCipher->encrypt(theStream->getBuffer() + encryptStartOffset,
      theStream->getBuffer() + encryptStartOffset,
      theStream->getBytePosition() - encryptStartOffset);   
}

bool BitStreamDecryptAndCheckHash(BitStream *theStream, U32 hashDigestSize, U32 decryptStartOffset, SymmetricCipher *theCipher)
{
   U32 bufferSize = theStream->getBufferSize();
   U8 *buffer = theStream->getBuffer();

   if(bufferSize < decryptStartOffset + hashDigestSize)
      return false;

   theCipher->decrypt(buffer + decryptStartOffset,
      buffer + decryptStartOffset,
      bufferSize - decryptStartOffset);

   hash_state hashState;
   U8 hash[32];

   sha256_init(&hashState);
   sha256_process(&hashState, buffer, bufferSize - hashDigestSize);
   sha256_done(&hashState, hash);

   bool ret = !memcmp(buffer + bufferSize - hashDigestSize, hash, hashDigestSize);
   if(ret)
      theStream->resize(bufferSize - hashDigestSize);
   return ret;
}

//------------------------------------------------------------------------------

NetError PacketStream::sendto(Socket &outgoingSocket, const Address &addr)
{
   return outgoingSocket.sendto(addr, buffer, getBytePosition());
}

NetError PacketStream::recvfrom(Socket &incomingSocket, Address *recvAddress)
{
   NetError error;
   S32 dataSize;
   error = incomingSocket.recvfrom(recvAddress, buffer, sizeof(buffer), &dataSize);
   setBuffer(buffer, dataSize);
   setMaxSizes(dataSize, 0);
   reset();
   return error;
}

};