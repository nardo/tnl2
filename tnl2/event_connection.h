PoolAllocator<EventConnection::EventNote> EventConnection::mEventNoteChunker;


/// EventConnection is a NetConnection subclass used for sending guaranteed and unguaranteed
/// event packages across a connection.
///
/// The EventConnection is responsible for transmitting NetEvents over the wire.
/// It deals with ensuring that the various types of NetEvents are delivered appropriately,
/// and with notifying the event of its delivery status.
///
/// The EventConnection is mainly accessed via postNetEvent(), which accepts NetEvents.
///
/// @see NetEvent for a more thorough explanation of how to use events.

class EventConnection : public NetConnection
{
	typedef NetConnection Parent;
	
	/// EventNote associates a single event posted to a connection with a sequence number for ordered processing
	struct EventNote
	{
		RefPtr<NetEvent> mEvent; ///< A safe reference to the event
		S32 mSeqCount; ///< the sequence number of this event for ordering
		EventNote *mNextEvent; ///< The next event either on the connection or on the PacketNotify
	};
public:
	/// EventPacketNotify tracks all the events sent with a single packet
	struct EventPacketNotify : public NetConnection::PacketNotify
	{
		EventNote *eventList; ///< linked list of events sent with this packet
		EventPacketNotify() { eventList = NULL; }
	};
	
	EventConnection()
	{
		// event management data:
		
		mNotifyEventList = NULL;
		mSendEventQueueHead = NULL;
		mSendEventQueueTail = NULL;
		mUnorderedSendEventQueueHead = NULL;
		mUnorderedSendEventQueueTail = NULL;
		mWaitSeqEvents = NULL;
		
		mNextSendEventSeq = FirstValidSendEventSeq;
		mNextRecvEventSeq = FirstValidSendEventSeq;
		mLastAckedEventSeq = -1;
		mEventClassCount = 0;
		mEventClassBitSize = 0;
	}
	
	~EventConnection()
	{
		while(mNotifyEventList)
		{
			EventNote *temp = mNotifyEventList;
			mNotifyEventList = temp->mNextEvent;
			
			temp->mEvent->notifyDelivered(this, true);
			mEventNoteChunker.deallocate(temp);
		}
		while(mUnorderedSendEventQueueHead)
		{
			EventNote *temp = mUnorderedSendEventQueueHead;
			mUnorderedSendEventQueueHead = temp->mNextEvent;
			
			temp->mEvent->notifyDelivered(this, true);
			mEventNoteChunker.deallocate(temp);
		}
		while(mSendEventQueueHead)
		{
			EventNote *temp = mSendEventQueueHead;
			mSendEventQueueHead = temp->mNextEvent;
			
			temp->mEvent->notifyDelivered(this, true);
			mEventNoteChunker.deallocate(temp);
		}
	}
	
protected:
	enum DebugConstants
	{
		DebugChecksum = 0xF00DBAAD,
		BitStreamPosBitSize = 16,
	};
	
	/// Allocates a PacketNotify for this connection
	PacketNotify *allocNotify() { return new EventPacketNotify; }
	
	/// Override processing to requeue any guaranteed events in the packet that was dropped
	void packet_dropped(PacketNotify *notify)
	{
		Parent::packet_dropped(pnotify);
		EventPacketNotify *notify = static_cast<EventPacketNotify *>(pnotify);
		
		EventNote *walk = notify->eventList;
		EventNote **insertList = &mSendEventQueueHead;
		EventNote *temp;
		
		while(walk)
		{
			switch(walk->mEvent->mGuaranteeType)
			{
				case NetEvent::GuaranteedOrdered:
					// It was a guaranteed ordered packet, reinsert it back into
					// mSendEventQueueHead in the right place (based on seq numbers)
					
					TorqueLogMessageFormatted(LogEventConnection, ("EventConnection %s: DroppedGuaranteed - %d", getNetAddressString().c_str(), walk->mSeqCount));
					while(*insertList && (*insertList)->mSeqCount < walk->mSeqCount)
						insertList = &((*insertList)->mNextEvent);
					
					temp = walk->mNextEvent;
					walk->mNextEvent = *insertList;
					if(!walk->mNextEvent)
						mSendEventQueueTail = walk;
					*insertList = walk;
					insertList = &(walk->mNextEvent);
					walk = temp;
					break;
				case NetEvent::Guaranteed:
					// It was a guaranteed packet, put it at the top of
					// mUnorderedSendEventQueueHead.
					temp = walk->mNextEvent;
					walk->mNextEvent = mUnorderedSendEventQueueHead;
					mUnorderedSendEventQueueHead = walk;
					if(!walk->mNextEvent)
						mUnorderedSendEventQueueTail = walk;
					walk = temp;
					break;
				case NetEvent::Unguaranteed:
					// Or else it was an unguaranteed packet, notify that
					// it was _not_ delivered and blast it.
					walk->mEvent->notifyDelivered(this, false);
					temp = walk->mNextEvent;
					mEventNoteChunker.deallocate(walk);
					walk = temp;
			}
		}
	}
	
	/// Override processing to notify for delivery and dereference any events sent in the packet
	void packet_received(PacketNotify *notify)
	{
		Parent::packet_received(pnotify);
		
		EventPacketNotify *notify = static_cast<EventPacketNotify *>(pnotify);
		
		EventNote *walk = notify->eventList;
		EventNote **noteList = &mNotifyEventList;
		
		while(walk)
		{
			EventNote *next = walk->mNextEvent;
			if(walk->mEvent->mGuaranteeType != NetEvent::GuaranteedOrdered)
			{
				walk->mEvent->notifyDelivered(this, true);
				mEventNoteChunker.deallocate(walk);
				walk = next;
			}
			else
			{
				while(*noteList && (*noteList)->mSeqCount < walk->mSeqCount)
					noteList = &((*noteList)->mNextEvent);
				
				walk->mNextEvent = *noteList;
				*noteList = walk;
				noteList = &walk->mNextEvent;
				walk = next;
			}
		}
		while(mNotifyEventList && mNotifyEventList->mSeqCount == mLastAckedEventSeq + 1)
		{
			mLastAckedEventSeq++;
			EventNote *next = mNotifyEventList->mNextEvent;
			TorqueLogMessageFormatted(LogEventConnection, ("EventConnection %s: NotifyDelivered - %d", getNetAddressString().c_str(), mNotifyEventList->mSeqCount));
			mNotifyEventList->mEvent->notifyDelivered(this, true);
			mEventNoteChunker.deallocate(mNotifyEventList);
			mNotifyEventList = next;
		}
	}
	
	/// Writes pending events into the packet, and attaches them to the PacketNotify
	void writePacket(BitStream *bstream, PacketNotify *notify)
	{
		Parent::writePacket(bstream, pnotify);
		EventPacketNotify *notify = static_cast<EventPacketNotify *>(pnotify);
		
		if(mConnectionParameters.mDebugObjectSizes)
			bstream->writeInt(DebugChecksum, 32);
		
		EventNote *packQueueHead = NULL, *packQueueTail = NULL;
		
		while(mUnorderedSendEventQueueHead)
		{
			if(bstream->isFull())
				break;
			// get the first event
			EventNote *ev = mUnorderedSendEventQueueHead;
			
			bstream->writeFlag(true);
			S32 start = bstream->getBitPosition();
			
			if(mConnectionParameters.mDebugObjectSizes)
				bstream->advanceBitPosition(BitStreamPosBitSize);
			
			S32 classId = NetClassRegistry::GetClassIndexOfObject(ev->mEvent, getNetClassGroup());
			bstream->writeInt(classId, mEventClassBitSize);
			
			ev->mEvent->pack(this, bstream);
			TorqueLogMessageFormatted(LogEventConnection, ("EventConnection %s: WroteEvent %s - %d bits", getNetAddressString().c_str(), ev->mEvent->getDebugName(), bstream->getBitPosition() - start));
			
			if(mConnectionParameters.mDebugObjectSizes)
				bstream->writeIntAt(bstream->getBitPosition(), BitStreamPosBitSize, start);
			
			if(bstream->getBitSpaceAvailable() < MinimumPaddingBits)
			{
				// rewind to before the event, and break out of the loop:
				bstream->setBitPosition(start - 1);
				bstream->clearError();
				break;
			}
			
			// dequeue the event and add this event onto the packet queue
			mUnorderedSendEventQueueHead = ev->mNextEvent;
			ev->mNextEvent = NULL;
			
			if(!packQueueHead)
				packQueueHead = ev;
			else
				packQueueTail->mNextEvent = ev;
			packQueueTail = ev;
		}
		
		bstream->writeFlag(false);   
		S32 prevSeq = -2;
		
		while(mSendEventQueueHead)
		{
			if(bstream->isFull())
				break;
			
			// if the event window is full, stop processing
			if(mSendEventQueueHead->mSeqCount > mLastAckedEventSeq + 126)
				break;
			
			// get the first event
			EventNote *ev = mSendEventQueueHead;
			S32 eventStart = bstream->getBitPosition();
			
			bstream->writeFlag(true);
			
			if(!bstream->writeFlag(ev->mSeqCount == prevSeq + 1))
				bstream->writeInt(ev->mSeqCount, 7);
			prevSeq = ev->mSeqCount;
			
			if(mConnectionParameters.mDebugObjectSizes)
				bstream->advanceBitPosition(BitStreamPosBitSize);
			
			S32 start = bstream->getBitPosition();
			
			S32 classId = NetClassRegistry::GetClassIndexOfObject(ev->mEvent, getNetClassGroup());
			bstream->writeInt(classId, mEventClassBitSize);
			
			ev->mEvent->pack(this, bstream);
			TorqueLogMessageFormatted(LogEventConnection, ("EventConnection wrote class id: %d, name = %s", classId, ev->mEvent->getClassName()));
			
			NetClassRegistry::AddInitialUpdate(ev->mEvent, bstream->getBitPosition() - start);
			TorqueLogMessageFormatted(LogEventConnection, ("EventConnection %s: WroteEvent %s - %d bits", getNetAddressString().c_str(), ev->mEvent->getDebugName(), bstream->getBitPosition() - start));
			
			if(mConnectionParameters.mDebugObjectSizes)
				bstream->writeIntAt(bstream->getBitPosition(), BitStreamPosBitSize, start - BitStreamPosBitSize);
			
			if(bstream->getBitSpaceAvailable() < MinimumPaddingBits)
			{
				// rewind to before the event, and break out of the loop:
				bstream->setBitPosition(eventStart);
				bstream->clearError();
				break;
			}
			
			// dequeue the event:
			mSendEventQueueHead = ev->mNextEvent;      
			ev->mNextEvent = NULL;
			if(!packQueueHead)
				packQueueHead = ev;
			else
				packQueueTail->mNextEvent = ev;
			packQueueTail = ev;
		}
		for(EventNote *ev = packQueueHead; ev; ev = ev->mNextEvent)
			ev->mEvent->notifySent(this);
		
		notify->eventList = packQueueHead;
		bstream->writeFlag(0);
	}
	
	/// Reads events from the stream, and queues them for processing
	void readPacket(BitStream *bstream)
	{
		Parent::readPacket(bstream);
		
		if(mConnectionParameters.mDebugObjectSizes)
		{
			U32 sum = bstream->readInt(32);
			Assert(sum == DebugChecksum, "Invalid checksum.");
		}
		
		S32 prevSeq = -2;
		EventNote **waitInsert = &mWaitSeqEvents;
		bool unguaranteedPhase = true;
		
		while(true)
		{
			bool bit = bstream->readFlag();
			if(unguaranteedPhase && !bit)
			{
				unguaranteedPhase = false;
				bit = bstream->readFlag();
			}
			if(!unguaranteedPhase && !bit)
				break;
			
			S32 seq = -1;
			
			if(!unguaranteedPhase) // get the sequence
			{
				if(bstream->readFlag())
					seq = (prevSeq + 1) & 0x7f;
				else
					seq = bstream->readInt(7);
				prevSeq = seq;
			}
			
			U32 endingPosition;
			if(mConnectionParameters.mDebugObjectSizes)
				endingPosition = bstream->readInt(BitStreamPosBitSize);
			
			S32 start = bstream->getBitPosition();
			U32 classId = bstream->readInt(mEventClassBitSize);
			if(classId >= mEventClassCount)
			{
				setLastError("Invalid packet.");
				return;
			}
			NetEvent *evt = (NetEvent *) NetClassRegistry::Create(getNetClassGroup(), NetClassTypeEvent, classId);
			if(!evt)
			{
				setLastError("Invalid packet.");
				return;
			}
			TorqueLogMessageFormatted(LogEventConnection, ("EventConnection got class id: %d, name = %s", classId, evt->getClassName()));
			
			// check if the direction this event moves is a valid direction.
			if(   (evt->getEventDirection() == NetEvent::DirUnset)
			   || (evt->getEventDirection() == NetEvent::DirServerToClient && isConnectionToClient())
			   || (evt->getEventDirection() == NetEvent::DirClientToServer && isConnectionToServer()) )
			{
				setLastError("Invalid Packet.");
				return;
			}
			
			
			evt->unpack(this, bstream);
			if(!mErrorString.isEmpty())
				return;
			
			if(mConnectionParameters.mDebugObjectSizes)
			{
				AssertFormatted(endingPosition == bstream->getBitPosition(),
								("unpack did not match pack for event of class %s.",
								 evt->getClassName().c_str()) );
			}
			
			if(unguaranteedPhase)
			{
				processEvent(evt);
				delete evt;
				if(!mErrorString.isEmpty())
					return;
				continue;
			}
			seq |= (mNextRecvEventSeq & ~0x7F);
			if(seq < mNextRecvEventSeq)
				seq += 128;
			
			EventNote *note = mEventNoteChunker.allocate();
			note->mEvent = evt;
			note->mSeqCount = seq;
			TorqueLogMessageFormatted(LogEventConnection, ("EventConnection %s: RecvdGuaranteed %d", getNetAddressString().c_str(), seq));
			
			while(*waitInsert && (*waitInsert)->mSeqCount < seq)
				waitInsert = &((*waitInsert)->mNextEvent);
			
			note->mNextEvent = *waitInsert;
			*waitInsert = note;
			waitInsert = &(note->mNextEvent);
		}
		while(mWaitSeqEvents && mWaitSeqEvents->mSeqCount == mNextRecvEventSeq)
		{
			mNextRecvEventSeq++;
			EventNote *temp = mWaitSeqEvents;
			mWaitSeqEvents = temp->mNextEvent;
			
			TorqueLogMessageFormatted(LogEventConnection, ("EventConnection %s: ProcessGuaranteed %d", getNetAddressString().c_str(), temp->mSeqCount));
			processEvent(temp->mEvent);
			mEventNoteChunker.deallocate(temp);
			if(!mErrorString.isEmpty())
				return;
		}
	}
	
	/// Returns true if there are events pending that should be sent across the wire
	virtual bool isDataToTransmit()
	{
		return mUnorderedSendEventQueueHead || mSendEventQueueHead || Parent::isDataToTransmit();
	}
	
	/// Dispatches an event
	void processEvent(NetEvent *theEvent)
	{
		if(getConnectionState() == NetConnection::Connected)
			theEvent->process(this);
	}
	
	
	//----------------------------------------------------------------
	// event manager functions/code:
	//----------------------------------------------------------------
	
private:
	static PoolAllocator<EventNote> mEventNoteChunker; ///< Quick memory allocator for net event notes
	
	EventNote *mSendEventQueueHead;          ///< Head of the list of events to be sent to the remote host
	EventNote *mSendEventQueueTail;          ///< Tail of the list of events to be sent to the remote host.  New events are tagged on to the end of this list
	EventNote *mUnorderedSendEventQueueHead; ///< Head of the list of events sent without ordering information
	EventNote *mUnorderedSendEventQueueTail; ///< Tail of the list of events sent without ordering information
	EventNote *mWaitSeqEvents;   ///< List of ordered events on the receiving host that are waiting on previous sequenced events to arrive.
	EventNote *mNotifyEventList; ///< Ordered list of events on the sending host that are waiting for receipt of processing on the client.
	
	S32 mNextSendEventSeq;  ///< The next sequence number for an ordered event sent through this connection
	S32 mNextRecvEventSeq;  ///< The next receive event sequence to process
	S32 mLastAckedEventSeq; ///< The last event the remote host is known to have processed
	
	enum {
		InvalidSendEventSeq = -1,
		FirstValidSendEventSeq = 0
	};
	
protected:
	U32 mEventClassCount;      ///< Number of NetEvent classes supported by this connection
	U32 mEventClassBitSize;    ///< Bit field width of NetEvent class count.
	U32 mEventClassVersion;    ///< The highest version number of events on this connection.
	
	/// Writes the NetEvent class count into the stream, so that the remote
	/// host can negotiate a class count for the connection
	void writeConnectRequest(BitStream *stream)
	{
		Parent::writeConnectRequest(stream);
		U32 classCount = NetClassRegistry::GetClassCount(getNetClassGroup(), NetClassTypeEvent);
		write(*stream, classCount);
	}
	
	/// Reads the NetEvent class count max that the remote host is requesting.
	/// If this host has MORE NetEvent classes declared, the mEventClassCount
	/// is set to the requested count, and is verified to lie on a boundary between versions.
	bool readConnectRequest(BitStream *stream, const char **errorString)
	{
		if(!Parent::readConnectRequest(stream, errorString))
			return false;
		
		U32 classCount;
		read(*stream, &classCount);
		
		U32 myCount = NetClassRegistry::GetClassCount(getNetClassGroup(), NetClassTypeEvent);
		if(myCount <= classCount)
			mEventClassCount = myCount;
		else
		{
			mEventClassCount = classCount;
			if(!NetClassRegistry::IsVersionBorderCount(getNetClassGroup(), NetClassTypeEvent, mEventClassCount))
				return false;
		}
		if(!mEventClassCount)
			return false;
		
		mEventClassVersion = NetClassRegistry::GetClassVersion(getNetClassGroup(), NetClassTypeEvent, mEventClassCount-1);
		mEventClassBitSize = getNextBinaryLog2(mEventClassCount);
		return true;
	}
	
	
	/// Writes the negotiated NetEvent class count into the stream.   
	void writeConnectAccept(BitStream *stream)
	{
		Parent::writeConnectAccept(stream);
		write(*stream, mEventClassCount);
	}
	
	/// Reads the negotiated NetEvent class count from the stream and validates that it is on
	/// a boundary between versions.
	bool readConnectAccept(BitStream *stream, const char **errorString)
	{
		if(!Parent::readConnectAccept(stream, errorString))
			return false;
		
		read(*stream, &mEventClassCount);
		
		U32 myCount = NetClassRegistry::GetClassCount(getNetClassGroup(), NetClassTypeEvent);
		if(mEventClassCount > myCount)
			return false;
		
		if(!NetClassRegistry::IsVersionBorderCount(getNetClassGroup(), NetClassTypeEvent, mEventClassCount))
			return false;
		
		mEventClassBitSize = getNextBinaryLog2(mEventClassCount);
		return true;
	}
	
	
public:
	/// returns the highest event version number supported on this connection.
	U32 getEventClassVersion() { return mEventClassVersion; }
	
	/// Posts a NetEvent for processing on the remote host
	bool postNetEvent(NetEvent *event)
	{   
		S32 classId = NetClassRegistry::GetClassIndexOfObject(theEvent, getNetClassGroup());
		if(U32(classId) >= mEventClassCount && getConnectionState() == Connected)
			return false;
		
		theEvent->notifyPosted(this);
		
		EventNote *event = mEventNoteChunker.allocate();
		event->mEvent = theEvent;
		event->mNextEvent = NULL;
		
		if(event->mEvent->mGuaranteeType == NetEvent::GuaranteedOrdered)
		{
			event->mSeqCount = mNextSendEventSeq++;
			if(!mSendEventQueueHead)
				mSendEventQueueHead = event;
			else
				mSendEventQueueTail->mNextEvent = event;
			mSendEventQueueTail = event;
		}
		else
		{
			event->mSeqCount = InvalidSendEventSeq;
			if(!mUnorderedSendEventQueueHead)
				mUnorderedSendEventQueueHead = event;
			else
				mUnorderedSendEventQueueTail->mNextEvent = event;
			mUnorderedSendEventQueueTail = event;
		}
		return true;
	}
};

