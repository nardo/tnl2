/// Hashes the BitStream, writing the hash digest into the end of the buffer, and then encrypts with the given cipher
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
   theStream->writeBuffer(hashDigestSize, hash);

   theCipher->encrypt(theStream->getBuffer() + encryptStartOffset,
      theStream->getBuffer() + encryptStartOffset,
      theStream->getBytePosition() - encryptStartOffset);   
}


/// Decrypts the BitStream, then checks the hash digest at the end of the buffer to validate the contents
bool BitStreamDecryptAndCheckHash(BitStream *theStream, U32 hashDigestSize, U32 decryptStartOffset, SymmetricCipher *theCipher)
{
   U32 bufferSize = theStream->getStreamSize();
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
      theStream->setMaxSizes(bufferSize - hashDigestSize, 0);
   return ret;
}

/// PacketStream provides a network interface to the BitStream for easy construction of data packets.
class PacketStream : public BitStream
{
   typedef BitStream Parent;
   U8 buffer[NetSocket::MaxPacketDataSize]; ///< internal buffer for packet data, sized to the maximum UDP packet size.
public:
   /// Constructor assigns the internal buffer to the BitStream.
   PacketStream(U32 targetPacketSize = NetSocket::MaxPacketDataSize) : BitStream(buffer, targetPacketSize, NetSocket::MaxPacketDataSize) {}
   /// Sends this packet to the specified address through the specified socket.
   NetSocket::Status sendto(NetSocket &outgoingSocket, const NetAddress &theAddress)
	{
	   return outgoingSocket.sendto(addr, buffer, getBytePosition());
	}

   /// Reads a packet into the stream from the specified socket.
   NetSocket::Status recvfrom(NetSocket &incomingSocket, NetAddress *recvAddress)
	{
	   NetSocket::Status error;
	   S32 dataSize;
	   error = incomingSocket.recvfrom(recvAddress, buffer, sizeof(buffer), &dataSize);
	   setMaxSizes(dataSize, 0);
	   reset();
	   return error;
	}

};
