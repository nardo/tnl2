struct Nonce
{
   enum {
      NonceSize = 8,
   };
   U8 data[NonceSize];

   Nonce() {}
   Nonce(const U8 *ptr) { memcpy(data, ptr, NonceSize); }

   bool operator==(const Nonce &theOtherNonce) const { return !memcmp(data, theOtherNonce.data, NonceSize); }
   bool operator!=(const Nonce &theOtherNonce) const { return memcmp(data, theOtherNonce.data, NonceSize) != 0; }

   void operator=(const Nonce &theNonce) { memcpy(data, theNonce.data, NonceSize); }
   
   void getRandom() { Random::read(data, NonceSize); }
};

template<class S> inline void read(S &stream, Nonce *theNonce)
{ 
   stream.readBuffer(Nonce::NonceSize, theNonce->data);
}

template<class S> inline void write(S &stream, const Nonce &theNonce)
{
   stream.writeBuffer(Nonce::NonceSize, theNonce.data);
}
