for(uint32 i = 0; i < _connection_list.size(); i++)
_connection_list[i]->check_packet_send(false, get_process_start_time());

read_packet_rate_info(bstream);
_last_packet_recv_time = _interface->get_process_start_time();

time pk_send_delay = time((pstream.read_integer(8) << 3) + 4);

/// Dispatches a notify when a packet is ACK'd or NACK'd.
void handle_notify(uint32 sequence, bool recvd)
{
	TorqueLogMessageFormatted(LogNetConnection, ("connection %s: NOTIFY %d %s", _address.to_string().c_str(), sequence, recvd ? "RECVD" : "DROPPED"));
	
	if(recvd)
	{
		_highest_acked_send_time = note->send_time;
		packetReceived(note);
	}
	else
	{
		packetDropped(note);
	}
	delete note;
}

/// Clears out the pending notify list.
void clear_all_packet_notifies() 
{
	while(_notify_queue_head)
		handle_notify(0, false);
}

void write_raw_packet(bit_stream &bstream, net_packet_type packet_type)
{
	write_packet_header(bstream, packet_type);
	if(packet_type == data_packet)
	{
		packet_notify *note = alloc_notify();
		if(!_notify_queue_head)
			_notify_queue_head = note;
		else
			_notify_queue_tail->next_packet = note;
		_notify_queue_tail = note;
		note->next_packet = NULL;
		note->send_time = _interface->get_process_start_time();
		
		write_packet_rate_info(bstream, note);
		int32 start = bstream.get_bit_position();
		
		TorqueLogMessageFormatted(LogNetConnection, ("connection %s: START", _address.to_string().c_str()) );
		write_packet(bstream, note);
		TorqueLogMessageFormatted(LogNetConnection, ("connection %s: END - %llu bits", _address.to_string().c_str(), bstream.get_bit_position() - start) );

		time send_delay = _interface->get_process_start_time() - _last_packet_recv_time;
		if(send_delay > time(2047))
			send_delay = time(2047);
		stream.write_integer(uint32(send_delay.get_milliseconds() >> 3), 8);
		
		
	}
	if(!_symmetric_cipher.is_null())
	{
		_symmetric_cipher->setup_counter(_last_send_seq, _last_seq_recvd, packet_type, 0);
		bit_stream_hash_and_encrypt(bstream, message_signature_bytes, packet_header_byte_size, _symmetric_cipher);
	}
}

class net_connection : connection
{
	/// Returns the running average packet round trip time.
	float32 get_round_trip_time()
	{ return _round_trip_time; }
	
	/// Returns have of the average of the round trip packet time.
	float32 getOneWayTime()
	{ return _round_trip_time * 0.5f; }
	
	
	/// Structure used to track what was sent in an individual packet for processing
	/// upon notification of delivery success or failure.
	struct packet_notify
	{
		// packet stream notify stuff:
		bool rate_changed;  ///< True if this packet requested a change of rate.
		time send_time;     ///< getRealMilliseconds() when packet was sent.
		packet_notify *next_packet; ///< Pointer to the next packet sent on this connection
		packet_notify()
		{
			rate_changed = false;
		}
	};
	time _last_packet_recv_time; ///< time of the receipt of the last data packet.
	
	
	~connection()
	{
		clear_all_packet_notifies();
		assert(_notify_queue_head == NULL);
	}
	
	_round_trip_time = 0;
	_send_delay_credit = time(0);
	_local_rate.max_recv_bandwidth = default_fixed_bandwidth;
	_local_rate.max_send_bandwidth = default_fixed_bandwidth;
	_local_rate.min_packet_recv_period = default_fixed_send_period;
	_local_rate.min_packet_send_period = default_fixed_send_period;
	
	_remote_rate = _local_rate;
	_local_rate_changed = true;
	compute_negotiated_rate();
	
	/// Writes any packet send rate change information into the packet.
	void write_packet_rate_info(bit_stream &bstream, packet_notify *note)
	{
		note->rate_changed = _local_rate_changed;
		_local_rate_changed = false;
		if(bstream.write_bool(note->rate_changed))
		{
			bstream.write_ranged_uint32(_local_rate.max_recv_bandwidth, 0, max_fixed_bandwidth);
			bstream.write_ranged_uint32(_local_rate.max_send_bandwidth, 0, max_fixed_bandwidth);
			bstream.write_ranged_uint32(_local_rate.min_packet_recv_period, 1, max_fixed_send_period);
			bstream.write_ranged_uint32(_local_rate.min_packet_send_period, 1, max_fixed_send_period);
		}
	}
	
	/// Reads any packet send rate information requests from the packet.
	void read_packet_rate_info(bit_stream &bstream)
	{
		if(bstream.read_bool())
		{
			_remote_rate.max_recv_bandwidth = bstream.read_ranged_uint32(0, max_fixed_bandwidth);
			_remote_rate.max_send_bandwidth = bstream.read_ranged_uint32(0, max_fixed_bandwidth);
			_remote_rate.min_packet_recv_period = bstream.read_ranged_uint32(1, max_fixed_send_period);
			_remote_rate.min_packet_send_period = bstream.read_ranged_uint32(1, max_fixed_send_period);
			compute_negotiated_rate();
		}
	}
	
	void handle_notify(uint32 sequence, bool recvd)
	_highest_acked_send_time = time(0);

	packet_notify *note = _notify_queue_head;
	assert(note != NULL);
	_notify_queue_head = _notify_queue_head->next_packet;
	
	if(note->rate_changed && !recvd)
		_local_rate_changed = true;
	
	// Running average of roundTrip time
	if(_highest_acked_send_time != time(0))
	{
		time round_trip_delta = _interface->get_process_start_time() - (_highest_acked_send_time + pk_send_delay);
		_round_trip_time = _round_trip_time * 0.9f + round_trip_delta.get_milliseconds() * 0.1f;
		if(_round_trip_time < 0)
			_round_trip_time = 0;
	}      
	
	float32 _round_trip_time; ///< Running average round trip time.
	time _send_delay_credit; ///< Metric to help compensate for irregularities on fixed rate packet sends.
	
	enum rate_defaults {
		default_fixed_bandwidth = 2500, ///< The default send/receive bandwidth - 2.5 Kb per second.
		default_fixed_send_period = 96, ///< The default delay between each packet send - approx 10 packets per second.
		max_fixed_bandwidth = 65535, ///< The maximum bandwidth for a connection using the fixed rate transmission method.
		max_fixed_send_period = 2047, ///< The maximum period between packets in the fixed rate send transmission method.
	};
	
	/// Rate management structure used specify the rate at which packets are sent and the maximum size of each packet.
	struct net_rate
	{
		uint32 min_packet_send_period; ///< Minimum millisecond delay (maximum rate) between packet sends.
		uint32 min_packet_recv_period; ///< Minimum millisecond delay the remote host should allow between sends.
		uint32 max_send_bandwidth; ///< Number of bytes per second we can send over the connection.
		uint32 max_recv_bandwidth; ///< Number of bytes per second max that the remote instance should send.
	};
    /// Called internally when the local or remote rate changes.
	void compute_negotiated_rate()
	{
		_current_packet_send_period = max(_local_rate.min_packet_send_period, _remote_rate.min_packet_recv_period);
		
		uint32 max_bandwidth = min(_local_rate.max_send_bandwidth, _remote_rate.max_recv_bandwidth);
		_current_packet_send_size = uint32(max_bandwidth * _current_packet_send_period * 0.001f);
		
		// make sure we don't try to overwrite the maximum packet size
		if(_current_packet_send_size > udp_socket::max_datagram_size)
			_current_packet_send_size = udp_socket::max_datagram_size;
	}
	
	net_rate _local_rate; ///< Current communications rate negotiated for this connection.
	net_rate _remote_rate; ///< Maximum allowable communications rate for this connection.
	
	bool _local_rate_changed; ///< Set to true when the local connection's rate has changed.
	uint32 _current_packet_send_size; ///< Current size of each packet sent to the remote host.
	uint32 _current_packet_send_period; ///< Millisecond delay between sent packets.
	
	packet_notify *_notify_queue_head; ///< Linked list of structures representing the data in sent packets
	packet_notify *_notify_queue_tail; ///< Tail of the notify queue linked list.  New packets are added to the end of the tail.
	
	/// Returns the notify structure for the current packet write, or last written packet.
	packet_notify *get_current_write_packet_notify() { return _notify_queue_tail; }

	/// Checks to see if a packet should be sent at the currentTime to the remote host.
	///
	/// If force is true and there is space in the window, it will always send a packet.
	void check_packet_send(bool force, time current_time)
	{
		time delay = time( _current_packet_send_period );
		
		if(!force)
		{
			if(current_time - _last_update_time + _send_delay_credit < delay)
				return;
			
			_send_delay_credit = current_time - (_last_update_time + delay - _send_delay_credit);
			if(_send_delay_credit > time(1000))
				_send_delay_credit = time(1000);
		}
		prepare_write_packet();
		if(window_full() || !is_data_to_transmit())
		{
			return;
		}
		packet_stream stream(_current_packet_send_size);
		_last_update_time = current_time;
		
		write_raw_packet(stream, data_packet);   
		
		send_packet(stream);
	}
	
	
	/// There are a few state variables here that aren't documented.
	///
	/// @{
	
public:
	/// sets the fixed rate send and receive data sizes, and sets the connection to not behave as an adaptive rate connection
	void set_fixed_rate_parameters( uint32 min_packet_send_period, uint32 min_packet_recv_period, uint32 max_send_bandwidth, uint32 max_recv_bandwidth )
	{
		_local_rate.max_recv_bandwidth = max_recv_bandwidth;
		_local_rate.max_send_bandwidth = max_send_bandwidth;
		_local_rate.min_packet_recv_period = min_packet_recv_period;
		_local_rate.min_packet_send_period = min_packet_send_period;
		_local_rate_changed = true;
		compute_negotiated_rate();
	}
	
	/// Returns true if this connection has data to transmit.
	///
	/// The adaptive rate protocol needs to be able to tell if there is data
	/// ready to be sent, so that it can avoid sending unnecessary packets.
	/// Each subclass of connection may need to send different data - events,
	/// ghost updates, or other things. Therefore, this hook is provided so
	/// that child classes can overload it and let the adaptive protocol
	/// function properly.
	///
	/// @note Make sure this calls to its parents - the accepted idiom is:
	///       @code
	///       return Parent::is_data_to_transmit() || localConditions();
	///       @endcode
	virtual bool is_data_to_transmit() { return false; }
	
	/// @}
	
	/// Called to read a subclass's packet data from the packet.
	virtual void read_packet(bit_stream &bstream) {}
	
	/// Called to prepare the connection for packet writing.
	virtual void prepare_write_packet() {}
	
	///
	///  Any setup work to determine if there is_data_to_transmit() should happen in
	///  this function.  prepare_write_packet should _always_ call the Parent:: function.
	
	/// Called to write a subclass's packet data into the packet.Information about what the instance wrote into the packet can be attached to the notify ref_object.
	virtual void write_packet(bit_stream &bstream, packet_notify *note) {}
	
	/// Called when the packet associated with the specified notify is known to have been received by the remote host.  Packets are guaranteed to be notified in the order in which they were sent.
	virtual void packetReceived(packet_notify *note)
	{
	}
	
	/// Called when the packet associated with the specified notify is known to have been not received by the remote host.  Packets are guaranteed to be notified in the order in which they were sent.
	virtual void packetDropped(packet_notify *note)
	{
	}
	
	/// Allocates a data record to track data sent on an individual packet.  If you need to track additional notification information, you'll have to override this so you allocate a subclass of packet_notify with extra fields.
	virtual packet_notify *alloc_notify() { return new packet_notify; }
	
};