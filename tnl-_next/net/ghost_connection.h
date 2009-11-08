/// GhostConnection is a subclass of EventConnection that manages the transmission (ghosting) and updating of NetObjects over a connection.  The GhostConnection is responsible for doing scoping calculations (on the server side) and transmitting most-recent ghost information to the client.
/// Ghosting is the most complex, and most powerful, part of TNL's capabilities. It allows the information sent to clients to be very precisely matched to what they need, so that no excess bandwidth is wasted.  Each GhostConnection has a <b>scope object</b> that is responsible for determining what other NetObject instances are relevant to that connection's client.  Each time GhostConnection sends a packet, NetObject::performScopeQuery() is called on the scope object, which calls GhostConnection::objectInScope() for each relevant object.
/// Each object that is in scope, and in need of update (based on its maskbits) is given a priority ranking by calling that object's getUpdatePriority() method.  The packet is then filled with updates, ordered by priority. This way the most important updates get through first, with less important updates being sent as space is available.
/// There is a cap on the maximum number of ghosts that can be active through a GhostConnection at once.  The enum GhostIdBitSize (defaults to 10) determines how many bits will be used to transmit the ID for each ghost, so the maximum number is 2^GhostIdBitSize or 1024.  This can be easily raised; see the  GhostConstants enum.
/// Each object ghosted is assigned a ghost ID; the client is <b>only</b> aware of the ghost ID. This acts to enhance simulation security, as it becomes difficult to map objects from one connection to another, or to reliably identify objects from ID alone. IDs are also reassigned based on need, making it hard to track objects that have fallen out of scope (as any object which the player shouldn't see would).
/// resolveGhost() is used on the client side, and resolveObjectFromGhostIndex() on the server side, to convert ghost IDs to object references.
/// @see NetObject for more information on network object functionality.
class GhostConnection : public EventConnection
{
   typedef EventConnection Parent;
   friend class ConnectionMessageEvent;
public:
   /// GhostRef tracks an update sent in one packet for the ghost of one NetObject.
   ///
   /// When we are notified that a pack is sent/lost, this is used to determine what
   /// updates need to be resent and so forth.
   struct GhostRef
   {
      U32 mask;              ///< The mask of bits that were updated in this packet
      U32 ghostInfoFlags;    ///< GhostInfo::Flags bitset, determes if the ghost is in a
                             ///  special processing mode (created/deleted)
      GhostInfo *ghost;      ///< The ghost information for the object on the connection that sent
                             ///  the packet this GhostRef is attached to
      GhostRef *nextRef;     ///< The next ghost updated in this packet
      GhostRef *updateChain; ///< A pointer to the GhostRef on the least previous packet that
                             ///  updated this ghost, or NULL, if no prior packet updated this ghost
   };

   /// Notify structure attached to each packet with information about the ghost updates in the packet
   struct GhostPacketNotify : public EventConnection::EventPacketNotify
   {
      GhostRef *ghostList; ///< list of ghosts updated in this packet
      GhostPacketNotify() { ghostList = NULL; }
   };

protected:

   /// Override of EventConnection's allocNotify, to use the GhostPacketNotify structure.
   PacketNotify *allocNotify() { return new GhostPacketNotify; }

   /// Override to properly update the GhostInfo's for all ghosts that had upates in the dropped packet.
   void packetDropped(PacketNotify *notify)
	{
	   Parent::packetDropped(pnotify);
	   GhostPacketNotify *notify = static_cast<GhostPacketNotify *>(pnotify);

	   GhostRef *packRef = notify->ghostList;
	   // loop through all the packRefs in the packet
	 
	   while(packRef)
	   {
		  GhostRef *temp = packRef->nextRef;

		  U32 updateFlags = packRef->mask;

		  // figure out which flags need to be updated on the object
		  for(GhostRef *walk = packRef->updateChain; walk && updateFlags; walk = walk->updateChain)
			 updateFlags &= ~walk->mask;
		 
		  // for any flags we haven't updated since this (dropped) packet
		  // or them into the mask so they'll get updated soon

		  if(updateFlags)
		  {
			 if(!packRef->ghost->updateMask)
			 {
				packRef->ghost->updateMask = updateFlags;
				ghostPushNonZero(packRef->ghost);
			 }
			 else
				packRef->ghost->updateMask |= updateFlags;
		  }

		  // make sure this packRef isn't the last one on the GhostInfo
		  if(packRef->ghost->lastUpdateChain == packRef)
			 packRef->ghost->lastUpdateChain = NULL;
		  
		  // if this packet was ghosting an object, set it
		  // to re ghost at it's earliest convenience

		  if(packRef->ghostInfoFlags & GhostInfo::Ghosting)
		  {
			 packRef->ghost->flags |= GhostInfo::NotYetGhosted;
			 packRef->ghost->flags &= ~GhostInfo::Ghosting;
		  }
		  
		  // otherwise, if it was being deleted,
		  // set it to re-delete

		  else if(packRef->ghostInfoFlags & GhostInfo::KillingGhost)
		  {
			 packRef->ghost->flags |= GhostInfo::KillGhost;
			 packRef->ghost->flags &= ~GhostInfo::KillingGhost;
		  }

		  delete packRef;
		  packRef = temp;
	   }
	}

   /// Override to update flags associated with the ghosts updated in this packet.
   void packetReceived(PacketNotify *notify)
	{
	   Parent::packetReceived(pnotify);
	   GhostPacketNotify *notify = static_cast<GhostPacketNotify *>(pnotify);

	   GhostRef *packRef = notify->ghostList;

	   // loop through all the notifies in this packet

	   while(packRef)
	   {
		  // make sure this packRef isn't the last one on the GhostInfo
		  if(packRef->ghost->lastUpdateChain == packRef)
			 packRef->ghost->lastUpdateChain = NULL;

		  GhostRef *temp = packRef->nextRef;      
		  // if this object was ghosting , it is now ghosted

		  if(packRef->ghostInfoFlags & GhostInfo::Ghosting)
		  {
			 packRef->ghost->flags &= ~GhostInfo::Ghosting;
			 if(packRef->ghost->obj)
				packRef->ghost->obj->onGhostAvailable(this);
		  }
		  // otherwise, if it was dieing, free the ghost

		  else if(packRef->ghostInfoFlags & GhostInfo::KillingGhost)
			 freeGhostInfo(packRef->ghost);

		  delete packRef;
		  packRef = temp;
	   }
	}

	static bool UQECompare(const GhostInfo *a,const GhostInfo *b)
	{
	   return a->priority < b->priority;
	} 


   /// Performs the scoping query in order to determine if there is data to send from this GhostConnection.
   void prepareWritePacket()
	{
	   Parent::prepareWritePacket();

	   if(!doesGhostFrom() && !mGhosting)
		  return;
	   // first step is to check all our polled ghosts:

	   // 1. Scope query - find if any new objects have come into
	   //    scope and if any have gone out.

	   // Each packet we loop through all the objects with non-zero masks and
	   // mark them as "out of scope" before the scope query runs.
	   // if the object has a zero update mask, we wait to remove it until it requests
	   // an update

	   for(S32 i = 0; i < mGhostZeroUpdateIndex; i++)
	   {
		  // increment the updateSkip for everyone... it's all good
		  GhostInfo *walk = mGhostArray[i];
		  walk->updateSkipCount++;
		  if(!(walk->flags & (GhostInfo::ScopeLocalAlways)))
			 walk->flags &= ~GhostInfo::InScope;
	   }

	   if(mScopeObject)
		  mScopeObject->performScopeQuery(this);
	}

   /// Override to write ghost updates into each packet.
   void writePacket(BitStream *bstream, PacketNotify *notify)
	{
	   Parent::writePacket(bstream, pnotify);
	   GhostPacketNotify *notify = static_cast<GhostPacketNotify *>(pnotify);

	   if(mConnectionParameters.mDebugObjectSizes)
		  bstream->writeInt(DebugChecksum, 32);

	   notify->ghostList = NULL;
	   
	   if(!doesGhostFrom())
		  return;
	   
	   if(!bstream->writeFlag(mGhosting && mScopeObject.isValid()))
		  return;
		  
	   // fill a packet (or two) with ghosting data

	   // 2. call scoped objects' priority functions if the flag set is nonzero
	   //    A removed ghost is assumed to have a high priority
	   // 3. call updates based on sorted priority until the packet is
	   //    full.  set flags to zero for all updated objects

	   GhostInfo *walk;

	   for(S32 i = mGhostZeroUpdateIndex - 1; i >= 0; i--)
	   {
		  if(!(mGhostArray[i]->flags & GhostInfo::InScope))
			 detachObject(mGhostArray[i]);
	   }

	   U32 maxIndex = 0;
	   for(S32 i = mGhostZeroUpdateIndex - 1; i >= 0; i--)
	   {
		  walk = mGhostArray[i];
		  if(walk->index > maxIndex)
			 maxIndex = walk->index;

		  // clear out any kill objects that haven't been ghosted yet
		  if((walk->flags & GhostInfo::KillGhost) && (walk->flags & GhostInfo::NotYetGhosted))
		  {
			 freeGhostInfo(walk);
			 continue;
		  }
		  // don't do any ghost processing on objects that are being killed
		  // or in the process of ghosting
		  else if(!(walk->flags & (GhostInfo::KillingGhost | GhostInfo::Ghosting)))
		  {
			 if(walk->flags & GhostInfo::KillGhost)
				walk->priority = 10000;
			 else
				walk->priority = walk->obj->getUpdatePriority(mScopeObject, walk->updateMask, walk->updateSkipCount);
		  }
		  else
			 walk->priority = 0;
	   }
	   GhostRef *updateList = NULL;
	   quickSort(mGhostArray, mGhostArray + mGhostZeroUpdateIndex, UQECompare);
	   // reset the array indices...
	   for(S32 i = mGhostZeroUpdateIndex - 1; i >= 0; i--)
		  mGhostArray[i]->arrayIndex = i;

	   S32 sendSize = 1;
	   while(maxIndex >>= 1)
		  sendSize++;

	   if(sendSize < 3)
		  sendSize = 3;

	   bstream->writeInt(sendSize - 3, 3); // 0-7 3 bit number

	   U32 count = 0;
	   // 
	   for(S32 i = mGhostZeroUpdateIndex - 1; i >= 0 && !bstream->isFull(); i--)
	   {
		  GhostInfo *walk = mGhostArray[i];
			if(walk->flags & (GhostInfo::KillingGhost | GhostInfo::Ghosting))
			   continue;

		  U32 updateStart = bstream->getBitPosition();
		  U32 updateMask = walk->updateMask;
		  U32 retMask;
			   
		  bstream->writeFlag(true);
		  bstream->writeInt(walk->index, sendSize);
		  if(!bstream->writeFlag(walk->flags & GhostInfo::KillGhost))
		  {
			 // this is an update of some kind:
			 if(mConnectionParameters.mDebugObjectSizes)
				bstream->advanceBitPosition(BitStreamPosBitSize);

			 S32 startPos = bstream->getBitPosition();

			 if(walk->flags & GhostInfo::NotYetGhosted)
			 {
				S32 classId = NetClassRegistry::GetClassIndexOfObject(walk->obj, getNetClassGroup());
				NetClassRegistry::WriteClassIndex(bstream, classId, NetClassTypeObject, getNetClassGroup());
				NetObject::mIsInitialUpdate = true;
			 }

			 // update the object
			 retMask = walk->obj->packUpdate(this, updateMask, bstream);

			 if(NetObject::mIsInitialUpdate)
			 {
				NetObject::mIsInitialUpdate = false;
				NetClassRegistry::AddInitialUpdate(walk->obj, bstream->getBitPosition() - startPos);
			 }
			 else
				NetClassRegistry::AddPartialUpdate(walk->obj, bstream->getBitPosition() - startPos);

			 if(mConnectionParameters.mDebugObjectSizes)
				bstream->writeIntAt(bstream->getBitPosition(), BitStreamPosBitSize, startPos - BitStreamPosBitSize);

			 TorqueLogMessageFormatted(LogGhostConnection, ("GhostConnection %s GHOST %d", walk->obj->getClassName(), bstream->getBitPosition() - 16 - startPos));

			 Assert((retMask & (~updateMask)) == 0, "Cannot set new bits in packUpdate return");
		  }

		  // check for packet overrun, and rewind this update if there
		  // was one:
		  if(bstream->getBitSpaceAvailable() < MinimumPaddingBits)
		  {
			 bstream->setBitPosition(updateStart);
			 bstream->clearError();
			 break;
		  }

		  // otherwise, create a record of this ghost update and
		  // attach it to the packet.
		  GhostRef *upd = new GhostRef;

		  upd->nextRef = updateList;
		  updateList = upd;

		  if(walk->lastUpdateChain)
			 walk->lastUpdateChain->updateChain = upd;
		  walk->lastUpdateChain = upd;

		  upd->ghost = walk;
		  upd->ghostInfoFlags = 0;
		  upd->updateChain = NULL;

		  if(walk->flags & GhostInfo::KillGhost)
		  {
			 walk->flags &= ~GhostInfo::KillGhost;
			 walk->flags |= GhostInfo::KillingGhost;
			 walk->updateMask = 0;
			 upd->mask = updateMask;
			 ghostPushToZero(walk);
			 upd->ghostInfoFlags = GhostInfo::KillingGhost;
		  }
		  else
		  {
			 if(walk->flags & GhostInfo::NotYetGhosted)
			 {
				walk->flags &= ~GhostInfo::NotYetGhosted;
				walk->flags |= GhostInfo::Ghosting;
				upd->ghostInfoFlags = GhostInfo::Ghosting;
			 }
			 walk->updateMask = retMask;
			 if(!retMask)
				ghostPushToZero(walk);
			 upd->mask = updateMask & ~retMask;
			 walk->updateSkipCount = 0;
			 count++;
		  }
	   }
	   // count # of ghosts have been updated,
	   // mGhostZeroUpdateIndex # of ghosts remain to be updated.
	   // no more objects...
	   bstream->writeFlag(false);
	   notify->ghostList = updateList;
	}

   /// Override to read updated ghost information from the packet stream.
   void readPacket(BitStream *bstream)
	{
	   Parent::readPacket(bstream);

	   if(mConnectionParameters.mDebugObjectSizes)
	   {
		  U32 sum = bstream->readInt(32);
		  Assert(sum == DebugChecksum, "Invalid checksum.");
	   }

	   if(!doesGhostTo())
		  return;
	   if(!bstream->readFlag())
		  return;

	   S32 idSize;
	   idSize = bstream->readInt( 3 );
	   idSize += 3;

	   // while there's an object waiting...

	   while(bstream->readFlag())
	   {
		  U32 index;
		  //S32 startPos = bstream->getCurPos();
		  index = (U32) bstream->readInt(idSize);
		  if(bstream->readFlag()) // is this ghost being deleted?
		  {
			 Assert(mLocalGhosts[index] != NULL, "Error, NULL ghost encountered.");
			   if(mLocalGhosts[index])
			 {
				mLocalGhosts[index]->onGhostRemove();
				  delete mLocalGhosts[index];
				mLocalGhosts[index] = NULL;
			 }
		  }
		  else
		  {
			 U32 endPosition = 0;
			 if(mConnectionParameters.mDebugObjectSizes)
				endPosition = bstream->readInt(BitStreamPosBitSize);

			 if(!mLocalGhosts[index]) // it's a new ghost... cool
			 {
				S32 classId = NetClassRegistry::ReadClassIndex(bstream, NetClassTypeObject, getNetClassGroup());
				if(classId == -1)
				{
				   setLastError("Invalid packet.");
				   return;
				}
				NetObject *obj = (NetObject *) 
					  NetClassRegistry::Create(getNetClassGroup(), NetClassTypeObject, classId);
				if(!obj)
				{
				   setLastError("Invalid packet.");
				   return;
				}
				obj->mOwningConnection = this;
				obj->mNetFlags = NetObject::IsGhost;

				// object gets initial update before adding to the manager

				obj->mNetIndex = index;
				mLocalGhosts[index] = obj;

				NetObject::mIsInitialUpdate = true;
				mLocalGhosts[index]->unpackUpdate(this, bstream);
				NetObject::mIsInitialUpdate = false;
				
				if(!obj->onGhostAdd(this))
				{
				   if(mErrorString.isEmpty())
					  setLastError("Invalid packet.");
				   return;
				}
				if(mRemoteConnection)
				{
				   GhostConnection *gc = static_cast<GhostConnection *>(mRemoteConnection.getPointer());
				   obj->mServerObject = gc->resolveGhostParent(index);
				}
			 }
			 else
			 {
				mLocalGhosts[index]->unpackUpdate(this, bstream);
			 }

			 if(mConnectionParameters.mDebugObjectSizes)
			 {
				AssertFormatted(bstream->getBitPosition() == endPosition,
				("unpackUpdate did not match packUpdate for object of class %s. Expected %d bits, got %d bits.",
				   mLocalGhosts[index]->getClassName(), endPosition, bstream->getBitPosition()) );
			 }

			 if(!mErrorString.isEmpty())
				return;
		  }
	   }
	}

   /// Override to check if there is data pending on this GhostConnection.
   bool isDataToTransmit()
	{
	   // once we've run the scope query - if there are no objects that need to be updated,
	   // we return false
	   return Parent::isDataToTransmit() || mGhostZeroUpdateIndex != 0;
	}

//----------------------------------------------------------------
// ghost manager functions/code:
//----------------------------------------------------------------

protected:
	/// Array of GhostInfo structures used to track all the objects ghosted by this side of the connection. For efficiency, ghosts are stored in three segments - the first segment contains GhostInfos that have pending updates, the second ghostrefs that need no updating, and last, free GhostInfos that may be reused.
   GhostInfo **mGhostArray;
   
   
   S32 mGhostZeroUpdateIndex; ///< Index in mGhostArray of first ghost with 0 update mask (ie, with no updates).

   S32 mGhostFreeIndex; ///< index in mGhostArray of first free ghost.

   bool mGhosting; ///< Am I currently ghosting objects over?
   bool mScoping; ///< Am I currently allowing objects to be scoped?
   U32  mGhostingSequence; ///< Sequence number describing this ghosting session.

   NetObject **mLocalGhosts; ///< Local ghost array for remote objects, or NULL if mGhostTo is false.

   GhostInfo *mGhostRefs; ///< Allocated array of ghostInfos, or NULL if mGhostFrom is false.
   GhostInfo **mGhostLookupTable; ///< Table indexed by object id->GhostInfo, or NULL if mGhostFrom is false.

   SafePtr<NetObject> mScopeObject;///< The local NetObject that performs scoping queries to determine what objects to ghost to the client.

   void clearGhostInfo()
	{
	   // gotta clear out the ghosts...
	   for(PacketNotify *walk = mNotifyQueueHead; walk; walk = walk->nextPacket)
	   {
		  GhostPacketNotify *note = static_cast<GhostPacketNotify *>(walk);
		  GhostRef *delWalk = note->ghostList;
		  note->ghostList = NULL;
		  while(delWalk)
		  {
			 GhostRef *next = delWalk->nextRef;
			 delete delWalk;
			 delWalk = next;
		  }
	   }
	   for(S32 i = 0; i < MaxGhostCount; i++)
	   {
		  if(mGhostRefs[i].arrayIndex < mGhostFreeIndex)
		  {
			 detachObject(&mGhostRefs[i]);
			 mGhostRefs[i].lastUpdateChain = NULL;
			 freeGhostInfo(&mGhostRefs[i]);
		  }
	   }
	   Assert((mGhostFreeIndex == 0) && (mGhostZeroUpdateIndex == 0), "Invalid indices.");
	}

   void deleteLocalGhosts()
   {
	   if(!mLocalGhosts)
		  return;
	   // just delete all the local ghosts,
	   // and delete all the ghosts in the current save list
	   for(S32 i = 0; i < MaxGhostCount; i++)
	   {
		  if(mLocalGhosts[i])
		  {
			 mLocalGhosts[i]->onGhostRemove();
			 delete mLocalGhosts[i];
			 mLocalGhosts[i] = NULL;
		  }
	   }
	}

   
   
   bool validateGhostArray()
	{
	   Assert(mGhostZeroUpdateIndex >= 0 && mGhostZeroUpdateIndex <= mGhostFreeIndex, "Invalid update index range.");
	   Assert(mGhostFreeIndex <= MaxGhostCount, "Invalid free index range.");
	   S32 i;
	   for(i = 0; i < mGhostZeroUpdateIndex; i ++)
	   {
		  Assert(mGhostArray[i]->arrayIndex == i, "Invalid array index.");
		  Assert(mGhostArray[i]->updateMask != 0, "Invalid ghost mask.");
	   }
	   for(; i < mGhostFreeIndex; i ++)
	   {
		  Assert(mGhostArray[i]->arrayIndex == i, "Invalid array index.");
		  Assert(mGhostArray[i]->updateMask == 0, "Invalid ghost mask.");
	   }
	   for(; i < MaxGhostCount; i++)
	   {
		  Assert(mGhostArray[i]->arrayIndex == i, "Invalid array index.");
	   }
	   return true;
	}

   void freeGhostInfo(GhostInfo *)
	{
	   Assert(ghost->arrayIndex < mGhostFreeIndex, "Ghost already freed.");
	   if(ghost->arrayIndex < mGhostZeroUpdateIndex)
	   {
		  Assert(ghost->updateMask != 0, "Invalid ghost mask.");
		  ghost->updateMask = 0;
		  ghostPushToZero(ghost);
	   }
	   ghostPushZeroToFree(ghost);
	   Assert(ghost->lastUpdateChain == NULL, "Ack!");
	}

   /// Notifies subclasses that the remote host is about to start ghosting objects.
   virtual void onStartGhosting() {}                              

   /// Notifies subclasses that the server has stopped ghosting objects on this connection.
   virtual void onEndGhosting() {}

public:
   GhostConnection()
	{
	   // ghost management data:
	   mScopeObject = NULL;
	   mGhostingSequence = 0;
	   mGhosting = false;
	   mScoping = false;
	   mGhostArray = NULL;
	   mGhostRefs = NULL;
	   mGhostLookupTable = NULL;
	   mLocalGhosts = NULL;
	   mGhostZeroUpdateIndex = 0;
	}

	~GhostConnection()
	{
	   clearAllPacketNotifies();

	   // delete any ghosts that may exist for this connection, but aren't added
	   if(mGhostArray)
		  clearGhostInfo();
	   deleteLocalGhosts();
	   delete[] mLocalGhosts;
	   delete[] mGhostLookupTable;
	   delete[] mGhostRefs;
	   delete[] mGhostArray;
	}

   /// Sets whether ghosts transmit from this side of the connection.
   void setGhostFrom(bool ghostFrom)
	{
	   if(mGhostArray)
		  return;

	   if(ghostFrom)
	   {
		  mGhostFreeIndex = mGhostZeroUpdateIndex = 0;
		  mGhostArray = new GhostInfo *[MaxGhostCount];
		  mGhostRefs = new GhostInfo[MaxGhostCount];
		  S32 i;
		  for(i = 0; i < MaxGhostCount; i++)
		  {
			 mGhostRefs[i].obj = NULL;
			 mGhostRefs[i].index = i;
			 mGhostRefs[i].updateMask = 0;
		  }
		  mGhostLookupTable = new GhostInfo *[GhostLookupTableSize];
		  for(i = 0; i < GhostLookupTableSize; i++)
			 mGhostLookupTable[i] = 0;
	   }
	}

	/// Sets whether ghosts are allowed from the other side of the connection.
   void setGhostTo(bool ghostTo)
   {
	   if(mLocalGhosts) // if ghosting to this is already enabled, silently return
		  return;

	   if(ghostTo)
	   {
		  mLocalGhosts = new NetObject *[MaxGhostCount];
		  for(S32 i = 0; i < MaxGhostCount; i++)
			 mLocalGhosts[i] = NULL;
	   }
	}


   /// Does this GhostConnection ghost NetObjects to the remote host?
   bool doesGhostFrom() { return mGhostArray != NULL; } 
   
   /// Does this GhostConnection receive ghosts from the remote host?
   bool doesGhostTo() { return mLocalGhosts != NULL; }

   /// Returns the sequence number of this ghosting session.
   U32 getGhostingSequence() { return mGhostingSequence; }

   enum GhostConstants {
      GhostIdBitSize = 10, ///< Size, in bits, of the integer used to transmit ghost IDs
      GhostLookupTableSizeShift = 10, ///< The size of the hash table used to lookup source NetObjects by remote ghost ID is 1 << GhostLookupTableSizeShift.

      MaxGhostCount = (1 << GhostIdBitSize), ///< Maximum number of ghosts that can be active at any one time.
      GhostCountBitSize = GhostIdBitSize + 1, ///< Size of the field needed to transmit the total number of ghosts.

      GhostLookupTableSize = (1 << GhostLookupTableSizeShift), ///< Size of the hash table used to lookup source NetObjects by remote ghost ID.
      GhostLookupTableMask = (GhostLookupTableSize - 1), ///< Hashing mask for table lookups.
   };

	/// Sets the object that is queried at each packet to determine what NetObjects should be ghosted on this connection.
	void setScopeObject(NetObject *object)
	{
	   if(((NetObject *) mScopeObject) == obj)
		  return;
	   mScopeObject = obj;
	}

   
	/// Returns the current scope object.
	NetObject *getScopeObject() { return (NetObject*)mScopeObject; }; 
   
	/// Indicate that the specified object is currently in scope.  Method called by the scope object to indicate that the specified object is in scope.
   void objectInScope(NetObject *object)
	{
	   if (!mScoping || !doesGhostFrom())
		  return;
		if (!obj->isGhostable() || (obj->isScopeLocal() && !isLocalConnection()))
			return;
	   S32 index = obj->getHashId() & GhostLookupTableMask;
	   
	   // check if it's already in scope
	   // the object may have been cleared out without the lookupTable being cleared
	   // so validate that the object pointers are the same.

	   for(GhostInfo *walk = mGhostLookupTable[index ]; walk; walk = walk->nextLookupInfo)
	   {
		  if(walk->obj != obj)
			 continue;
		  walk->flags |= GhostInfo::InScope;
		  return;
	   }

	   if (mGhostFreeIndex == MaxGhostCount)
		  return;

	   GhostInfo *giptr = mGhostArray[mGhostFreeIndex];
	   ghostPushFreeToZero(giptr);
	   giptr->updateMask = 0xFFFFFFFF;
	   ghostPushNonZero(giptr);

	   giptr->flags = GhostInfo::NotYetGhosted | GhostInfo::InScope;
	   
	   giptr->obj = obj;
	   giptr->lastUpdateChain = NULL;
	   giptr->updateSkipCount = 0;

	   giptr->connection = this;

	   giptr->nextObjectRef = obj->mFirstObjectRef;
	   if(obj->mFirstObjectRef)
		  obj->mFirstObjectRef->prevObjectRef = giptr;
	   giptr->prevObjectRef = NULL;
	   obj->mFirstObjectRef = giptr;
	   
	   giptr->nextLookupInfo = mGhostLookupTable[index];
	   mGhostLookupTable[index] = giptr;
	   //Assert(validateGhostArray(), "Invalid ghost array!");
	}
   
	/// The specified object should be always in scope for this connection.
   void objectLocalScopeAlways(NetObject *object)
   {
	   if(!doesGhostFrom())
		  return;
	   objectInScope(obj);
	   for(GhostInfo *walk = mGhostLookupTable[obj->getHashId() & GhostLookupTableMask]; walk; walk = walk->nextLookupInfo)
	   {
		  if(walk->obj != obj)
			 continue;
		  walk->flags |= GhostInfo::ScopeLocalAlways;
		  return;
	   }
	}

	 /// The specified object should not be always in scope for this connection.
   void objectLocalClearAlways(NetObject *object)
   {
	   if(!doesGhostFrom())
		  return;
	   for(GhostInfo *walk = mGhostLookupTable[obj->getHashId() & GhostLookupTableMask]; walk; walk = walk->nextLookupInfo)
	   {
		  if(walk->obj != obj)
			 continue;
		  walk->flags &= ~GhostInfo::ScopeLocalAlways;
		  return;
	   }
	}


	/// Given an object's ghost id, returns the ghost of the object (on the client side).
   NetObject *resolveGhost(S32 id)
   {
	   if(id == -1)
		  return NULL;

	   return mLocalGhosts[id];
	}


	/// Given an object's ghost id, returns the source object (on the server side).
   NetObject *resolveGhostParent(S32 id)
   {
	   return mGhostRefs[id].obj;
	}

	/// Moves the specified GhostInfo into the range of the ghost array for non-zero updateMasks.
   void ghostPushNonZero(GhostInfo *gi)
	{
	   Assert(info->arrayIndex >= mGhostZeroUpdateIndex && info->arrayIndex < mGhostFreeIndex, "Out of range arrayIndex.");
	   Assert(mGhostArray[info->arrayIndex] == info, "Invalid array object.");
	   if(info->arrayIndex != mGhostZeroUpdateIndex)
	   {
		  mGhostArray[mGhostZeroUpdateIndex]->arrayIndex = info->arrayIndex;
		  mGhostArray[info->arrayIndex] = mGhostArray[mGhostZeroUpdateIndex];
		  mGhostArray[mGhostZeroUpdateIndex] = info;
		  info->arrayIndex = mGhostZeroUpdateIndex;
	   }
	   mGhostZeroUpdateIndex++;
	   //Assert(validateGhostArray(), "Invalid ghost array!");
	}

	/// Moves the specified GhostInfo into the range of the ghost array for zero updateMasks.
   void ghostPushToZero(GhostInfo *gi)
   {
	   Assert(info->arrayIndex < mGhostZeroUpdateIndex, "Out of range arrayIndex.");
	   Assert(mGhostArray[info->arrayIndex] == info, "Invalid array object.");
	   mGhostZeroUpdateIndex--;
	   if(info->arrayIndex != mGhostZeroUpdateIndex)
	   {
		  mGhostArray[mGhostZeroUpdateIndex]->arrayIndex = info->arrayIndex;
		  mGhostArray[info->arrayIndex] = mGhostArray[mGhostZeroUpdateIndex];
		  mGhostArray[mGhostZeroUpdateIndex] = info;
		  info->arrayIndex = mGhostZeroUpdateIndex;
	   }
	   //Assert(validateGhostArray(), "Invalid ghost array!");
	}

	/// Moves the specified GhostInfo into the range of the ghost array for free (unused) GhostInfos.
   void ghostPushZeroToFree(GhostInfo *gi)
   {
	   Assert(info->arrayIndex >= mGhostZeroUpdateIndex && info->arrayIndex < mGhostFreeIndex, "Out of range arrayIndex.");
	   Assert(mGhostArray[info->arrayIndex] == info, "Invalid array object.");
	   mGhostFreeIndex--;
	   if(info->arrayIndex != mGhostFreeIndex)
	   {
		  mGhostArray[mGhostFreeIndex]->arrayIndex = info->arrayIndex;
		  mGhostArray[info->arrayIndex] = mGhostArray[mGhostFreeIndex];
		  mGhostArray[mGhostFreeIndex] = info;
		  info->arrayIndex = mGhostFreeIndex;
	   }
	   //Assert(validateGhostArray(), "Invalid ghost array!");
	}

   /// Moves the specified GhostInfo from the free area into the range of the ghost array for zero updateMasks.
   inline void ghostPushFreeToZero(GhostInfo *info)
	{
	   Assert(info->arrayIndex >= mGhostFreeIndex, "Out of range arrayIndex.");
	   Assert(mGhostArray[info->arrayIndex] == info, "Invalid array object.");
	   if(info->arrayIndex != mGhostFreeIndex)
	   {
		  mGhostArray[mGhostFreeIndex]->arrayIndex = info->arrayIndex;
		  mGhostArray[info->arrayIndex] = mGhostArray[mGhostFreeIndex];
		  mGhostArray[mGhostFreeIndex] = info;
		  info->arrayIndex = mGhostFreeIndex;
	   }
	   mGhostFreeIndex++;
	   //Assert(validateGhostArray(), "Invalid ghost array!");
	}

	/// Returns the client-side ghostIndex of the specified server object, or -1 if the object is not available on the client.
   S32 getGhostIndex(NetObject *object)
   {
	   if(!obj)
		  return -1;
	   if(!doesGhostFrom())
		  return obj->mNetIndex;
	   S32 index = obj->getHashId() & GhostLookupTableMask;

	   for(GhostInfo *gptr = mGhostLookupTable[index]; gptr; gptr = gptr->nextLookupInfo)
	   {
		  if(gptr->obj == obj && (gptr->flags & (GhostInfo::KillingGhost | GhostInfo::Ghosting | GhostInfo::NotYetGhosted | GhostInfo::KillGhost)) == 0)
			 return gptr->index;
	   }
	   return -1;
	}


   /// Returns true if the object is available on the client.
   bool isGhostAvailable(NetObject *object) { return getGhostIndex(object) != -1; }

	/// Stops ghosting objects from this GhostConnection to the remote host, which causes all ghosts to be destroyed on the client.
   void resetGhosting()
	{
	   if(!doesGhostFrom())
		  return;
	   // stop all ghosting activity
	   // send a message to the other side notifying of this
	   
	   mGhosting = false;
	   mScoping = false;
	   rpcEndGhosting();
	   mGhostingSequence++;
	   clearGhostInfo();
	   //Assert(validateGhostArray(), "Invalid ghost array!");
	}
   
	/// Begins ghosting objects from this GhostConnection to the remote host, starting with the GhostAlways objects.
   void activateGhosting()
   {
	   if(!doesGhostFrom())
		  return;

	   mGhostingSequence++;
	   TorqueLogMessageFormatted(LogGhostConnection, ("Ghosting activated - %d", mGhostingSequence));
	   
	   Assert((mGhostFreeIndex == 0) && (mGhostZeroUpdateIndex == 0), "Error: ghosts in the ghost list before activate.");
	   
	   // iterate through the ghost always objects and InScope them...
	   // also post em all to the other side.

	   S32 j;
	   for(j = 0; j < MaxGhostCount; j++)
	   {
		  mGhostArray[j] = mGhostRefs + j;
		  mGhostArray[j]->arrayIndex = j;
	   }
	   mScoping = true; // so that objectInScope will work

	   rpcStartGhosting(mGhostingSequence);
	   //Assert(validateGhostArray(), "Invalid ghost array!");
	}

   bool isGhosting() { return mGhosting; } ///< Returns true if this connection is currently ghosting objects to the remote host.

   /// Notifies the GhostConnection that the specified GhostInfo should no longer be scoped to the client.
   void detachObject(GhostInfo *info)
	{
	   // mark it for ghost killin'
	   info->flags |= GhostInfo::KillGhost;

	   // if the mask is in the zero range, we've got to move it up...
	   if(!info->updateMask)
	   {
		  info->updateMask = 0xFFFFFFFF;
		  ghostPushNonZero(info);
	   }
	   if(info->obj)
	   {
		  if(info->prevObjectRef)
			 info->prevObjectRef->nextObjectRef = info->nextObjectRef;
		  else
			 info->obj->mFirstObjectRef = info->nextObjectRef;
		  if(info->nextObjectRef)
			 info->nextObjectRef->prevObjectRef = info->prevObjectRef;
		  // remove it from the lookup table
		  
		  U32 id = info->obj->getHashId();
		  for(GhostInfo **walk = &mGhostLookupTable[id & GhostLookupTableMask]; *walk; walk = &((*walk)->nextLookupInfo))
		  {
			 GhostInfo *temp = *walk;
			 if(temp == info)
			 {
				*walk = temp->nextLookupInfo;
				break;
			 }
		  }
		  info->prevObjectRef = info->nextObjectRef = NULL;
		  info->obj = NULL;
	   }
	}

   /// RPC from server to client before the GhostAlwaysObjects are transmitted
   TNL_DECLARE_RPC(rpcStartGhosting, (U32 sequence));
	TNL_IMPLEMENT_RPC(GhostConnection, rpcStartGhosting, 
					  (U32 sequence), (sequence),
		  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
	{
	   TorqueLogMessageFormatted(LogGhostConnection, ("Got GhostingStarting %d", sequence));

	   if(!doesGhostTo())
	   {
		  setLastError("Invalid packet.");
		  return;
	   }
	   onStartGhosting();
	   rpcReadyForNormalGhosts(sequence);
	}

   /// RPC from client to server sent when the client receives the rpcGhostAlwaysActivated
   TNL_DECLARE_RPC(rpcReadyForNormalGhosts, (U32 sequence));
	TNL_IMPLEMENT_RPC(GhostConnection, rpcReadyForNormalGhosts, 
					  (U32 sequence), (sequence),
		  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
	{
	   TorqueLogMessageFormatted(LogGhostConnection, ("Got ready for normal ghosts %d %d", sequence, mGhostingSequence));
	   if(!doesGhostFrom())
	   {
		  setLastError("Invalid packet.");
		  return;
	   }
	   if(sequence != mGhostingSequence)
		  return;
	   mGhosting = true;
	}


   /// RPC from server to client sent to notify that ghosting should stop
   TNL_DECLARE_RPC(rpcEndGhosting, ());
	TNL_IMPLEMENT_RPC(GhostConnection, rpcEndGhosting, (), (),
		  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
	{
	   if(!doesGhostTo())
	   {
		  setLastError("Invalid packet.");
		  return;
	   }
	   deleteLocalGhosts();
	   onEndGhosting();
	}
};

//----------------------------------------------------------------------------

/// Each GhostInfo structure tracks the state of a single NetObject's ghost for a single GhostConnection.
struct GhostInfo
{
   // NOTE:
   // if the size of this structure changes, the
   // NetConnection::getGhostIndex function MUST be changed
   // to reflect.

   NetObject *obj; ///< The real object on the server.
   U32 updateMask; ///< The current out-of-date state mask for the object for this connection.
   GhostConnection::GhostRef *lastUpdateChain; ///< The GhostRef for this object in the last packet it was updated in,
                                               ///   or NULL if that last packet has been notified yet.
   GhostInfo *nextObjectRef;  ///< Next GhostInfo for this object in the doubly linked list of GhostInfos across
                              ///  all connections that scope this object
   GhostInfo *prevObjectRef;  ///< Previous GhostInfo for this object in the doubly linked list of GhostInfos across
                              ///  all connections that scope this object

   GhostConnection *connection; ///< The connection that owns this GhostInfo
   GhostInfo *nextLookupInfo;   ///< Next GhostInfo in the hash table for NetObject*->GhostInfo*
   U32 updateSkipCount;         ///< How many times this object has NOT been updated in writePacket

   U32 flags;      ///< Current flag status of this object for this connection.
   F32 priority;   ///< Priority for the update of this object, computed after the scoping process has run.
   U32 index;      ///< Fixed index of the object in the mGhostRefs array for the connection, and the ghostId of the object on the client.
   S32 arrayIndex; ///< Position of the object in the mGhostArray for the connection, which changes as the object is pushed to zero, non-zero and free.

    enum Flags
    {
      InScope = Bit(0),             ///< This GhostInfo's NetObject is currently in scope for this connection.
      ScopeLocalAlways = Bit(1),    ///< This GhostInfo's NetObject is always in scope for this connection.
      NotYetGhosted = Bit(2),       ///< This GhostInfo's NetObject has not been sent to or constructed on the remote host.
      Ghosting = Bit(3),            ///< This GhostInfo's NetObject has been sent to the client, but the packet it was sent in hasn't been acked yet.
      KillGhost = Bit(4),           ///< The ghost of this GhostInfo's NetObject should be destroyed ASAP.
      KillingGhost = Bit(5),        ///< The ghost of this GhostInfo's NetObject is in the process of being destroyed.

      /// Flag mask - if any of these are set, the object is not yet available for ghost ID lookup.
      NotAvailable = (NotYetGhosted | Ghosting | KillGhost | KillingGhost),
    };
};
