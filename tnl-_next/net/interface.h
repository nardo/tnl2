/// NetInterface class.
///
/// Manages all valid and pending notify protocol connections for a port/IP. If you are
/// providing multiple services or servicing multiple networks, you may have more than
/// one NetInterface.
///
/// <b>Connection handshaking basic overview:</b>
///
/// TNL does a two phase connect handshake to prevent a several types of
/// Denial-of-Service (DoS) attacks.
///
/// The initiator of the connection (client) starts the connection by sending
/// a unique random nonce (number, used once) value to the server as part of
/// the ConnectChallengeRequest packet.
/// C->S: ConnectChallengeRequest, Nc
///
/// The server responds to the ConnectChallengeRequest with a "Client Puzzle"
/// that has the property that verifying a solution to the puzzle is computationally
/// simple, but can be of a specified computational, brute-force difficulty to
/// compute the solution itself.  The client puzzle is of the form:
/// secureHash(Ic, Nc, Ns, X) = Y >> k, where Ic is the identity of the client,
/// and X is a value computed by the client such that the high k bits of the value
/// y are all zero.  The client identity is computed by the server as a partial hash
/// of the client's IP address and port and some random data on the server.
/// its current nonce (Ns), Nc, k, and the server's authentication certificate.
/// S->C: ConnectChallengeResponse, Nc, Ns, Ic, Cs
///
/// The client, upon receipt of the ConnectChallengeResponse, validates the packet
/// sent by the server and computes a solution to the puzzle the server sent.  If
/// the connection is to be authenticated, the client can also validate the server's
/// certificate (if it's been signed by a Certificate Authority), and then generates
/// a shared secret from the client's key pair and the server's public key.  The client
/// response to the server consists of:
/// C->S: ConnectRequest, Nc, Ns, X, Cc, sharedSecret(key1, sequence1, NetConnectionClass, class-specific sendData)
///
/// The server then can validation the solution to the puzzle the client submitted, along
/// with the client identity (Ic).
/// Until this point the server has allocated no memory for the client and has
/// verified that the client is sending from a valid IP address, and that the client
/// has done some amount of work to prove its willingness to start a connection.
/// As the server load increases, the server may choose to increase the difficulty (k) of
/// the client puzzle, thereby making a resource depletion DoS attack successively more
/// difficult to launch.
///
/// If the server accepts the connection, it sends a connect accept packet that is
/// encrypted and hashed using the shared secret.  The contents of the packet are
/// another sequence number (sequence2) and another key (key2).  The sequence numbers 
/// are the initial send and receive sequence numbers for the connection, and the
/// key2 value becomes the IV of the symmetric cipher.  The connection subclass is
/// also allowed to write any connection specific data into this packet.
///
/// This system can operate in one of 3 ways: unencrypted, encrypted key exchange (ECDH),
/// or encrypted key exchange with server and/or client signed certificates (ECDSA).
/// 
/// The unencrypted communication mode is NOT secure.  Packets en route between hosts
/// can be modified without detection by the hosts at either end.  Connections using
/// the secure key exchange are still vulnerable to Man-in-the-middle attacks, but still
/// much more secure than the unencrypted channels.  Using certificate(s) signed by a
/// trusted certificate authority (CA), makes the communications channel as securely
/// trusted as the trust in the CA.
///
/// <b>Arranged Connection handshaking:</b>
///
/// NetInterface can also facilitate "arranged" connections.  Arranged connections are
/// necessary when both parties to the connection are behind firewalls or NAT routers.
/// Suppose there are two clients, A and B that want to esablish a direct connection with
/// one another.  If A and B are both logged into some common server S, then S can send
/// A and B the public (NAT'd) address, as well as the IP addresses each client detects
/// for itself.
///
/// A and B then both send "Punch" packets to the known possible addresses of each other.
/// The punch packet client A sends enables the punch packets client B sends to be 
/// delivered through the router or firewall since it will appear as though it is a service
/// response to A's initial packet.
///
/// Upon receipt of the Punch packet by the "initiator"
/// of the connection, an ArrangedConnectRequest packet is sent.
/// if the non-initiator of the connection gets an ArrangedPunch
/// packet, it simply sends another Punch packet to the
/// remote host, but narrows down its Address range to the address
/// it received the packet from.
/// The ArrangedPunch packet from the intiator contains the nonce 
/// for the non-initiator, and the nonce for the initiator encrypted
/// with the shared secret.
/// The ArrangedPunch packet for the receiver of the connection
/// contains all that, plus the public key/keysize or the certificate
/// of the receiver.


class NetInterface : public Object
{
   friend class NetConnection;
public:
   /// PacketType is encoded as the first byte of each packet.
   ///
   /// Subclasses of NetInterface can add custom, non-connected data
   /// packet types starting at FirstValidInfoPacketId, and overriding 
   /// handleInfoPacket to process them.
   ///
   /// Packets that arrive with the high bit of the first byte set
   /// (i.e. the first unsigned byte is greater than 127), are
   /// assumed to be connected protocol packets, and are dispatched to
   /// the appropriate connection for further processing.

   enum PacketType
   {
      ConnectChallengeRequest       = 0, ///< Initial packet of the two-phase connect process
      ConnectChallengeResponse      = 1, ///< Response packet to the ChallengeRequest containing client identity, a client puzzle, and possibly the server's public key.
      ConnectRequest                = 2, ///< A connect request packet, including all puzzle solution data and connection initiation data.
      ConnectReject                 = 3, ///< A packet sent to notify a host that a ConnectRequest was rejected.
      ConnectAccept                 = 4, ///< A packet sent to notify a host that a connection was accepted.
      Disconnect                    = 5, ///< A packet sent to notify a host that the specified connection has terminated.
      Punch                         = 6, ///< A packet sent in order to create a hole in a firewall or NAT so packets from the remote host can be received.
      ArrangedConnectRequest        = 7, ///< A connection request for an "arranged" connection.
      FirstValidInfoPacketId        = 8, ///< The first valid ID for a NetInterface subclass's info packets.
   };

protected:
   Array<RefPtr<NetConnection> > mConnectionList;      ///< List of all the connections that are in a connected state on this NetInterface.
   Array<NetConnection *> mConnectionHashTable; ///< A resizable hash table for all connected connections.  This is a flat hash table (no buckets).

   Array<RefPtr<NetConnection> > mPendingConnections; ///< List of connections that are in the startup state, where the remote host has not fully
                                                ///  validated the connection.

   RefPtr<AsymmetricKey> mPrivateKey;  ///< The private key used by this NetInterface for secure key exchange.
   RefPtr<Certificate> mCertificate;   ///< A certificate, signed by some Certificate Authority, to authenticate this host.
   ClientPuzzleManager mPuzzleManager; ///< The object that tracks the current client puzzle difficulty, current puzzle and solutions for this NetInterface.

   /// @name NetInterfaceSocket Socket
   ///
   /// State regarding the socket this NetInterface controls.
   ///
   /// @{

   ///
   NetSocket    mSocket;   ///< Network socket this NetInterface communicates over.

   /// @}

   Time mProcessStartTime;      ///< Current time tracked by this NetInterface.
   bool mRequiresKeyExchange;   ///< True if all connections outgoing and incoming require key exchange.
   Time mLastTimeoutCheckTime;  ///< Last time all the active connections were checked for timeouts.
   U8  mRandomHashData[12];    ///< Data that gets hashed with connect challenge requests to prevent connection spoofing.
   bool mAllowConnections;      ///< Set if this NetInterface allows connections from remote instances.

   /// Structure used to track packets that are delayed in sending for simulating a high-latency connection.
   ///
   /// The DelaySendPacket is allocated as sizeof(DelaySendPacket) + packetSize;
   struct DelaySendPacket
   {
      DelaySendPacket *nextPacket; ///< The next packet in the list of delayed packets.
      NetAddress remoteAddress;    ///< The address to send this packet to.
      Time sendTime;                ///< Time when we should send the packet.
      U32 packetSize;              ///< Size, in bytes, of the packet data.
      U8 packetData[1];            ///< Packet data.
   };
   DelaySendPacket *mSendPacketList; ///< List of delayed packets pending to send.


   enum NetInterfaceConstants {
      ChallengeRetryCount = 4,     ///< Number of times to send connect challenge requests before giving up.
      ChallengeRetryTime = 2500,   ///< Timeout interval in milliseconds before retrying connect challenge.

      ConnectRetryCount = 4,       ///< Number of times to send connect requests before giving up.
      ConnectRetryTime = 2500,     ///< Timeout interval in milliseconds before retrying connect request.

      PunchRetryCount = 6,         ///< Number of times to send groups of firewall punch packets before giving up.
      PunchRetryTime = 2500,       ///< Timeout interval in milliseconds before retrying punch sends.

      TimeoutCheckInterval = 1500, ///< Interval in milliseconds between checking for connection timeouts.
      PuzzleSolutionTimeout = 30000, ///< If the server gives us a puzzle that takes more than 30 seconds, time out.
   };

   /// Computes an identity token for the connecting client based on the address of the client and the
   /// client's unique nonce value.
   U32 computeClientIdentityToken(const NetAddress &theAddress, const Nonce &theNonce)
	{
	   hash_state hashState;
	   U32 hash[8];

	   sha256_init(&hashState);
	   sha256_process(&hashState, (const U8 *) &address, sizeof(NetAddress));
	   sha256_process(&hashState, theNonce.data, Nonce::NonceSize);
	   sha256_process(&hashState, mRandomHashData, sizeof(mRandomHashData));
	   sha256_done(&hashState, (U8 *) hash);

	   return hash[0];
	}


   /// Finds a connection instance that this NetInterface has initiated.
   NetConnection *findPendingConnection(const NetAddress &address)
	{
	   // Loop through all the pending connections and compare the NetAddresses
	   for(U32 i = 0; i < mPendingConnections.size(); i++)
		  if(address == mPendingConnections[i]->getNetAddress())
			 return mPendingConnections[i];
	   return NULL;
	}

   /// Adds a connection the list of pending connections.
   void addPendingConnection(NetConnection *conn)
	{
	   // make sure we're not already connected to the host at the
	   // connection's NetAddress
	   findAndRemovePendingConnection(connection->getNetAddress());
	   NetConnection *temp = findConnection(connection->getNetAddress());
	   if(temp)
		  disconnect(temp, NetConnection::ReasonSelfDisconnect, "Reconnecting");

	   // hang on to the connection and add it to the pending connection list
	   mPendingConnections.pushBack(connection);
	}

   /// Removes a connection from the list of pending connections.
   void removePendingConnection(NetConnection *conn)
	{
	   // search the pending connection list for the specified connection
	   // and remove it.
	   for(U32 i = 0; i < mPendingConnections.size(); i++)
		  if(mPendingConnections[i] == connection)
		  {
			 mPendingConnections.erase(i);
			 return;
		  }
	}


   /// Finds a connection by address from the pending list and removes it.
   void findAndRemovePendingConnection(const NetAddress &address)
	{
	   // Search through the list by NetAddress and remove any connection
	   // that matches.
	   for(U32 i = 0; i < mPendingConnections.size(); i++)
		  if(address == mPendingConnections[i]->getNetAddress())
		  {
			 mPendingConnections.erase(i);
			 return;
		  }
	}

   /// Adds a connection to the internal connection list.
   void addConnection(NetConnection *connection)
	{
	   mConnectionList.pushBack(conn);
	   U32 numConnections = mConnectionList.size();
	   if(numConnections > mConnectionHashTable.size() / 2)
	   {
		  mConnectionHashTable.resize(numConnections * 4 - 1);
		  for(U32 i = 0; i < mConnectionHashTable.size(); i++)
			 mConnectionHashTable[i] = NULL;
		  for(U32 i = 0; i < numConnections; i++)
		  {
			 U32 index = mConnectionList[i]->getNetAddress().hash() % mConnectionHashTable.size();
			 while(mConnectionHashTable[index] != NULL)
			 {
				index++;
				if(index >= (U32) mConnectionHashTable.size())
				   index = 0;
			 }
			 mConnectionHashTable[index] = mConnectionList[i];
		  }
	   }
	   else
	   {
		  U32 index = mConnectionList[numConnections - 1]->getNetAddress().hash() % mConnectionHashTable.size();
		  while(mConnectionHashTable[index] != NULL)
		  {
			 index++;
			 if(index >= (U32) mConnectionHashTable.size())
				index = 0;
		  }
		  mConnectionHashTable[index] = mConnectionList[numConnections - 1];
	   }
	}


   /// Remove a connection from the list.
   void removeConnection(NetConnection *connection)
	{
	   // hold the reference to the object until the function exits.
	   RefPtr<NetConnection> hold = conn;
	   for(U32 i = 0; i < mConnectionList.size(); i++)
	   {
		  if(mConnectionList[i] == conn)
		  {
			 mConnectionList.eraseUnstable(i);
			 break;
		  }
	   }
	   U32 index = conn->getNetAddress().hash() % mConnectionHashTable.size();
	   U32 startIndex = index;

	   while(mConnectionHashTable[index] != conn)
	   {
		  index++;
		  if(index >= (U32) mConnectionHashTable.size())
			 index = 0;
		  Assert(index != startIndex, "Attempting to remove a connection that is not in the table."); // not in the table
		  if(index == startIndex)
			 return;
	   }
	   mConnectionHashTable[index] = NULL;

	   // rehash all subsequent entries until we find a NULL entry:
	   for(;;)
	   {
		  index++;
		  if(index >= (U32) mConnectionHashTable.size())
			 index = 0;
		  if(!mConnectionHashTable[index])
			 break;
		  NetConnection *rehashConn = mConnectionHashTable[index];
		  mConnectionHashTable[index] = NULL;
		  U32 realIndex = rehashConn->getNetAddress().hash() % mConnectionHashTable.size();
		  while(mConnectionHashTable[realIndex] != NULL)
		  {
			 realIndex++;
			 if(realIndex >= (U32) mConnectionHashTable.size())
				realIndex = 0;
		  }
		  mConnectionHashTable[realIndex] = rehashConn;
	   }
	}


   /// Begins the connection handshaking process for a connection.  Called from NetConnection::connect()
   void startConnection(NetConnection *conn)
	{
	   Assert(conn->getConnectionState() == NetConnection::NotConnected,
			 "Cannot start unless it is in the NotConnected state.");

	   addPendingConnection(conn);
	   conn->mConnectSendCount = 0;
	   conn->setConnectionState(NetConnection::AwaitingChallengeResponse);
	   sendConnectChallengeRequest(conn);
	}

   /// Sends a connect challenge request on behalf of the connection to the remote host.
   void sendConnectChallengeRequest(NetConnection *conn)
	{
	   TorqueLogMessageFormatted(LogNetInterface, ("Sending Connect Challenge Request to %s", conn->getNetAddress().toString()));
	   PacketStream out;
	   write(out, U8(ConnectChallengeRequest));
	   ConnectionParameters &params = conn->getConnectionParameters();
	   write(out, params.mNonce);
	   out.writeFlag(params.mRequestKeyExchange);
	   out.writeFlag(params.mRequestCertificate);
	 
	   conn->mConnectSendCount++;
	   conn->mConnectLastSendTime = getProcessStartTime();
	   out.sendto(mSocket, conn->getNetAddress());
	}


   /// Handles a connect challenge request by replying to the requestor of a connection with a
   /// unique token for that connection, as well as (possibly) a client puzzle (for DoS prevention),
   /// or this NetInterface's public key.
   void handleConnectChallengeRequest(const NetAddress &addr, BitStream *stream)
	{
	   TorqueLogMessageFormatted(LogNetInterface, ("Received Connect Challenge Request from %s", addr.toString()));

	   if(!mAllowConnections)
		  return;
	   Nonce clientNonce;
	   read(*stream, &clientNonce);
	   bool wantsKeyExchange = stream->readFlag();
	   bool wantsCertificate = stream->readFlag();

	   sendConnectChallengeResponse(addr, clientNonce, wantsKeyExchange, wantsCertificate);
	}


   /// Sends a connect challenge request to the specified address.  This can happen as a result
   /// of receiving a connect challenge request, or during an "arranged" connection for the non-initiator
   /// of the connection.
   void sendConnectChallengeResponse(const NetAddress &addr, Nonce &clientNonce, bool wantsKeyExchange, bool wantsCertificate)
	{
	   PacketStream out;
	   write(out, U8(ConnectChallengeResponse));
	   write(out, clientNonce);
	   
	   U32 identityToken = computeClientIdentityToken(addr, clientNonce);
	   write(out, identityToken);

	   // write out a client puzzle
	   Nonce serverNonce = mPuzzleManager.getCurrentNonce();
	   U32 difficulty = mPuzzleManager.getCurrentDifficulty();
	   write(out, serverNonce);
	   write(out, difficulty);

	   if(out.writeFlag(mRequiresKeyExchange || (wantsKeyExchange && !mPrivateKey.isNull())))
	   {
		  if(out.writeFlag(wantsCertificate && !mCertificate.isNull()))
			 write(out, mCertificate);
		  else
			 write(out, mPrivateKey->getPublicKey());
	   }
	   TorqueLogMessageFormatted(LogNetInterface, ("Sending Challenge Response: %8x", identityToken));

	   out.sendto(mSocket, addr);
	}


   /// Processes a ConnectChallengeResponse, by issueing a connect request if it was for
   /// a connection this NetInterface has pending.
   void handleConnectChallengeResponse(const NetAddress &address, BitStream *stream)
	{
	   NetConnection *conn = findPendingConnection(address);
	   if(!conn || conn->getConnectionState() != NetConnection::AwaitingChallengeResponse)
		  return;
	   
	   Nonce theNonce;
	   read(*stream, &theNonce);

	   ConnectionParameters &theParams = conn->getConnectionParameters();
	   if(theNonce != theParams.mNonce)
		  return;

	   read(*stream, &theParams.mClientIdentity);

	   // see if the server wants us to solve a client puzzle
	   read(*stream, &theParams.mServerNonce);
	   read(*stream, &theParams.mPuzzleDifficulty);

	   if(theParams.mPuzzleDifficulty > ClientPuzzleManager::MaxPuzzleDifficulty)
		  return;

	   // see if the connection needs to be authenticated or uses key exchange
	   if(stream->readFlag())
	   {
		  if(stream->readFlag())
		  {
			 theParams.mCertificate = new Certificate(stream);
			 if(!theParams.mCertificate->isValid() || !conn->validateCertficate(theParams.mCertificate, true))
				return;         
			 theParams.mPublicKey = theParams.mCertificate->getPublicKey();
		  }
		  else
		  {
			 theParams.mPublicKey = new AsymmetricKey(stream);
			 if(!theParams.mPublicKey->isValid() || !conn->validatePublicKey(theParams.mPublicKey, true))
				return;
		  }
		  if(mPrivateKey.isNull() || mPrivateKey->getKeySize() != theParams.mPublicKey->getKeySize())
		  {
			 // we don't have a private key, so generate one for this connection
			 theParams.mPrivateKey = new AsymmetricKey(theParams.mPublicKey->getKeySize());
		  }
		  else
			 theParams.mPrivateKey = mPrivateKey;
		  theParams.mSharedSecret = theParams.mPrivateKey->computeSharedSecretKey(theParams.mPublicKey);
		  //logprintf("shared secret (client) %s", theParams.mSharedSecret->encodeBase64()->getBuffer());
		  Random::read(theParams.mSymmetricKey, SymmetricCipher::KeySize);
		  theParams.mUsingCrypto = true;
	   }

	   TorqueLogMessageFormatted(LogNetInterface, ("Received Challenge Response: %8x", theParams.mClientIdentity ));

	   conn->setConnectionState(NetConnection::ComputingPuzzleSolution);
	   conn->mConnectSendCount = 0;

	   theParams.mPuzzleSolution = 0;
	   conn->mConnectLastSendTime = getProcessStartTime();
	   continuePuzzleSolution(conn);   
	}


   /// Continues computation of the solution of a client puzzle, and issues a connect request
   /// when the solution is found.
   void continuePuzzleSolution(NetConnection *conn)
	{
	   ConnectionParameters &theParams = conn->getConnectionParameters();
	   bool solved = ClientPuzzleManager::solvePuzzle(&theParams.mPuzzleSolution, theParams.mNonce, theParams.mServerNonce, theParams.mPuzzleDifficulty, theParams.mClientIdentity);

	   if(solved)
	   {
		  logprintf("Client puzzle solved in %d ms.", GetTime() - conn->mConnectLastSendTime);
		  conn->setConnectionState(NetConnection::AwaitingConnectResponse);
		  sendConnectRequest(conn);
	   }
	}

   /// Sends a connect request on behalf of a pending connection.
   void sendConnectRequest(NetConnection *conn)
	{
	   TorqueLogMessageFormatted(LogNetInterface, ("Sending Connect Request"));
	   PacketStream out;
	   ConnectionParameters &theParams = conn->getConnectionParameters();

	   write(out, U8(ConnectRequest));
	   write(out, theParams.mNonce);
	   write(out, theParams.mServerNonce);
	   write(out, theParams.mClientIdentity);
	   write(out, theParams.mPuzzleDifficulty);
	   write(out, theParams.mPuzzleSolution);

	   U32 encryptPos = 0;

	   if(out.writeFlag(theParams.mUsingCrypto))
	   {
		  write(out, theParams.mPrivateKey->getPublicKey());
		  encryptPos = out.getBytePosition();
		  out.setBytePosition(encryptPos);
		  out.writeBuffer(SymmetricCipher::KeySize, theParams.mSymmetricKey);
	   }
	   out.writeFlag(theParams.mDebugObjectSizes);
	   write(out, conn->getInitialSendSequence());
	   out.writeString(conn->getClassName());
	   conn->writeConnectRequest(&out);

	   if(encryptPos)
	   {
		  // if we're using crypto on this connection,
		  // then write a hash of everything we wrote into the packet
		  // key.  Then we'll symmetrically encrypt the packet from
		  // the end of the public key to the end of the signature.

		  SymmetricCipher theCipher(theParams.mSharedSecret);
		  BitStreamHashAndEncrypt(&out, NetConnection::MessageSignatureBytes, encryptPos, &theCipher);      
	   }

	   conn->mConnectSendCount++;
	   conn->mConnectLastSendTime = getProcessStartTime();

	   out.sendto(mSocket, conn->getNetAddress());
	}


   /// Handles a connection request from a remote host.
   ///
   /// This will verify the validity of the connection token, as well as any solution
   /// to a client puzzle this NetInterface sent to the remote host.  If those tests
   /// pass, it will construct a local connection instance to handle the rest of the
   /// connection negotiation.
   void handleConnectRequest(const NetAddress &address, BitStream *stream)
	{
	   if(!mAllowConnections)
		  return;

	   ConnectionParameters theParams;
	   read(*stream, &theParams.mNonce);
	   read(*stream, &theParams.mServerNonce);
	   read(*stream, &theParams.mClientIdentity);

	   if(theParams.mClientIdentity != computeClientIdentityToken(address, theParams.mNonce))
		  return;

	   read(*stream, &theParams.mPuzzleDifficulty);
	   read(*stream, &theParams.mPuzzleSolution);

	   // see if the connection is in the main connection table.
	   // If the connection is in the connection table and it has
	   // the same initiatorSequence, we'll just resend the connect
	   // acceptance packet, assuming that the last time we sent it
	   // it was dropped.
	   NetConnection *connect = findConnection(address);
	   if(connect)
	   {
		  ConnectionParameters &cp = connect->getConnectionParameters();
		  if(cp.mNonce == theParams.mNonce && cp.mServerNonce == theParams.mServerNonce)
		  {
			 sendConnectAccept(connect);
			 return;
		  }
	   }

	   // check the puzzle solution
	   ClientPuzzleManager::ErrorCode result = mPuzzleManager.checkSolution(
		  theParams.mPuzzleSolution, theParams.mNonce, theParams.mServerNonce,
		  theParams.mPuzzleDifficulty, theParams.mClientIdentity);

	   if(result != ClientPuzzleManager::Success)
	   {
		  sendConnectReject(&theParams, address, "Puzzle");
		  return;
	   }

	   if(stream->readFlag())
	   {
		  if(mPrivateKey.isNull())
			 return;

		  theParams.mUsingCrypto = true;
		  theParams.mPublicKey = new AsymmetricKey(stream);
		  theParams.mPrivateKey = mPrivateKey;

		  U32 decryptPos = stream->getBytePosition();

		  stream->setBytePosition(decryptPos);
		  theParams.mSharedSecret = theParams.mPrivateKey->computeSharedSecretKey(theParams.mPublicKey);
		  //logprintf("shared secret (server) %s", theParams.mSharedSecret->encodeBase64()->getBuffer());

		  SymmetricCipher theCipher(theParams.mSharedSecret);

		  if(!BitStreamDecryptAndCheckHash(stream, NetConnection::MessageSignatureBytes, decryptPos, &theCipher))
			 return;

		  // now read the first part of the connection's symmetric key
		  stream->readBuffer(SymmetricCipher::KeySize, theParams.mSymmetricKey);
		  Random::read(theParams.mInitVector, SymmetricCipher::KeySize);
	   }

	   U32 connectSequence;
	   theParams.mDebugObjectSizes = stream->readFlag();
	   read(*stream, &connectSequence);
	   TorqueLogMessageFormatted(LogNetInterface, ("Received Connect Request %8x", theParams.mClientIdentity));

	   if(connect)
		  disconnect(connect, NetConnection::ReasonSelfDisconnect, "NewConnection");

	   char connectionClass[256];
	   stream->readString(connectionClass);

	   NetConnection *conn = NetConnectionRep::create(connectionClass);

	   if(!conn)
		  return;

	   RefPtr<NetConnection> theConnection = conn;
	   conn->getConnectionParameters() = theParams;

	   conn->setNetAddress(address);
	   conn->setInitialRecvSequence(connectSequence);
	   conn->setInterface(this);

	   if(theParams.mUsingCrypto)
		  conn->setSymmetricCipher(new SymmetricCipher(theParams.mSymmetricKey, theParams.mInitVector));

	   const char *errorString = NULL;
	   if(!conn->readConnectRequest(stream, &errorString))
	   {
		  sendConnectReject(&theParams, address, errorString);
		  return;
	   }
	   addConnection(conn);
	   conn->setConnectionState(NetConnection::Connected);
	   conn->onConnectionEstablished();
	   sendConnectAccept(conn);
	}


   /// Sends a connect accept packet to acknowledge the successful acceptance of a connect request.
   void sendConnectAccept(NetConnection *conn)
	{
	   TorqueLogMessageFormatted(LogNetInterface, ("Sending Connect Accept - connection established."));

	   PacketStream out;
	   write(out, U8(ConnectAccept));
	   ConnectionParameters &theParams = conn->getConnectionParameters();

	   write(out, theParams.mNonce);
	   write(out, theParams.mServerNonce);
	   U32 encryptPos = out.getBytePosition();
	   out.setBytePosition(encryptPos);

	   write(out, conn->getInitialSendSequence());
	   conn->writeConnectAccept(&out);

	   if(theParams.mUsingCrypto)
	   {
		  out.writeBuffer(SymmetricCipher::KeySize, theParams.mInitVector);
		  SymmetricCipher theCipher(theParams.mSharedSecret);
		  BitStreamHashAndEncrypt(&out, NetConnection::MessageSignatureBytes, encryptPos, &theCipher);
	   }
	   out.sendto(mSocket, conn->getNetAddress());
	}

   /// Handles a connect accept packet, putting the connection associated with the
   /// remote host (if there is one) into an active state.
   void handleConnectAccept(const NetAddress &address, BitStream *stream)
	{
	   Nonce nonce, serverNonce;

	   read(*stream, &nonce);
	   read(*stream, &serverNonce);
	   U32 decryptPos = stream->getBytePosition();
	   stream->setBytePosition(decryptPos);

	   NetConnection *conn = findPendingConnection(address);
	   if(!conn || conn->getConnectionState() != NetConnection::AwaitingConnectResponse)
		  return;

	   ConnectionParameters &theParams = conn->getConnectionParameters();

	   if(theParams.mNonce != nonce || theParams.mServerNonce != serverNonce)
		  return;

	   if(theParams.mUsingCrypto)
	   {
		  SymmetricCipher theCipher(theParams.mSharedSecret);
		  if(!BitStreamDecryptAndCheckHash(stream, NetConnection::MessageSignatureBytes, decryptPos, &theCipher))
			 return;
	   }
	   U32 recvSequence;
	   read(*stream, &recvSequence);
	   conn->setInitialRecvSequence(recvSequence);

	   const char *errorString = NULL;
	   if(!conn->readConnectAccept(stream, &errorString))
	   {
		  removePendingConnection(conn);
		  return;
	   }
	   if(theParams.mUsingCrypto)
	   {
		  stream->readBuffer(SymmetricCipher::KeySize, theParams.mInitVector);
		  conn->setSymmetricCipher(new SymmetricCipher(theParams.mSymmetricKey, theParams.mInitVector));
	   }

	   addConnection(conn); // first, add it as a regular connection
	   removePendingConnection(conn); // remove from the pending connection list

	   conn->setConnectionState(NetConnection::Connected);
	   conn->onConnectionEstablished(); // notify the connection that it has been established
	   TorqueLogMessageFormatted(LogNetInterface, ("Received Connect Accept - connection established."));
	}


   /// Sends a connect rejection to a valid connect request in response to possible error
   /// conditions (server full, wrong password, etc).
   void sendConnectReject(ConnectionParameters *theParams, const NetAddress &theAddress, const String &reason)
	{
	   if(reason.isEmpty())
		  return; // if the reason is "", we reject silently

	   PacketStream out;
	   write(out, U8(ConnectReject));
	   write(out, conn->mNonce);
	   write(out, conn->mServerNonce);
	   out.writeString(reason.c_str());
	   out.sendto(mSocket, theAddress);
	}


   /// Handles a connect rejection packet by notifying the connection object
   /// that the connection was rejected.
   void handleConnectReject(const NetAddress &address, BitStream *stream)
	{
	   Nonce nonce;
	   Nonce serverNonce;

	   read(*stream, &nonce);
	   read(*stream, &serverNonce);

	   NetConnection *conn = findPendingConnection(address);
	   if(!conn || (conn->getConnectionState() != NetConnection::AwaitingChallengeResponse &&
					conn->getConnectionState() != NetConnection::AwaitingConnectResponse))
		  return;
	   ConnectionParameters &p = conn->getConnectionParameters();
	   if(p.mNonce != nonce || p.mServerNonce != serverNonce)
		  return;

	   char reason[256];
	   stream->readString(reason);

	   TorqueLogMessageFormatted(LogNetInterface, ("Received Connect Reject - reason %s", reason));
	   // if the reason is a bad puzzle solution, try once more with a
	   // new nonce.
	   if(!strcmp(reason, "Puzzle") && !p.mPuzzleRetried)
	   {
		  p.mPuzzleRetried = true;
		  conn->setConnectionState(NetConnection::AwaitingChallengeResponse);
		  conn->mConnectSendCount = 0;
		  p.mNonce.getRandom();
		  sendConnectChallengeRequest(conn);
		  return;
	   }

	   conn->setConnectionState(NetConnection::ConnectRejected);
	   conn->onConnectTerminated(NetConnection::ReasonRemoteHostRejectedConnection, reason);
	   removePendingConnection(conn);
	}


   /// Begins the connection handshaking process for an arranged connection.
   void startArrangedConnection(NetConnection *conn)
	{

	   conn->setConnectionState(NetConnection::SendingPunchPackets);
	   addPendingConnection(conn);
	   conn->mConnectSendCount = 0;
	   conn->mConnectLastSendTime = getProcessStartTime();
	   sendPunchPackets(conn);
	}

   /// Sends Punch packets to each address in the possible connection address list.
   void sendPunchPackets(NetConnection *conn)
	{
	   ConnectionParameters &theParams = conn->getConnectionParameters();
	   PacketStream out;
	   write(out, U8(Punch));

	   if(theParams.mIsInitiator)
		  write(out, theParams.mNonce);
	   else
		  write(out, theParams.mServerNonce);

	   U32 encryptPos = out.getBytePosition();
	   out.setBytePosition(encryptPos);

	   if(theParams.mIsInitiator)
		  write(out, theParams.mServerNonce);
	   else
	   {
		  write(out, theParams.mNonce);
		  if(out.writeFlag(mRequiresKeyExchange || (theParams.mRequestKeyExchange && !mPrivateKey.isNull())))
		  {
			 if(out.writeFlag(theParams.mRequestCertificate && !mCertificate.isNull()))
				write(out, mCertificate);
			 else
				write(out, mPrivateKey->getPublicKey());
		  }
	   }
	   SymmetricCipher theCipher(theParams.mArrangedSecret);
	   BitStreamHashAndEncrypt(&out, NetConnection::MessageSignatureBytes, encryptPos, &theCipher);

	   for(U32 i = 0; i < theParams.mPossibleAddresses.size(); i++)
	   {
		  out.sendto(mSocket, theParams.mPossibleAddresses[i]);

		  TorqueLogMessageFormatted(LogNetInterface, ("Sending punch packet (%s, %s) to %s",
			 BufferEncodeBase64(ByteBuffer(theParams.mNonce.data, Nonce::NonceSize))->getBuffer(),
			 BufferEncodeBase64(ByteBuffer(theParams.mServerNonce.data, Nonce::NonceSize))->getBuffer(),
			 theParams.mPossibleAddresses[i].toString()));
	   }
	   conn->mConnectSendCount++;
	   conn->mConnectLastSendTime = getProcessStartTime();
	}


   /// Handles an incoming Punch packet from a remote host.
   void handlePunch(const NetAddress &theAddress, BitStream *stream)
	{
	   U32 i, j;
	   NetConnection *conn;

	   Nonce firstNonce;
	   read(*stream, &firstNonce);

	   ByteBuffer b(firstNonce.data, Nonce::NonceSize);

	   TorqueLogMessageFormatted(LogNetInterface, ("Received punch packet from %s - %s", theAddress.toString(), BufferEncodeBase64(b)->getBuffer()));

	   for(i = 0; i < mPendingConnections.size(); i++)
	   {
		  conn = mPendingConnections[i];
		  ConnectionParameters &theParams = conn->getConnectionParameters();

		  if(conn->getConnectionState() != NetConnection::SendingPunchPackets)
			 continue;

		  if((theParams.mIsInitiator && firstNonce != theParams.mServerNonce) ||
				(!theParams.mIsInitiator && firstNonce != theParams.mNonce))
			 continue;

		  // first see if the address is in the possible addresses list:
		  
		  for(j = 0; j < theParams.mPossibleAddresses.size(); j++)
			 if(theAddress == theParams.mPossibleAddresses[j])
				break;

		  // if there was an exact match, just exit the loop, or
		  // continue on to the next pending if this is not an initiator:
		  if(j != theParams.mPossibleAddresses.size())
		  {
			 if(theParams.mIsInitiator)
				break;
			 else
				continue;
		  }

		  // if there was no exact match, we may have a funny NAT in the
		  // middle.  But since a packet got through from the remote host
		  // we'll want to send a punch to the address it came from, as long
		  // as only the port is not an exact match:
		  for(j = 0; j < theParams.mPossibleAddresses.size(); j++)
			 if(theAddress.isEqualAddress(theParams.mPossibleAddresses[j]))
				break;

		  // if the address wasn't even partially in the list, just exit out
		  if(j == theParams.mPossibleAddresses.size())
			 continue;

		  // otherwise, as long as we don't have too many ping addresses,
		  // add this one to the list:
		  if(theParams.mPossibleAddresses.size() < 5)
			 theParams.mPossibleAddresses.pushBack(theAddress);      

		  // if this is the initiator of the arranged connection, then
		  // process the punch packet from the remote host by issueing a
		  // connection request.
		  if(theParams.mIsInitiator)
			 break;
	   }
	   if(i == mPendingConnections.size())
		  return;

	   ConnectionParameters &theParams = conn->getConnectionParameters();
	   SymmetricCipher theCipher(theParams.mArrangedSecret);
	   if(!BitStreamDecryptAndCheckHash(stream, NetConnection::MessageSignatureBytes, stream->getBytePosition(), &theCipher))
		  return;

	   Nonce nextNonce;
	   read(*stream, &nextNonce);

	   if(nextNonce != theParams.mNonce)
		  return;

	   // see if the connection needs to be authenticated or uses key exchange
	   if(stream->readFlag())
	   {
		  if(stream->readFlag())
		  {
			 theParams.mCertificate = new Certificate(stream);
			 if(!theParams.mCertificate->isValid() || !conn->validateCertficate(theParams.mCertificate, true))
				return;         
			 theParams.mPublicKey = theParams.mCertificate->getPublicKey();
		  }
		  else
		  {
			 theParams.mPublicKey = new AsymmetricKey(stream);
			 if(!theParams.mPublicKey->isValid() || !conn->validatePublicKey(theParams.mPublicKey, true))
				return;
		  }
		  if(mPrivateKey.isNull() || mPrivateKey->getKeySize() != theParams.mPublicKey->getKeySize())
		  {
			 // we don't have a private key, so generate one for this connection
			 theParams.mPrivateKey = new AsymmetricKey(theParams.mPublicKey->getKeySize());
		  }
		  else
			 theParams.mPrivateKey = mPrivateKey;
		  theParams.mSharedSecret = theParams.mPrivateKey->computeSharedSecretKey(theParams.mPublicKey);
		  //logprintf("shared secret (client) %s", theParams.mSharedSecret->encodeBase64()->getBuffer());
		  Random::read(theParams.mSymmetricKey, SymmetricCipher::KeySize);
		  theParams.mUsingCrypto = true;
	   }
	   conn->setNetAddress(theAddress);
	   TorqueLogMessageFormatted(LogNetInterface, ("Punch from %s matched nonces - connecting...", theAddress.toString()));

	   conn->setConnectionState(NetConnection::AwaitingConnectResponse);
	   conn->mConnectSendCount = 0;
	   conn->mConnectLastSendTime = getProcessStartTime();

	   sendArrangedConnectRequest(conn);
	}

   /// Sends an arranged connect request.
   void sendArrangedConnectRequest(NetConnection *conn)
	{
	   TorqueLogMessageFormatted(LogNetInterface, ("Sending Arranged Connect Request"));
	   PacketStream out;

	   ConnectionParameters &theParams = conn->getConnectionParameters();

	   write(out, U8(ArrangedConnectRequest));
	   write(out, theParams.mNonce);
	   U32 encryptPos = out.getBytePosition();
	   U32 innerEncryptPos = 0;

	   out.setBytePosition(encryptPos);

	   write(out, theParams.mServerNonce);
	   if(out.writeFlag(theParams.mUsingCrypto))
	   {
		  write(out, theParams.mPrivateKey->getPublicKey());
		  innerEncryptPos = out.getBytePosition();
		  out.setBytePosition(innerEncryptPos);
		  out.writeBuffer(SymmetricCipher::KeySize, theParams.mSymmetricKey);
	   }
	   out.writeFlag(theParams.mDebugObjectSizes);
	   write(out, conn->getInitialSendSequence());
	   conn->writeConnectRequest(&out);

	   if(innerEncryptPos)
	   {
		  SymmetricCipher theCipher(theParams.mSharedSecret);
		  BitStreamHashAndEncrypt(&out, NetConnection::MessageSignatureBytes, innerEncryptPos, &theCipher);
	   }
	   SymmetricCipher theCipher(theParams.mArrangedSecret);
	   BitStreamHashAndEncrypt(&out, NetConnection::MessageSignatureBytes, encryptPos, &theCipher);

	   conn->mConnectSendCount++;
	   conn->mConnectLastSendTime = getProcessStartTime();

	   out.sendto(mSocket, conn->getNetAddress());
	}

   /// Handles an incoming connect request from an arranged connection.
   void handleArrangedConnectRequest(const NetAddress &theAddress, BitStream *stream)
	{
	   U32 i, j;
	   NetConnection *conn;
	   Nonce nonce, serverNonce;
	   read(*stream, &nonce);

	   // see if the connection is in the main connection table.
	   // If the connection is in the connection table and it has
	   // the same initiatorSequence, we'll just resend the connect
	   // acceptance packet, assuming that the last time we sent it
	   // it was dropped.
	   NetConnection *oldConnection = findConnection(theAddress);
	   if(oldConnection)
	   {
		  ConnectionParameters &cp = oldConnection->getConnectionParameters();
		  if(cp.mNonce == nonce)
		  {
			 sendConnectAccept(oldConnection);
			 return;
		  }
	   }

	   for(i = 0; i < mPendingConnections.size(); i++)
	   {
		  conn = mPendingConnections[i];
		  ConnectionParameters &theParams = conn->getConnectionParameters();

		  if(conn->getConnectionState() != NetConnection::SendingPunchPackets || theParams.mIsInitiator)
			 continue;

		  if(nonce != theParams.mNonce)
			 continue;

		  for(j = 0; j < theParams.mPossibleAddresses.size(); j++)
			 if(theAddress.isEqualAddress(theParams.mPossibleAddresses[j]))
				break;
		  if(j != theParams.mPossibleAddresses.size())
			 break;
	   }
	   if(i == mPendingConnections.size())
		  return;
	   
	   ConnectionParameters &theParams = conn->getConnectionParameters();
	   SymmetricCipher theCipher(theParams.mArrangedSecret);
	   if(!BitStreamDecryptAndCheckHash(stream, NetConnection::MessageSignatureBytes, stream->getBytePosition(), &theCipher))
		  return;

	   stream->setBytePosition(stream->getBytePosition());

	   read(*stream, &serverNonce);
	   if(serverNonce != theParams.mServerNonce)
		  return;

	   if(stream->readFlag())
	   {
		  if(mPrivateKey.isNull())
			 return;
		  theParams.mUsingCrypto = true;
		  theParams.mPublicKey = new AsymmetricKey(stream);
		  theParams.mPrivateKey = mPrivateKey;

		  U32 decryptPos = stream->getBytePosition();
		  stream->setBytePosition(decryptPos);
		  theParams.mSharedSecret = theParams.mPrivateKey->computeSharedSecretKey(theParams.mPublicKey);
		  SymmetricCipher theCipher(theParams.mSharedSecret);
		  
		  if(!BitStreamDecryptAndCheckHash(stream, NetConnection::MessageSignatureBytes, decryptPos, &theCipher))
			 return;

		  // now read the first part of the connection's session (symmetric) key
		  stream->readBuffer(SymmetricCipher::KeySize, theParams.mSymmetricKey);
		  Random::read(theParams.mInitVector, SymmetricCipher::KeySize);
	   }

	   U32 connectSequence;
	   theParams.mDebugObjectSizes = stream->readFlag();
	   read(*stream, &connectSequence);
	   TorqueLogMessageFormatted(LogNetInterface, ("Received Arranged Connect Request"));

	   if(oldConnection)
		  disconnect(oldConnection, NetConnection::ReasonSelfDisconnect, "");

	   conn->setNetAddress(theAddress);
	   conn->setInitialRecvSequence(connectSequence);
	   if(theParams.mUsingCrypto)
		  conn->setSymmetricCipher(new SymmetricCipher(theParams.mSymmetricKey, theParams.mInitVector));

	   const char *errorString = NULL;
	   if(!conn->readConnectRequest(stream, &errorString))
	   {
		  sendConnectReject(&theParams, theAddress, errorString);
		  removePendingConnection(conn);
		  return;
	   }
	   addConnection(conn);
	   removePendingConnection(conn);
	   conn->setConnectionState(NetConnection::Connected);
	   conn->onConnectionEstablished();
	   sendConnectAccept(conn);
	}

   
   /// Dispatches a disconnect packet for a specified connection.
   void handleDisconnect(const NetAddress &address, BitStream *stream)
	{
	   NetConnection *conn = findConnection(address);
	   if(!conn)
		  return;

	   ConnectionParameters &theParams = conn->getConnectionParameters();

	   Nonce nonce, serverNonce;
	   char reason[256];

	   read(*stream, &nonce);
	   read(*stream, &serverNonce);

	   if(nonce != theParams.mNonce || serverNonce != theParams.mServerNonce)
		  return;

	   U32 decryptPos = stream->getBytePosition();
	   stream->setBytePosition(decryptPos);

	   if(theParams.mUsingCrypto)
	   {
		  SymmetricCipher theCipher(theParams.mSharedSecret);
		  if(!BitStreamDecryptAndCheckHash(stream, NetConnection::MessageSignatureBytes, decryptPos, &theCipher))
			 return;
	   }
	   stream->readString(reason);

	   conn->setConnectionState(NetConnection::Disconnected);
	   conn->onConnectionTerminated(NetConnection::ReasonRemoteDisconnectPacket, reason);
	   removeConnection(conn);
	}

   /// Handles an error reported while reading a packet from this remote connection.
   void handleConnectionError(NetConnection *theConnection, const String &errorString)
	{
	   disconnect(theConnection, NetConnection::ReasonError, errorString);
	}

   /// Disconnects the given connection and removes it from the NetInterface
   void disconnect(NetConnection *conn, NetConnection::TerminationReason reason, const String &reasonString)
	{
	   if(conn->getConnectionState() == NetConnection::AwaitingChallengeResponse ||
		  conn->getConnectionState() == NetConnection::AwaitingConnectResponse)
	   {
		  conn->onConnectTerminated(reason, reasonString);
		  removePendingConnection(conn);
	   }
	   else if(conn->getConnectionState() == NetConnection::Connected)
	   {
		  conn->setConnectionState(NetConnection::Disconnected);
		  conn->onConnectionTerminated(reason, reasonString);
		  if(conn->isNetworkConnection())
		  {
			 // send a disconnect packet...
			 PacketStream out;
			 write(out, U8(Disconnect));
			 ConnectionParameters &theParams = conn->getConnectionParameters();
			 write(out, theParams.mNonce);
			 write(out, theParams.mServerNonce);
			 U32 encryptPos = out.getBytePosition();
			 out.setBytePosition(encryptPos);
			 out.writeString(reasonString.c_str());

			 if(theParams.mUsingCrypto)
			 {
				SymmetricCipher theCipher(theParams.mSharedSecret);
				BitStreamHashAndEncrypt(&out, NetConnection::MessageSignatureBytes, encryptPos, &theCipher);
			 }
			 out.sendto(mSocket, conn->getNetAddress());
		  }
		  removeConnection(conn);
	   }
	}

   /// @}
public:
   /// @param   bindAddress    Local network address to bind this interface to.
   NetInterface(const NetAddress &bindAddress)
   {
	   NetClassRegistry::Initialize(); // initialize the net class reps, if they haven't been initialized already.

	   mLastTimeoutCheckTime = millisecondsToTime(0);
	   mAllowConnections = true;
	   mRequiresKeyExchange = false;

	   Random::read(mRandomHashData, sizeof(mRandomHashData));

	   mConnectionHashTable.resize(129);
	   for(U32 i = 0; i < mConnectionHashTable.size(); i++)
		  mConnectionHashTable[i] = NULL;
	   mSendPacketList = NULL;
	   mProcessStartTime = GetTime();
	}

   ~NetInterface()
	{
	   // gracefully close all the connections on this NetInterface:
	   while(mConnectionList.size())
	   {
		  NetConnection *c = mConnectionList[0];
		  disconnect(c, NetConnection::ReasonSelfDisconnect, "Shutdown");
	   }
	}


   /// Returns the address of the first network interface in the list that the socket on this NetInterface is bound to.
   NetAddress getFirstBoundInterfaceAddress()
	{
	   NetAddress theAddress = mSocket.getBoundAddress();

	   if(theAddress.isEqualAddress(NetAddress(NetAddress::IPProtocol, NetAddress::Any, 0)))
	   {
		  Array<NetAddress> interfaceAddresses;
		  NetSocket::getInterfaceAddresses(interfaceAddresses);
		  U16 savePort = theAddress.port;
		  if(interfaceAddresses.size())
		  {
			 theAddress = interfaceAddresses[0];
			 theAddress.port = savePort;
		  }
	   }
	   return theAddress;
	}


	/// Sets the private key this NetInterface will use for authentication and key exchange
	void setPrivateKey(AsymmetricKey *theKey)
	{
	   mPrivateKey = theKey;
	}

   /// Requires that all connections use encryption and key exchange
   void setRequiresKeyExchange(bool requires) { mRequiresKeyExchange = requires; }

   /// Sets the public certificate that validates the private key and stores
   /// information about this host.  If no certificate is set, this interface can
   /// still initiate and accept encrypted connections, but they will be vulnerable to
   /// man in the middle attacks, unless the remote host can validate the public key
   /// in another way.
   void setCertificate(Certificate *theCertificate);

   /// Returns whether or not this NetInterface allows connections from remote hosts.
   bool doesAllowConnections() { return mAllowConnections; }

   /// Sets whether or not this NetInterface allows connections from remote hosts.
   void setAllowsConnections(bool conn) { mAllowConnections = conn; }

   /// Returns the Socket associated with this NetInterface
   NetSocket &getSocket() { return mSocket; }

	/// Sends a packet to the remote address over this interface's socket.
	NetSocket::Status sendto(const NetAddress &address, BitStream *stream)
	{
		return mSocket.sendto(address, stream->getBuffer(), stream->getBytePosition());
	}


   /// Sends a packet to the remote address after millisecondDelay time has elapsed.
   ///
   /// This is used to simulate network latency on a LAN or single computer.
   void sendtoDelayed(const NetAddress &address, BitStream *stream, U32 millisecondDelay)
	{
	   U32 dataSize = stream->getBytePosition();

	   // allocate the send packet, with the data size added on
	   DelaySendPacket *thePacket = (DelaySendPacket *) MemoryAllocate(sizeof(DelaySendPacket) + dataSize);
	   thePacket->remoteAddress = address;
	   thePacket->sendTime = getProcessStartTime() + millisecondsToTime(millisecondDelay);
	   thePacket->packetSize = dataSize;
	   memcpy(thePacket->packetData, stream->getBuffer(), dataSize);

	   // insert it into the DelaySendPacket list, sorted by time
	   DelaySendPacket **list;
	   for(list = &mSendPacketList; *list && ((*list)->sendTime <= thePacket->sendTime); list = &((*list)->nextPacket))
		  ;
	   thePacket->nextPacket = *list;
	   *list = thePacket;
	}

   /// Dispatch function for processing all network packets through this NetInterface.
   void checkIncomingPackets()
	{
	   PacketStream stream;
	   NetSocket::Status error;
	   NetAddress sourceAddress;

	   mProcessStartTime = GetTime();

	   // read out all the available packets:
	   while((error = stream.recvfrom(mSocket, &sourceAddress)) == NetSocket::OK)
		  processPacket(sourceAddress, &stream);
	}

   /// Processes a single packet, and dispatches either to handleInfoPacket or to
   /// the NetConnection associated with the remote address.
   virtual void processPacket(const NetAddress &address, BitStream *packetStream)
	{

	   // Determine what to do with this packet:

	   if(pStream->getBuffer()[0] & 0x80) // it's a protocol packet...
	   {
		  // if the LSB of the first byte is set, it's a game data packet
		  // so pass it to the appropriate connection.

		  // lookup the connection in the addressTable
		  // if this packet causes a disconnection, keep the conn around until this function exits
		  RefPtr<NetConnection> conn = findConnection(sourceAddress);
		  if(conn)
			 conn->readRawPacket(pStream);
	   }
	   else
	   {
		  // Otherwise, it's either a game info packet or a
		  // connection handshake packet.

		  U8 packetType;
		  read(*pStream, &packetType);

		  if(packetType >= FirstValidInfoPacketId)
			 handleInfoPacket(sourceAddress, packetType, pStream);
		  else
		  {
			 // check if there's a connection already:
			 switch(packetType)
			 {
				case ConnectChallengeRequest:
				   handleConnectChallengeRequest(sourceAddress, pStream);
				   break;
				case ConnectChallengeResponse:
				   handleConnectChallengeResponse(sourceAddress, pStream);
				   break;
				case ConnectRequest:
				   handleConnectRequest(sourceAddress, pStream);
				   break;
				case ConnectReject:
				   handleConnectReject(sourceAddress, pStream);
				   break;
				case ConnectAccept:
				   handleConnectAccept(sourceAddress, pStream);
				   break;
				case Disconnect:
				   handleDisconnect(sourceAddress, pStream);
				   break;
				case Punch:
				   handlePunch(sourceAddress, pStream);
				   break;
				case ArrangedConnectRequest:
				   handleArrangedConnectRequest(sourceAddress, pStream);
				   break;
			 }
		  }
	   }
	}


   /// Handles all packets that don't fall into the category of connection handshake or game data.
   virtual void handleInfoPacket(const NetAddress &address, U8 packetType, BitStream *stream)
   {}

   /// Checks all connections on this interface for packet sends, and for timeouts and all valid
   /// and pending connections.
   void processConnections()
	{
	   mProcessStartTime = GetTime();
	   mPuzzleManager.tick(mProcessStartTime);

	   // first see if there are any delayed packets that need to be sent...
	   while(mSendPacketList && mSendPacketList->sendTime < getProcessStartTime())
	   {
		  DelaySendPacket *next = mSendPacketList->nextPacket;
		  mSocket.sendto(mSendPacketList->remoteAddress,
				mSendPacketList->packetData, mSendPacketList->packetSize);
		  MemoryDeallocate(mSendPacketList);
		  mSendPacketList = next;
	   }

	   NetObject::collapseDirtyList(); // collapse all the mask bits...
	   for(U32 i = 0; i < mConnectionList.size(); i++)
		  mConnectionList[i]->checkPacketSend(false, getProcessStartTime());

	   if(getProcessStartTime() > mLastTimeoutCheckTime + millisecondsToTime(TimeoutCheckInterval))
	   {
		  for(U32 i = 0; i < mPendingConnections.size();)
		  {
			 NetConnection *pending = mPendingConnections[i];

			 if(pending->getConnectionState() == NetConnection::AwaitingChallengeResponse &&
				getProcessStartTime() > pending->mConnectLastSendTime + 
				millisecondsToTime(ChallengeRetryTime))
			 {
				if(pending->mConnectSendCount > ChallengeRetryCount)
				{
				   pending->setConnectionState(NetConnection::ConnectTimedOut);
				   pending->onConnectTerminated(NetConnection::ReasonTimedOut, "Timeout");
				   removePendingConnection(pending);
				   continue;
				}
				else
				   sendConnectChallengeRequest(pending);
			 }
			 else if(pending->getConnectionState() == NetConnection::AwaitingConnectResponse &&
				getProcessStartTime() > pending->mConnectLastSendTime + 
				millisecondsToTime(ConnectRetryTime))
			 {
				if(pending->mConnectSendCount > ConnectRetryCount)
				{
				   pending->setConnectionState(NetConnection::ConnectTimedOut);
				   pending->onConnectTerminated(NetConnection::ReasonTimedOut, "Timeout");
				   removePendingConnection(pending);
				   continue;
				}
				else
				{
				   if(pending->getConnectionParameters().mIsArranged)
					  sendArrangedConnectRequest(pending);
				   else
					  sendConnectRequest(pending);
				}
			 }
			 else if(pending->getConnectionState() == NetConnection::SendingPunchPackets &&
				getProcessStartTime() > pending->mConnectLastSendTime + millisecondsToTime(PunchRetryTime))
			 {
				if(pending->mConnectSendCount > PunchRetryCount)
				{
				   pending->setConnectionState(NetConnection::ConnectTimedOut);
				   pending->onConnectTerminated(NetConnection::ReasonTimedOut, "Timeout");
				   removePendingConnection(pending);
				   continue;
				}
				else
				   sendPunchPackets(pending);
			 }
			 else if(pending->getConnectionState() == NetConnection::ComputingPuzzleSolution &&
				getProcessStartTime() > pending->mConnectLastSendTime + 
				millisecondsToTime(PuzzleSolutionTimeout))
			 {
				pending->setConnectionState(NetConnection::ConnectTimedOut);
				pending->onConnectTerminated(NetConnection::ReasonTimedOut, "Timeout");
				removePendingConnection(pending);
			 }
			 i++;
		  }
		  mLastTimeoutCheckTime = getProcessStartTime();

		  for(U32 i = 0; i < mConnectionList.size();)
		  {
			 if(mConnectionList[i]->checkTimeout(getProcessStartTime()))
			 {
				mConnectionList[i]->setConnectionState(NetConnection::TimedOut);
				mConnectionList[i]->onConnectionTerminated(NetConnection::ReasonTimedOut, "Timeout");
				removeConnection(mConnectionList[i]);
			 }
			 else
				i++;
		  }
	   }

	   // check if we're trying to solve any client connection puzzles
	   for(U32 i = 0; i < mPendingConnections.size(); i++)
	   {
		  if(mPendingConnections[i]->getConnectionState() == NetConnection::ComputingPuzzleSolution)
		  {
			 continuePuzzleSolution(mPendingConnections[i]);
			 break;
		  }
	   }
	}


   /// Returns the list of connections on this NetInterface.
   Array<RefPtr<NetConnection> > &getConnectionList() { return mConnectionList; }

   /// looks up a connected connection on this NetInterface
   NetConnection *findConnection(const NetAddress &remoteAddress)
	{
	   // The connection hash table is a single vector, with hash collisions
	   // resolved to the next open space in the table.

	   // Compute the hash index based on the network address
	   U32 hashIndex = addr.hash() % mConnectionHashTable.size();

	   // Search through the table for an address that matches the source
	   // address.  If the connection pointer is NULL, we've found an
	   // empty space and a connection with that address is not in the table
	   while(mConnectionHashTable[hashIndex] != NULL)
	   {
		  if(addr == mConnectionHashTable[hashIndex]->getNetAddress())
			 return mConnectionHashTable[hashIndex];
		  hashIndex++;
		  if(hashIndex >= (U32) mConnectionHashTable.size())
			 hashIndex = 0;
	   }
	   return NULL;
	}


   /// returns the current process time for this NetInterface
   Time getProcessStartTime() { return mProcessStartTime; }
};

