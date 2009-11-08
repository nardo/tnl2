#define crypto_key            ecc_key
#define crypto_make_key       ecc_make_key
#define crypto_free           ecc_free
#define crypto_import         ecc_import
#define crypto_export         ecc_export
#define crypto_shared_secret  ecc_shared_secret

class AsymmetricKey : public Object
{
enum {
   StaticCryptoBufferSize = 2048,
};

   /// Raw key data.
   ///
   /// The specific format of this buffer varies by cryptosystem, so look
   /// at the subclass to see how it's laid out. In general this is an
   /// opaque buffer.
   void *mKeyData;

   /// Size of the key at construct time.
   U32 mKeySize;

   /// Do we have the private key for this?
   ///
   /// We may only have access to the public key (for instance, when validating
   /// a message signed by someone else).
   bool mHasPrivateKey;

   /// Buffer containing the public half of this keypair.
   ByteBufferPtr mPublicKey;

   /// Buffer containing the private half of this keypair.
   ByteBufferPtr mPrivateKey;
   bool mIsValid;

   /// Load keypair from a buffer.
   void load(const U8 *bufferPtr, U32 bufferSize)
	{
		U8 staticCryptoBuffer[StaticCryptoBufferSize];
	   mIsValid = false;

	   crypto_key *theKey = (crypto_key *) MemoryAllocate(sizeof(crypto_key));
	   mHasPrivateKey = bufferPtr[0] == KeyTypePrivate;

	   if(bufferSize < sizeof(U32) + 1)
		  return;

	   mKeySize = readU32FromBuffer(bufferPtr + 1);

	   if( crypto_import(bufferPtr + sizeof(U32) + 1, bufferSize - sizeof(U32) - 1, theKey)
			 != CRYPT_OK)
		  return;

	   mKeyData = theKey;

	   if(mHasPrivateKey)
	   {
		  unsigned long bufferLen = sizeof(staticCryptoBuffer) - sizeof(U32) - 1;
		  staticCryptoBuffer[0] = KeyTypePublic;

		  writeU32ToBuffer(mKeySize, staticCryptoBuffer);

		  if( crypto_export(staticCryptoBuffer + sizeof(U32) + 1, &bufferLen, PK_PUBLIC, theKey)
				!= CRYPT_OK )
			 return;

		  bufferLen += sizeof(U32) + 1;

		  mPublicKey = new ByteBuffer(staticCryptoBuffer, bufferLen);
		  mPrivateKey = new ByteBuffer((U8 *) bufferPtr, bufferSize);
	   }
	   else
	   {
		  mPublicKey = new ByteBuffer((U8 *) bufferPtr, bufferSize);
	   }
	   mIsValid = true;
	}



   /// Enum used to indicate the portion of the key we are working with.
   enum KeyType {
      KeyTypePrivate,
      KeyTypePublic,
   };
public:

   /// Constructs an AsymmetricKey from the specified data pointer.
   AsymmetricKey(U8 *dataPtr, U32 bufferSize) : mKeyData(NULL)
   {
      load(dataPtr, bufferSize);
   }

   /// Constructs an AsymmetricKey from a ByteBuffer.
   AsymmetricKey(const ByteBuffer &theBuffer) : mKeyData(NULL)
   {
      load(theBuffer.getBuffer(), theBuffer.getBufferSize());
   }

   /// Constructs an AsymmetricKey by reading it from a BitStream.
   AsymmetricKey(BitStream *theStream)
   {
      ByteBufferPtr theBuffer;
      read(*theStream, &theBuffer);
      load(theBuffer->getBuffer(), theBuffer->getBufferSize());
   }

   /// Generates a new asymmetric key of keySize bytes
   AsymmetricKey(U32 keySize)
	{
		U8 staticCryptoBuffer[StaticCryptoBufferSize];
	   mIsValid = false;

	   int descriptorIndex = register_prng ( &yarrow_desc );
	   crypto_key *theKey = (crypto_key *) MemoryAllocate(sizeof(crypto_key));

	   if( crypto_make_key((prng_state *) Random::getState(), descriptorIndex,
		  keySize, theKey) != CRYPT_OK )
		  return;

	   mKeyData = theKey;
	   mKeySize = keySize;

	   unsigned long bufferLen = sizeof(staticCryptoBuffer) - sizeof(U32) - 1;

	   staticCryptoBuffer[0] = KeyTypePrivate;
	   writeU32ToBuffer(mKeySize, staticCryptoBuffer + 1);

	   crypto_export(staticCryptoBuffer + sizeof(U32) + 1, &bufferLen, PK_PRIVATE, theKey);
	   bufferLen += sizeof(U32) + 1;

	   mPrivateKey = new ByteBuffer(staticCryptoBuffer, bufferLen);

	   bufferLen = sizeof(staticCryptoBuffer) - sizeof(U32) - 1;

	   staticCryptoBuffer[0] = KeyTypePublic;
	   writeU32ToBuffer(mKeySize, staticCryptoBuffer + 1);

	   crypto_export(staticCryptoBuffer + sizeof(U32) + 1, &bufferLen, PK_PUBLIC, theKey);
	   bufferLen += sizeof(U32) + 1;

	   mPublicKey = new ByteBuffer(staticCryptoBuffer, bufferLen);

	   mHasPrivateKey = true;
	   mIsValid = true;
	}


   
   /// Destructor for the AsymmetricKey.
   ~AsymmetricKey()
	{
	   if(mKeyData)
	   {
		  crypto_free((crypto_key *) mKeyData);
		  MemoryDeallocate(mKeyData);
	   }
	}



   /// Returns a ByteBuffer containing an encoding of the public key.
   ByteBufferPtr getPublicKey() { return mPublicKey; }

   /// Returns a ByteBuffer containing an encoding of the private key.
   ByteBufferPtr getPrivateKey() { return mPrivateKey; }

   /// Returns true if this AsymmetricKey is a key pair.
   bool hasPrivateKey() { return mHasPrivateKey; }

   /// Returns true if this is a valid key.
   bool isValid() { return mIsValid; }
   /// Compute a key we can share with the specified AsymmetricKey
   /// for a symmetric crypto.
   ByteBufferPtr computeSharedSecretKey(AsymmetricKey *publicKey)
	{
	   if(publicKey->getKeySize() != getKeySize() || !mHasPrivateKey)
		  return NULL;

	   U8 hash[32];
	   unsigned long outLen = sizeof(staticCryptoBuffer);
		U8 staticCryptoBuffer[StaticCryptoBufferSize];

	   crypto_shared_secret((crypto_key *) mKeyData, (crypto_key *) publicKey->mKeyData,
		  staticCryptoBuffer, &outLen);

	   hash_state hashState;
	   sha256_init(&hashState);
	   sha256_process(&hashState, staticCryptoBuffer, outLen);
	   sha256_done(&hashState, hash);

	   return new ByteBuffer(hash, 32);
	}

   /// Returns the strength of the AsymmetricKey in byte size.
   U32 getKeySize() { return mKeySize; }

   /// Constructs a digital signature for the specified buffer of bits.  This
   /// method only works for private keys.  A public key only Asymmetric key
   /// will generate a signature of 0 bytes in length.
   ByteBufferPtr hashAndSign(const U8 *buffer, U32 bufferSize)
	{
	   int descriptorIndex = register_prng ( &yarrow_desc );

	   U8 hash[32];
	   hash_state hashState;
		U8 staticCryptoBuffer[StaticCryptoBufferSize];

	   sha256_init(&hashState);
	   sha256_process(&hashState, buffer, bufferSize);
	   sha256_done(&hashState, hash);

	   unsigned long outlen = sizeof(staticCryptoBuffer);

	   ecc_sign_hash(hash, 32,
		  staticCryptoBuffer, &outlen,
		  (prng_state *) Random::getState(), descriptorIndex, (crypto_key *) mKeyData);

	   return new ByteBuffer(staticCryptoBuffer, (U32) outlen);
	}


   /// Returns true if the private key associated with this AsymmetricKey 
   /// signed theByteBuffer with theSignature.
   bool verifySignature(const U8 *signedBytes, U32 signedByteSize, const ByteBuffer &theSignature)
	{
	   U8 hash[32];
	   hash_state hashState;

	   sha256_init(&hashState);
	   sha256_process(&hashState, signedBytes, signedBytesSize);
	   sha256_done(&hashState, hash);

	   int stat;

	   ecc_verify_hash(theSignature.getBuffer(), theSignature.getBufferSize(), hash, 32, &stat, (crypto_key *) mKeyData);
	   return stat != 0;
	}


};

typedef RefPtr<AsymmetricKey> AsymmetricKeyPtr;

