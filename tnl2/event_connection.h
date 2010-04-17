/// event_connection is a net_connection subclass used for sending guaranteed and unguaranteed event packages across a connection.
///
/// The event_connection is responsible for transmitting NetEvents over the wire.  It deals with ensuring that the various types of NetEvents are delivered appropriately, and with notifying the event of its delivery status.
///
/// The event_connection is mainly accessed via postNetEvent(), which accepts NetEvents.
///
/// @see net_event for a more thorough explanation of how to use events.

enum rpc_direction {
	rpc_bidirectional, ///< This event can be sent from either the initiator or the host of the connection.
	rpc_host_to_initiator, ///< This event can only be sent from the host to the initiator
	rpc_initiator_to_host, ///< This event can only be sent from the initiator to the host
};

enum rpc_guarantee_type {
	rpc_guaranteed_ordered = 0, ///< Event delivery is guaranteed and will be processed in the order it was sent relative to other ordered events.
	rpc_guaranteed = 1, ///< Event delivery is guaranteed and will be processed in the order it was received.
	rpc_unguaranteed = 2 ///< Event delivery is not guaranteed - however, the event will remain ordered relative to other unguaranteed events.
};

class event_connection : public net_connection
{
public:
	typedef net_connection parent;
protected:
	struct rpc_record
	{
		uint32 method_hash;
		rpc_guarantee_type guarantee_type;
		rpc_direction direction;
		functor_creator *creator;
	};
	array<rpc_record> rpc_methods;

	/// event_note associates a single event posted to a connection with a sequence number for ordered processing
	struct event_note
	{
		ref_ptr<functor> _rpc; ///< A safe reference to the functor
		uint32 rpc_index; ///< index into rpc_methods array
		int32 _sequence_count; ///< the sequence number of this event for ordering
		event_note *_next_event; ///< The next event either on the connection or on the packet_notify
	};
	/// event_packet_notify tracks all the events sent with a single packet
	struct event_packet_notify : public net_connection::packet_notify
	{
		event_note *event_list; ///< linked list of events sent with this packet
		event_packet_notify() { event_list = NULL; }
	};
public:	
	template <typename signature> void register_rpc(signature the_method, rpc_guarantee_type guarantee_type, rpc_direction direction)
	{
		rpc_record the_record;
		the_record.creator = new functor_creator_decl<signature>(the_method);
		the_record.guarantee_type = guarantee_type;
		the_record.direction = direction;
		the_record.method_hash = hash_method(the_method);
		rpc_methods.push_back(the_record);
	}
	template <class T> void rpc(void (T::*method)())
	{
		uint32 method_hash = hash_method(method);
		functor_decl<void (T::*)()> *f = new functor_decl<void (T::*)()>(method);
		call_rpc(method_hash, f);
	}
	template <class T, class A> void rpc(void (T::*method)(A), A arg1)
	{
		uint32 method_hash = hash_method(method);
		functor_decl<void (T::*)(A)> *f = new functor_decl<void (T::*)(A)>(method);
		f->set(arg1);
		call_rpc(method_hash, f);
	}	
	template <class T, class A, class B> void rpc(void (T::*method)(A,B), A arg1, B arg2)
	{
		uint32 method_hash = hash_method(method);
		functor_decl<void (T::*)(A,B)> *f = new functor_decl<void (T::*)(A,B)>(method);
		f->set(arg1,arg2);
		call_rpc(method_hash, f);
	}
	
	/// Allocates a packet_notify for this connection
	packet_notify *alloc_notify() { return new event_packet_notify; }
	
	/// Override processing to requeue any guaranteed events in the packet that was dropped
	void packet_dropped(packet_notify *pnotify)
	{
		parent::packet_dropped(pnotify);
		event_packet_notify *notify = static_cast<event_packet_notify *>(pnotify);
		
		event_note *walk = notify->event_list;
		event_note **insert_list = &_send_event_queue_head;
		event_note *temp;
		
		while(walk)
		{
			switch(rpc_methods[walk->rpc_index].guarantee_type)
			{
				case rpc_guaranteed_ordered:
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
				case rpc_guaranteed:
					// It was a guaranteed packet, put it at the top of
					// _unordered_send_event_queue_head.
					temp = walk->_next_event;
					walk->_next_event = _unordered_send_event_queue_head;
					_unordered_send_event_queue_head = walk;
					if(!walk->_next_event)
						_unordered_send_event_queue_tail = walk;
					walk = temp;
					break;
				case rpc_unguaranteed:
					// Or else it was an unguaranteed packet, notify that
					// it was _not_ delivered and blast it.
					temp = walk->_next_event;
					delete walk;
					walk = temp;
			}
		}
	}
	
	/// Override processing to notify for delivery and dereference any events sent in the packet
	void packet_received(packet_notify *pnotify)
	{
		parent::packet_received(pnotify);
		
		event_packet_notify *notify = static_cast<event_packet_notify *>(pnotify);
		
		event_note *walk = notify->event_list;
		event_note **note_list = &_notify_event_list;
		
		while(walk)
		{
			event_note *next = walk->_next_event;
			if(rpc_methods[walk->rpc_index].guarantee_type != rpc_guaranteed_ordered)
			{
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
			delete _notify_event_list;
			_notify_event_list = next;
		}
	}
	
	/// Writes pending events into the packet, and attaches them to the packet_notify
	void write_packet(bit_stream &bstream, packet_notify *pnotify)
	{
		parent::write_packet(bstream, pnotify);
		event_packet_notify *notify = static_cast<event_packet_notify *>(pnotify);
		
		event_note *packet_queue_head = NULL, *packet_queue_tail = NULL;
		
		while(_unordered_send_event_queue_head)
		{
			if(bstream.is_full())
				break;
			// get the first event
			event_note *ev = _unordered_send_event_queue_head;
			
			bstream.write_bool(true);
			int32 start = bstream.get_bit_position();
			
			bstream.write_integer(ev->rpc_index, _rpc_id_bit_size);
			ev->_rpc->write(bstream);
			TorqueLogMessageFormatted(LogEventConnection, ("event_connection %d: WroteEvent %d - %d bits", get_torque_connection(), ev->rpc_index, bstream.get_bit_position() - start));
	
			if(bstream.get_bit_space_available() < minimum_padding_bits)
			{
				// rewind to before this RPC
				bstream.set_bit_position(start - 1);
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
		
		bstream.write_bool(false);   
		int32 previous_sequence = -2;
		
		while(_send_event_queue_head)
		{
			if(bstream.is_full())
				break;
			
			// if the event window is full, stop processing
			if(_send_event_queue_head->_sequence_count > _last_acked_event_sequence + 126)
				break;
			
			// get the first event
			event_note *ev = _send_event_queue_head;
			int32 eventStart = bstream.get_bit_position();
			
			bstream.write_bool(true);
			
			if(!bstream.write_bool(ev->_sequence_count == previous_sequence + 1))
				bstream.write_integer(ev->_sequence_count, 7);
			previous_sequence = ev->_sequence_count;

			int32 start = bstream.get_bit_position();
			bstream.write_integer(ev->rpc_index, _rpc_id_bit_size);
			
			ev->_rpc->write(bstream);
			TorqueLogMessageFormatted(LogEventConnection, ("event_connection %d: WroteEvent %d - %d bits", get_torque_connection(), ev->rpc_index, bstream.get_bit_position() - start));

			if(bstream.get_bit_space_available() < minimum_padding_bits)
			{
				// rewind to before the event, and break out of the loop:
				bstream.set_bit_position(eventStart);
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
		notify->event_list = packet_queue_head;
		bstream.write_bool(0);
	}
	
	/// Reads events from the stream, and queues them for processing
	void read_packet(bit_stream &bstream)
	{
		parent::read_packet(bstream);
		
		int32 previous_sequence = -2;
		event_note **wait_insert = &_wait_seq_events;
		bool unguaranteed_phase = true;
		
		while(true)
		{
			bool bit = bstream.read_bool();
			if(unguaranteed_phase && !bit)
			{
				unguaranteed_phase = false;
				bit = bstream.read_bool();
			}
			if(!unguaranteed_phase && !bit)
				break;
			
			int32 seq = -1;
			
			if(!unguaranteed_phase) // get the sequence
			{
				if(bstream.read_bool())
					seq = (previous_sequence + 1) & 0x7f;
				else
					seq = bstream.read_integer(7);
				previous_sequence = seq;
			}
			
			//int32 start = bstream.get_bit_position();
			uint32 rpc_index = bstream.read_integer(_rpc_id_bit_size);
			fflush(stdout); // FIXME
			if(rpc_index >= _rpc_count)
				assert(0); // FIXME: throw tnl_exception_invalid_packet;
			
			rpc_record &the_rpc = rpc_methods[rpc_index];
			
			functor *func = the_rpc.creator->create();
			
			// check if the direction this event moves is a valid direction.
			if(   (the_rpc.direction == rpc_initiator_to_host && is_connection_initiator())
			   || (the_rpc.direction == rpc_host_to_initiator && is_connection_host()) )
				assert(0); //throw tnl_exception_invalid_packet;
			func->read(bstream);
			
			if(unguaranteed_phase)
			{
				process_rpc(func);
				delete func;
				continue;
			}
			seq |= (_next_receive_event_sequence & ~0x7F);
			if(seq < _next_receive_event_sequence)
				seq += 128;
			
			event_note *note = new event_note;
			note->rpc_index = rpc_index;
			note->_rpc = func;
			note->_sequence_count = seq;
			TorqueLogMessageFormatted(LogEventConnection, ("event_connection %d: RecvdGuaranteed %d", get_torque_connection(), seq));
			
			while(*wait_insert && (*wait_insert)->_sequence_count < seq)
				wait_insert = &((*wait_insert)->_next_event);
			
			note->_next_event = *wait_insert;
			*wait_insert = note;
			wait_insert = &(note->_next_event);
		}
		while(_wait_seq_events && _wait_seq_events->_sequence_count == _next_receive_event_sequence)
		{
			_next_receive_event_sequence++;
			event_note *temp = _wait_seq_events;
			_wait_seq_events = temp->_next_event;
			
			TorqueLogMessageFormatted(LogEventConnection, ("event_connection %d: ProcessGuaranteed %d", get_torque_connection(), temp->_sequence_count));
			process_rpc(temp->_rpc);
			delete temp;
		}
	}
	
	/// Returns true if there are events pending that should be sent across the wire
	virtual bool is_data_to_transmit()
	{
		return _unordered_send_event_queue_head || _send_event_queue_head || parent::is_data_to_transmit();
	}
	
	/// Dispatches an event
	void process_rpc(functor *the_functor)
	{
		if(get_connection_state() == net_connection::state_established)
			the_functor->dispatch(this);
	}
	
	
	//----------------------------------------------------------------
	// event manager functions/code:
	//----------------------------------------------------------------
	
private:
protected:
	/// Writes the net_event class count into the stream, so that the remote host can negotiate a class count for the connection
	void write_connect_request(bit_stream &stream)
	{
		parent::write_connect_request(stream);
		_rpc_count = rpc_methods.size();
		core::write(stream, _rpc_count);
		_rpc_id_bit_size = get_next_binary_log(_rpc_count);
	}
	
	/// Reads the net_event class count max that the remote host is requesting.
	bool read_connect_request(bit_stream &stream, bit_stream &response_stream)
	{
		if(!parent::read_connect_request(stream, response_stream))
			return false;

		core::read(stream, _rpc_count);
		if(_rpc_count != rpc_methods.size())
			return false;
		
		_rpc_id_bit_size = get_next_binary_log(_rpc_count);
		return true;
	}
	
public:
	void call_rpc(uint32 method_hash, functor *the_functor)
	{
		uint32 rpc_index;
		for(rpc_index = 0 ; rpc_index< rpc_methods.size(); rpc_index++)
			if(rpc_methods[rpc_index].method_hash == method_hash)
				break;
		if(rpc_index == rpc_methods.size())
			return;

		rpc_record &record = rpc_methods[rpc_index];
		
		event_note *event = new event_note;
		event->_rpc = the_functor;
		event->_next_event = NULL;
		event->rpc_index = rpc_index;
		
		if(record.guarantee_type == rpc_guaranteed_ordered)
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
	}
	
	event_connection(bool is_initiator = false) : net_connection(is_initiator)
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
		_rpc_count = 0;
		_rpc_id_bit_size = 0;
	}
	
	~event_connection()
	{
		while(_notify_event_list)
		{
			event_note *temp = _notify_event_list;
			_notify_event_list = temp->_next_event;
			delete temp;
		}
		while(_unordered_send_event_queue_head)
		{
			event_note *temp = _unordered_send_event_queue_head;
			_unordered_send_event_queue_head = temp->_next_event;
			delete temp;
		}
		while(_send_event_queue_head)
		{
			event_note *temp = _send_event_queue_head;
			_send_event_queue_head = temp->_next_event;
			delete temp;
		}
	}
private:
	enum 
	{
		debug_checksum = 0xF00DBAAD,
		bit_stream_position_bit_size = 16,
		InvalidSendEventSeq = -1,
		first_valid_send_event_sequence = 0
	};
	event_note *_send_event_queue_head; ///< Head of the list of events to be sent to the remote host
	event_note *_send_event_queue_tail; ///< Tail of the list of events to be sent to the remote host.  New events are tagged on to the end of this list
	event_note *_unordered_send_event_queue_head; ///< Head of the list of events sent without ordering information
	event_note *_unordered_send_event_queue_tail; ///< Tail of the list of events sent without ordering information
	event_note *_wait_seq_events;   ///< List of ordered events on the receiving host that are waiting on previous sequenced events to arrive.
	event_note *_notify_event_list; ///< Ordered list of events on the sending host that are waiting for receipt of processing on the client.
	
	int32 _next_send_event_sequence; ///< The next sequence number for an ordered event sent through this connection
	int32 _next_receive_event_sequence; ///< The next receive event sequence to process
	int32 _last_acked_event_sequence; ///< The last event the remote host is known to have processed
	
	uint32 _rpc_count; ///< Number of net_event classes supported by this connection
	uint32 _rpc_id_bit_size; ///< Bit field width of net_event class count.
	uint32 mEventClassVersion; ///< The highest version number of events on this connection.
};

