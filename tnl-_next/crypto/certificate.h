/// The Certificate class manages a digitally signed certificate.
/// Certificates consist of an application-defined payload, a public
/// key, and a signature.  It is up to the application to determine
/// from the payload what, if any, certificate authority (CA) signed
/// the certificate.  The validate() method can be used to check the
/// certificate's authenticity against the known public key of the
/// signing Certificate Authority.
///
/// The payload could include such items as:
///  - The valid date range for the certificate
///  - The domain of the certificate holder
///  - The identifying name of the Certificate Authority
///  - A player's name and user id for a multiplayer game
///
/// 
class Certificate : public Object
{
protected:
   ByteBufferPtr mCertificateData;        ///< The full certificate data
   RefPtr<AsymmetricKey> mPublicKey;      ///< The public key for the holder of this certificate
   ByteBufferPtr mPayload;                ///< The certificate payload, including the identity of the holder and the Certificate Authority
   ByteBufferPtr mSignature;              ///< The digital signature of this certificate by the signatory
   bool mIsValid;                         ///< flag to signify whether this certificate has a valid form
   U32 mSignatureByteSize;                ///< Number of bytes of the ByteBuffer signed by the CA
public:
   enum {
      MaxPayloadSize = 512,
   };
   /// Certificate constructor
   Certificate(U8 *dataPtr, U32 dataSize)
   {
      ByteBufferPtr theData = new ByteBuffer(dataPtr, dataSize); 
      setCertificateData(theData);
   }

   Certificate(ByteBufferPtr &buffer)
   {
      setCertificateData(buffer);
   }

   Certificate(BitStream *stream)
   {
      ByteBufferPtr theBuffer;
      read(*stream, &theBuffer);
      setCertificateData(theBuffer);
   }
	Certificate(const ByteBufferPtr payload, RefPtr<AsymmetricKey> publicKey, RefPtr<AsymmetricKey> theCAPrivateKey)
	{
	   mIsValid = false;
	   mSignatureByteSize = 0;

	   if(payload->getBufferSize() > MaxPayloadSize || !publicKey->isValid())
		  return;

	   ByteBufferPtr thePublicKey = publicKey->getPublicKey();
	   BitStream packet;

	   write(packet, payload);
	   write(packet, thePublicKey);
	   mSignatureByteSize = packet.getBytePosition();
	   packet.setBytePosition(mSignatureByteSize);

	   mSignature = theCAPrivateKey->hashAndSign(packet.getBuffer(), packet.getBytePosition());
	   write(packet, mSignature);

	   mCertificateData = new ByteBuffer(packet.getBuffer(), packet.getBytePosition());
	}


	/// Parses this certificate into the payload, public key, identiy, certificate authority and signature
    void parse()
	{
	   BitStream aStream(mCertificateData->getBuffer(), mCertificateData->getBufferSize());
	   read(aStream, &mPayload);
	   
	   mPublicKey = new AsymmetricKey(&aStream);

	   mSignatureByteSize = aStream.getBytePosition();
	   
	   // advance the bit stream to the next byte:
	   aStream.setBytePosition(aStream.getBytePosition());

	   read(aStream, &mSignature);

	   if(aStream.isValid() && mCertificateData->getBufferSize() == aStream.getBytePosition() && mPublicKey->isValid())
		  mIsValid = true;
	}

   /// Sets the full certificate from a ByteBuffer, and parses it.
   void setCertificateData(ByteBufferPtr &theBuffer)
   {
      mCertificateData = theBuffer;
      mSignatureByteSize = 0;
      mIsValid = false;
      parse();
   }

   /// Returns a ByteBufferPtr to the full certificate data
   ByteBufferPtr getCertificateData()
   {
      return mCertificateData;
   }
   /// Returns a ByteBufferPtr to the full certificate data
   const ByteBufferPtr getCertificateData() const
   {
      return mCertificateData;
   }

   /// returns the validity of the certificate's formation
   bool isValid()
   {
      return mIsValid;
   }
   /// returns true if this certificate was signed by the private key corresponding to the passed public key.
	bool validate(RefPtr<AsymmetricKey> signatoryPublicKey)
	{
	   if(!mIsValid)
		  return false;
	   
	   return signatoryPublicKey->verifySignature(mCertificateData->getBuffer(), mSignatureByteSize, *mSignature);
	}

   /// Returns the public key from the certificate
   RefPtr<AsymmetricKey> getPublicKey() { return mPublicKey; }

   /// Returns the certificate payload
   ByteBufferPtr getPayload() { return mPayload; }
};

inline void read(BitStream &str, Certificate *theCertificate)
{
   ByteBufferPtr theBuffer;
   read(str, &theBuffer);
   theCertificate->setCertificateData(theBuffer);
}

inline void write(BitStream &str, const Certificate &theCertificate)
{
   write(str, theCertificate.getCertificateData());
}

inline void read(BitStream &str, RefPtr<Certificate> *theCertificate)
{
   read(str, theCertificate->getPointer());
}

inline void write(BitStream &str, const RefPtr<Certificate> &theCertificate)
{
   write(str, *theCertificate.getPointer());
}
