/// Encode the buffer to base 64, returning the encoded buffer.
RefPtr<ByteBuffer> BufferEncodeBase64(const U8 *buffer, U32 bufferSize)
{
   unsigned long outLen = ((bufferSize / 3) + 1) * 4 + 4 + 1;
   ByteBuffer *ret = new ByteBuffer(outLen);
   base64_encode(buffer, bufferSize, ret->getBuffer(), &outLen);
   ret->resize(outLen+1);
   ret->getBuffer()[outLen] = 0;
   return ret;
}

/// Encode the buffer to base 64, returning the encoded buffer.
inline RefPtr<ByteBuffer> BufferEncodeBase64(const ByteBuffer &buffer)
{
   return BufferEncodeBase64(buffer.getBuffer(), buffer.getBufferSize());
}

/// Decode the buffer from base 64, returning the decoded buffer.
RefPtr<ByteBuffer> BufferDecodeBase64(const U8 *buffer, U32 bufferSize)
{
   unsigned long outLen = bufferSize;
   ByteBuffer *ret = new ByteBuffer(outLen);
   base64_decode(buffer, bufferSize, ret->getBuffer(), &outLen);
   ret->resize(outLen);
   return ret;
}


/// Decode the buffer from base 64, returning the decoded buffer.
inline RefPtr<ByteBuffer> BufferDecodeBase64(const ByteBuffer &buffer)
{
   return BufferDecodeBase64(buffer.getBuffer(), buffer.getBufferSize());
}

/// Computes an MD5 hash and returns it in a ByteBuffer
RefPtr<ByteBuffer> BufferComputeMD5Hash(const U8 *buffer, U32 len)
{
   ByteBuffer *ret = new ByteBuffer(16);
   hash_state md;
   md5_init(&md);
   md5_process(&md, (unsigned char *) buffer, len);
   md5_done(&md, ret->getBuffer());
   return ret;
}

/// Converts to ascii-hex, returning the encoded buffer.
RefPtr<ByteBuffer> BufferEncodeBase16(const U8 *buffer, U32 len)
{
   U32 outLen = len * 2 + 1;
   ByteBuffer *ret = new ByteBuffer(outLen);
   U8 *outBuffer = ret->getBuffer();

   for(U32 i = 0; i < len; i++)
   {
      U8 b = *buffer++;
      U32 nib1 = b >> 4;
      U32 nib2 = b & 0xF;
      if(nib1 > 9)
         *outBuffer++ = 'a' + nib1 - 10;
      else
         *outBuffer++ = '0' + nib1;
      if(nib2 > 9)
         *outBuffer++ = 'a' + nib2 - 10;
      else
         *outBuffer++ = '0' + nib2;
   }
   *outBuffer = 0;
   return ret;
}

/// Decodes the buffer from base 16, returning the decoded buffer.
RefPtr<ByteBuffer> BufferDecodeBase16(const U8 *buffer, U32 len)
{
   U32 outLen = len >> 1;
   ByteBuffer *ret = new ByteBuffer(outLen);
   U8 *dst = ret->getBuffer();
   for(U32 i = 0; i < outLen; i++)
   {
      U8 out = 0;
      U8 nib1 = *buffer++;
      U8 nib2 = *buffer++;
      if(nib1 >= '0' && nib1 <= '9')
         out = (nib1 - '0') << 4;
      else if(nib1 >= 'a' && nib1 <= 'f')
         out = (nib1 - 'a' + 10) << 4;
      else if(nib1 >= 'A' && nib1 <= 'A')
         out = (nib1 - 'A' + 10) << 4;
      if(nib2 >= '0' && nib2 <= '9')
         out |= nib2 - '0';
      else if(nib2 >= 'a' && nib2 <= 'f')
         out |= nib2 - 'a' + 10;
      else if(nib2 >= 'A' && nib2 <= 'A')
         out |= nib2 - 'A' + 10;
      *dst++ = out;
   }
   return ret;
}


/// Returns a 32 bit CRC for the buffer.
U32 BufferCalculateCRC(const U8 *buffer, U32 len, U32 crcVal = 0xFFFFFFFF)
{
   static U32 crcTable[256];
   static bool crcTableValid = false;

   if(!crcTableValid)
   {
      U32 val;

      for(S32 i = 0; i < 256; i++)
      {
         val = i;
         for(S32 j = 0; j < 8; j++)
         {
            if(val & 0x01)
               val = 0xedb88320 ^ (val >> 1);
            else
               val = val >> 1;
         }
         crcTable[i] = val;
      }
      crcTableValid = true;
   }

   // now calculate the crc
   for(U32 i = 0; i < len; i++)
      crcVal = crcTable[(crcVal ^ buffer[i]) & 0xff] ^ (crcVal >> 8);
   return(crcVal);
}
