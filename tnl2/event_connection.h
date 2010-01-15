/// event_connection is a net_connection subclass used for sending guaranteed and unguaranteed event packages across a connection.
///
/// The event_connection is responsible for transmitting NetEvents over the wire.  It deals with ensuring that the various types of NetEvents are delivered appropriately, and with notifying the event of its delivery status.
///
/// The event_connection is mainly accessed via postNetEvent(), which accepts NetEvents.
///
/// @see net_event for a more thorough explanation of how to use events.

class event_connection : public net_connection
{
	typedef net_connection parent;
	
	/// event_note associates a single event posted to a connection with a sequence number for ordered processing
	struct event_note
	{
		ref_ptr<net_event> _event; ///< A safe reference to the event
		S32 _sequence_count; ///< the sequence number of this event for ordering
		event_note *_next_event; ///< The next event either on the connection or on the packet_notify
	};
public:
	/// event_packet_notify tracks all the events sent with a single packet
	struct event_packet_notify : public net_connection::packet_notify
	{
		event_note *event_list; ///< linked list of events sent with this packet
		event_packet_notify() { event_list = NULL; }
	};
	
	event_connection()
	{
		// event management data:
		
		_notify_event_list = NULL;
		_send_event_queue_head = NULL;
		_send_event_queue_tail = NULL;
		_unordered_send_event_queue_head = NULL;
		_unordered_send_event_queue_tail = NULL;
		_wait_seq_events = NULL;
		
		_next_send_event_sequence = first_valid_send_event_sequence;
		_next_receive_event_sequence = first_valid_send_event_sequence;
		_last_acked_event_sequence = -1;
		_event_class_count = 0;
		_event_class_bit_size = 0;
	}
	
	~event_connection()
	{
		while(_notify_event_list)
		{
			event_note *temp = _notify_event_list;
			_notify_event_list = temp->_next_event;
			
			temp->_event->notify_delivered(this, true);
			delete temp;
		}
		while(_unordered_send_event_queue_head)
		{
			event_note *temp = _unordered_send_event_queue_head;
			_unordered_send_event_queue_head = temp->_next_event;
			
			temp->_event->notify_delivered(this, true);
		}
		while(_send_event_queue_head)
		{
			event_note *temp = _send_event_queue_head;
			_send_event_queue_head = temp->_next_event;
			
			temp->_event->notify_delivered(this, true);
		}
	}
	
protected:
	enum 
	{
		debug_checksum = 0xF00DBAAD,
		bit_stream_position_bit_size = 16,
	};
	
	/// Allocates a packet_notify for this connection
	packet_notify *alloc_notify() { return new event_packet_notify; }
	
	/// Override processing to requeue any guaranteed events in the packet that was dropped
	void packet_dropped(packet_notify *notify)
	{
		parent::packet_dropped(pnotify);
		event_packet_notify *notify = static_cast<event_packet_notify *>(pnotify);
		
		event_note *walk = notify->event_list;
		event_note **insert_list = &_send_event_queue_head;
		event_note *temp;
		
		while(walk)
		{
			switch(walk->_event->_guarantee_type)
			{
				case net_event::guaranteed_ordered:
					// It was a guaranteed ordered packet, reinsert it back into
					// _send_event_queue_head in the right place (based on seq numbers)
					
					TorqueLogMessageFormatted(LogEventConnection, ("event_connection %d: DroppedGuaranteed - %d", get_torque_connection(), walk->_sequence_count));
					while(*insert_list && (*insert_list)->_sequence_count < walk->_sequence_count)
						insert_list = &((*insert_list)->_next_event);
					
					temp = walk->_next_event;
					walk->_next_event = *insert_list;
					if(!walk->_next_event)
						_send_event_queue_tail = walk;
					*insert_list = walk;
					insert_list = &(walk->_next_event);
					walk = temp;
					break;
				case net_event::guaranteed:
					// It was a guaranteed packet, put it at the top of
					// _unordered_send_event_queue_head.
					temp = walk->_next_event;
					walk->_next_event = _unordered_send_event_queue_head;
					_unordered_send_event_queue_head = walk;
					if(!walk->_next_event)
						_unordered_send_event_queue_tail = walk;
					walk = temp;
					break;
				case net_event::unguaranteed:
					// Or else it was an unguaranteed packet, notify that
					// it was _not_ delivered and blast it.
					walk->_event->notify_delivered(this, false);
					temp = walk->_next_event;
					delete walk;
					walk = temp;
			}
		}
	}
	
	/// Override processing to notify for delivery and dereference any events sent in the packet
	void packet_received(packet_notify *notify)
	{
		parent::packet_received(pnotify);
		
		event_packet_notify *notify = static_cast<event_packet_notify *>(pnotify);
		
		event_note *walk = notify->event_list;
		event_note **note_list = &_notify_event_list;
		
		while(walk)
		{
			event_note *next = walk->_next_event;
			if(walk->_event->_guarantee_type != net_event::guaranteed_ordered)
			{
				walk->_event->notify_delivered(this, true);
				delete walk;
				walk = next;
			}
			else
			{
				while(*note_list && (*note_list)->_sequence_count < walk->_sequence_count)
					note_list = &((*note_list)->_next_event);
				
				walk->_next_event = *note_list;
				*note_list = walk;
				note_list = &walk->_next_event;
				walk = next;
			}
		}
		while(_notify_event_list && _notify_event_list->_sequence_count == _last_acked_event_sequence + 1)
		{
			_last_acked_event_sequence++;
			event_note *next = _notify_event_list->_next_event;
			TorqueLogMessageFormatted(LogEventConnection, ("event_connection %d: NotifyDelivered - %d", get_torque_connection(), _notify_event_list->_sequence_count));
			_notify_event_list->_event->notify_delivered(this, true);
			delete _notify_event_list;
			_notify_event_list = next;
		}
	}
	
	/// Writes pending events into the packet, and attaches them to the packet_notify
	void write_packet(bit_stream *bstream, packet_notify *notify)
	{
		parent::write_packet(bstream, pnotify);
		event_packet_notify *notify = static_cast<event_packet_notify *>(pnotify);
		
		if(debug_connection())
			bstream->writeInt(debug_checksum, 32);
		
		event_note *packet_queue_head = NULL, *packet_queue_tail = NULL;
		
		while(_unordered_send_event_queue_head)
		{
			if(bstream->isFull())
				break;
			// get the first event
			event_note *ev = _unordered_send_event_queue_head;
			
			bstream->writeFlag(true);
			S32 start = bstream->getBitPosition();
			
			if(mConnectionParameters.mDebugObjectSizes)
				bstream->advanceBitPosition(bit_stream_position_bit_size);
			
			S32 classId = NetClassRegistry::GetClassIndexOfObject(ev->_event, getNetClassGroup());
			bstream->writeInt(classId, _event_class_bit_size);
			
			ev->_event->pack(this, bstream);
			TorqueLogMessageFormatted(LogEventConnection, ("event_connection %s: WroteEvent %s - %d bits", getNetAddressString().c_str(), ev->_event->get_debug_name(), bstream->getBitPosition() - start));
			
			if(mConnectionParameters.mDebugObjectSizes)
				bstream->writeIntAt(bstream->getBitPosition(), bit_stream_position_bit_size, start);
			
			if(bstream->getBitSpaceAvailable() < MinimumPaddingBits)
			{
				// rewind to before the event, and break out of the loop:
				bstream->setBitPosition(start - 1);
				bstream->clearError();
				break;
			}
			
			// dequeue the event and add this event onto the packet queue
			_unordered_send_event_queue_head = ev->_next_event;
			ev->_next_event = NULL;
			
			if(!packet_queue_head)
				packet_queue_head = ev;
			else
				packet_queue_tail->_next_event = ev;
			packet_queue_tail = ev;
		}
		
		bstream->writeFlag(false);   
		S32 prevSeq = -2;
		
		while(_send_event_queue_head)
		{
			if(bstream->isFull())
				break;
			
			// if the event window is full, stop processing
			if(_send_event_queue_head->_sequence_count > _last_acked_event_sequence + 126)
				break;
			
			// get the first event
			event_note *ev = _send_event_queue_head;
			S32 eventStart = bstream->getBitPosition();
			
			bstream->writeFlag(true);
			
			if(!bstream->writeFlag(ev->_sequence_count == prevSeq + 1))
				bstream->writeInt(ev->_sequence_count, 7);
			prevSeq = ev->_sequence_count;
			
			if(mConnectionParameters.mDebugObjectSizes)
				bstream->advanceBitPosition(bit_stream_position_bit_size);
			
			S32 start = bstream->getBitPosition();
			
			S32 classId = NetClassRegistry::GetClassIndexOfObject(ev->_event, getNetClassGroup());
			bstream->writeInt(classId, _event_class_bit_size);
			
			ev->_event->pack(this, bstream);
			TorqueLogMessageFormatted(LogEventConnection, ("event_connection wrote class id: %d, name = %s", classId, ev->_event->getClassName()));
			
			NetClassRegistry::AddInitialUpdate(ev->_event, bstream->getBitPosition() - start);
			TorqueLogMessageFormatted(LogEventConnection, ("event_connection %s: WroteEvent %s - %d bits", getNetAddressString().c_str(), ev->_event->get_debug_name(), bstream->getBitPosition() - start));
			
			if(mConnectionParameters.mDebugObjectSizes)
				bstream->writeIntAt(bstream->getBitPosition(), bit_stream_position_bit_size, start - bit_stream_position_bit_size);
			
			if(bstream->getBitSpaceAvailable() < MinimumPaddingBits)
			{
				// rewind to before the event, and break out of the loop:
				bstream->setBitPosition(eventStart);
				bstream->clearError();
				break;
			}
			
			// dequeue the event:
			_send_event_queue_head = ev->_next_event;      
			ev->_next_event = NULL;
			if(!packet_queue_head)
				packet_queue_head = ev;
			else
				packet_queue_tail->_next_event = ev;
			packet_queue_tail = ev;
		}
		for(event_note *ev = packet_queue_head; ev; ev = ev->_next_event)
			ev->_event->notify_sent(this);
		
		notify->event_list = packet_queue_head;
		bstream->writeFlag(0);
	}
	
	/// Reads events from the stream, and queues them for processing
	void readPacket(bit_stream *bstream)
	{
		parent::readPacket(bstream);
		
		if(mConnectionParameters.mDebugObjectSizes)
		{
			U32 sum = bstream->readInt(32);
			Assert(sum == debug_checksum, "Invalid checksum.");
		}
		
		S32 prevSeq = -2;
		event_note **waitInsert = &_wait_seq_events;
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
				endingPosition = bstream->readInt(bit_stream_position_bit_size);
			
			S32 start = bstream->getBitPosition();
			U32 classId = bstream->readInt(_event_class_bit_size);
			if(classId >= _event_class_count)
			{
				setLastError("Invalid packet.");
				return;
			}
			net_event *evt = (net_event *) NetClassRegistry::Create(getNetClassGroup(), NetClassTypeEvent, classId);
			if(!evt)
			{
				setLastError("Invalid packet.");
				return;
			}
			TorqueLogMessageFormatted(LogEventConnection, ("event_connection got class id: %d, name = %s", classId, evt->getClassName()));
			
			// check if the direction this event moves is a valid direction.
			if(   (evt->get_event_direction() == net_event::DirUnset)
			   || (evt->get_event_direction() == net_event::DirServerToClient && isConnectionToClient())
			   || (evt->get_event_direction() == net_event::DirClientToServer && isConnectionToServer()) )
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
			seq |= (_next_receive_event_sequence & ~0x7F);
			if(seq < _next_receive_event_sequence)
				seq += 128;
			
			event_note *note = mEventNoteChunker.allocate();
			note->_event = evt;
			note->_sequence_count = seq;
			TorqueLogMessageFormatted(LogEventConnection, ("event_connection %s: RecvdGuaranteed %d", getNetAddressString().c_str(), seq));
			
			while(*waitInsert && (*waitInsert)->_sequence_count < seq)
				waitInsert = &((*waitInsert)->_next_event);
			
			note->_next_event = *waitInsert;
			*waitInsert = note;
			waitInsert = &(note->_next_event);
		}
		while(_wait_seq_events && _wait_seq_events->_sequence_count == _next_receive_event_sequence)
		{
			_next_receive_event_sequence++;
			event_note *temp = _wait_seq_events;
			_wait_seq_events = temp->_next_event;
			
			TorqueLogMessageFormatted(LogEventConnection, ("event_connection %s: ProcessGuaranteed %d", getNetAddressString().c_str(), temp->_sequence_count));
			processEvent(temp->_event);
			mEventNoteChunker.deallocate(temp);
			if(!mErrorString.isEmpty())
				return;
		}
	}
	
	/// Returns true if there are events pending that should be sent across the wire
	virtual bool isDataToTransmit()
	{
		return _unordered_send_event_queue_head || _send_event_queue_head || parent::isDataToTransmit();
	}
	
	/// Dispatches an event
	void processEvent(net_event *theEvent)
	{
		if(getConnectionState() == net_connection::Connected)
			theEvent->process(this);
	}
	
	
	//----------------------------------------------------------------
	// event manager functions/code:
	//----------------------------------------------------------------
	
private:
	static PoolAllocator<event_note> mEventNoteChunker; ///< Quick memory allocator for net event notes
	
	event_note *_send_event_queue_head; ///< Head of the list of events to be sent to the remote host
	event_note *_send_event_queue_tail; ///< Tail of the list of events to be sent to the remote host.  New events are tagged on to the end of this list
	event_note *_unordered_send_event_queue_head; ///< Head of the list of events sent without ordering information
	event_note *_unordered_send_event_queue_tail; ///< Tail of the list of events sent without ordering information
	event_note *_wait_seq_events;   ///< List of ordered events on the receiving host that are waiting on previous sequenced events to arrive.
	event_note *_notify_event_list; ///< Ordered list of events on the sending host that are waiting for receipt of processing on the client.
	
	S32 _next_send_event_sequence; ///< The next sequence number for an ordered event sent through this connection
	S32 _next_receive_event_sequence; ///< The next receive event sequence to process
	S32 _last_acked_event_sequence; ///< The last event the remote host is known to have processed
	
	enum {
		InvalidSendEventSeq = -1,
		first_valid_send_event_sequence = 0
	};
	
protected:
	U32 _event_class_count; ///< Number of net_event classes supported by this connection
	U32 _event_class_bit_size; ///< Bit field width of net_event class count.
	U32 mEventClassVersion; ///< The highest version number of events on this connection.
	
	/// Writes the net_event class count into the stream, so that the remote host can negotiate a class count for the connection
	void writeConnectRequest(bit_stream *stream)
	{
		parent::writeConnectRequest(stream);
		U32 classCount = NetClassRegistry::GetClassCount(getNetClassGroup(), NetClassTypeEvent);
		write(*stream, classCount);
	}
	
	/// Reads the net_event class count max that the remote host is requesting.  If this host has MORE net_event classes declared, the _event_class_count  is set to the requested count, and is verified to lie on a boundary between versions.
	bool readConnectRequest(bit_stream *stream, const char **errorString)
	{
		if(!parent::readConnectRequest(stream, errorString))
			return false;
		
		U32 classCount;
		read(*stream, &classCount);
		
		U32 myCount = NetClassRegistry::GetClassCount(getNetClassGroup(), NetClassTypeEvent);
		if(myCount <= classCount)
			_event_class_count = myCount;
		else
		{
			_event_class_count = classCount;
			if(!NetClassRegistry::IsVersionBorderCount(getNetClassGroup(), NetClassTypeEvent, _event_class_count))
				return false;
		}
		if(!_event_class_count)
			return false;
		
		mEventClassVersion = NetClassRegistry::GetClassVersion(getNetClassGroup(), NetClassTypeEvent, _event_class_count-1);
		_event_class_bit_size = getNextBinaryLog2(_event_class_count);
		return true;
	}
	
	
	/// Writes the negotiated net_event class count into the stream.   
	void writeConnectAccept(bit_stream *stream)
	{
		parent::writeConnectAccept(stream);
		write(*stream, _event_class_count);
	}
	
	/// Reads the negotiated net_event class count from the stream and validates that it is on a boundary between versions.
	bool readConnectAccept(bit_stream *stream, const char **errorString)
	{
		if(!parent::readConnectAccept(stream, errorString))
			return false;
		
		read(*stream, &_event_class_count);
		
		U32 myCount = NetClassRegistry::GetClassCount(getNetClassGroup(), NetClassTypeEvent);
		if(_event_class_count > myCount)
			return false;
		
		if(!NetClassRegistry::IsVersionBorderCount(getNetClassGroup(), NetClassTypeEvent, _event_class_count))
			return false;
		
		_event_class_bit_size = getNextBinaryLog2(_event_class_count);
		return true;
	}
	
	
public:
	/// returns the highest event version number supported on this connection.
	U32 getEventClassVersion() { return mEventClassVersion; }
	
	/// Posts a net_event for processing on the remote host
	bool postNetEvent(net_event *event)
	{   
		S32 classId = NetClassRegistry::GetClassIndexOfObject(theEvent, getNetClassGroup());
		if(U32(classId) >= _event_class_count && getConnectionState() == Connected)
			return false;
		
		theEvent->notify_posted(this);
		
		event_note *event = mEventNoteChunker.allocate();
		event->_event = theEvent;
		event->_next_event = NULL;
		
		if(event->_event->_guarantee_type == net_event::guaranteed_ordered)
		{
			event->_sequence_count = _next_send_event_sequence++;
			if(!_send_event_queue_head)
				_send_event_queue_head = event;
			else
				_send_event_queue_tail->_next_event = event;
			_send_event_queue_tail = event;
		}
		else
		{
			event->_sequence_count = InvalidSendEventSeq;
			if(!_unordered_send_event_queue_head)
				_unordered_send_event_queue_head = event;
			else
				_unordered_send_event_queue_tail->_next_event = event;
			_unordered_send_event_queue_tail = event;
		}
		return true;
	}
};

