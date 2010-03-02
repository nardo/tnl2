
class net_connection : public ref_object
{
public:
	declare_dynamic_class()
	
	void set_connection_state(uint32 new_state)
	{
		_state = new_state;
	}
	
	uint32 get_connection_state()
	{
		return _state;
	}
	uint32 _state;
	
	enum
	{
		state_start,
		state_awaiting_challenge_response,
		state_requesting_connection,
		state_accepted,
		state_established,
		state_rejected,
		state_timed_out,
		state_disconnected,
	};
	/// Rate management structure used specify the rate at which packets are sent and the maximum size of each packet.
	struct net_rate
	{
		uint32 min_packet_send_period; ///< Minimum millisecond delay (maximum rate) between packet sends.
		uint32 min_packet_recv_period; ///< Minimum millisecond delay the remote host should allow between sends.
		uint32 max_send_bandwidth; ///< Number of bytes per second we can send over the connection.
		uint32 max_recv_bandwidth; ///< Number of bytes per second max that the remote instance should send.
	};
	
	/// Structure used to track what was sent in an individual packet for processing
	/// upon notification of delivery success or failure.
	struct packet_notify
	{
		// packet stream notify stuff:
		bool rate_changed;  ///< True if this packet requested a change of rate.
		time send_time;     ///< getRealMilliseconds() when packet was sent.
		uint32 sequence;
		
		packet_notify *next_packet; ///< Pointer to the next packet sent on this connection
		packet_notify()
		{
			rate_changed = false;
		}
	};
	
	void connect(net_interface *the_interface, const SOCKADDR *remote_address);
	
	virtual void write_connect_request(bit_stream &connect_stream)
	{
	}
	
	virtual bool read_connect_request(bit_stream &request_stream, bit_stream &response_stream)
	{
		return true;
	}
	
	virtual void on_challenge_response(bit_stream &challenge_response, byte_buffer_ptr &public_key)
	{
	}
	
	virtual void write_connect_accept(bit_stream &accept_stream)
	{
		
	}
		
	virtual bool read_connect_accept(bit_stream &accept_stream, bit_stream &response_stream)
	{
		return true;
	}
	
	virtual void on_connection_rejected(bit_stream &reject_stream)
	{
	}
	
	virtual void on_connection_disconnected(bit_stream &disconnect_stream)
	{
	}
	
	virtual void on_connection_timed_out()
	{
	}
	
	virtual void on_connection_established()
	{
	}
	
	time _highest_acked_send_time;
	
	virtual void on_packet_notify(uint32 send_sequence, bool recvd)
	{
		TorqueLogMessageFormatted(LogNetConnection, ("connection %d: NOTIFY %d %s", _connection, send_sequence, recvd ? "RECVD" : "DROPPED"));

		packet_notify *note = _notify_queue_head;
		assert(note != NULL);
		_notify_queue_head = _notify_queue_head->next_packet;

		if(note->rate_changed && !recvd)
			_local_rate_changed = true;
		
		if(recvd)
		{
			_highest_acked_send_time = note->send_time;
			// Running average of roundTrip time
			if(_highest_acked_send_time != time(0))
			{
				time round_trip_delta = _interface->get_process_start_time() - (_highest_acked_send_time + _last_received_send_delay);
				_round_trip_time = _round_trip_time * 0.9f + round_trip_delta.get_milliseconds() * 0.1f;
				if(_round_trip_time < 0)
					_round_trip_time = 0;
			}      
			packet_received(note);
		}
		else
		{
			packet_dropped(note);
		}
		delete note;
	}
	
	torque_connection get_torque_connection()
	{
		return _connection;
	}
	
	void set_torque_connection(torque_connection connection)
	{
		_connection = connection;
	}
	
	/// Called when the packet associated with the specified notify is known to have been received by the remote host.  Packets are guaranteed to be notified in the order in which they were sent.
	virtual void packet_received(packet_notify *note)
	{
	}
	
	/// Called when the packet associated with the specified notify is known to have been not received by the remote host.  Packets are guaranteed to be notified in the order in which they were sent.
	virtual void packet_dropped(packet_notify *note)
	{
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
			return;
		packet_stream stream(_current_packet_send_size);
		_last_update_time = current_time;
		
		packet_notify *note = alloc_notify();

		if(!_notify_queue_head)
			_notify_queue_head = note;
		else
			_notify_queue_tail->next_packet = note;
		_notify_queue_tail = note;
		note->next_packet = NULL;
		note->send_time = _interface->get_process_start_time();
		
		write_packet_rate_info(stream, note);
		int32 start = stream.get_bit_position();
		
		TorqueLogMessageFormatted(LogNetConnection, ("connection %d: START", _connection) );
		TorqueLogMessageFormatted(LogNetConnection, ("connection %d: END - %llu bits", _connection, stream.get_bit_position() - start) );
		
		time send_delay = _interface->get_process_start_time() - _last_packet_recv_time;
		if(send_delay > time(2047))
			send_delay = time(2047);
		stream.write_integer(uint32(send_delay.get_milliseconds() >> 3), 8);
		write_packet(stream, note);
		
		torque_connection_send_to(_connection, stream.get_next_byte_position(), stream.get_buffer(), &_last_send_sequence);
		_notify_queue_tail->sequence = _last_send_sequence;
	}

	virtual void on_packet(uint32 sequence, bit_stream &data)
	{
		read_packet_rate_info(data);
		_last_received_send_delay = time((data.read_integer(8) << 3) + 4);
		_last_packet_recv_time = _interface->get_process_start_time();
		
	}

	/// Called to prepare the connection for packet writing.
	virtual void prepare_write_packet() {}
	
	///
	///  Any setup work to determine if there is_data_to_transmit() should happen in
	///  this function.  prepare_write_packet should _always_ call the Parent:: function.
	
	/// Called to write a subclass's packet data into the packet.Information about what the instance wrote into the packet can be attached to the notify ref_object.
	virtual void write_packet(bit_stream &bstream, packet_notify *note) {}
	
	/// Called to read a subclass's packet data from the packet.
	virtual void read_packet(bit_stream &bstream) {}	
	
	/// Allocates a data record to track data sent on an individual packet.  If you need to track additional notification information, you'll have to override this so you allocate a subclass of packet_notify with extra fields.
	virtual packet_notify *alloc_notify() { return new packet_notify; }
	
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
	
	/// Returns the running average packet round trip time.
	float32 get_round_trip_time()
	{
		return _round_trip_time;
	}
	
	/// Returns have of the average of the round trip packet time.
	float32 get_one_way_time()
	{
		return _round_trip_time * 0.5f;
	}
	
	
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
	
	/// Returns the notify structure for the current packet write, or last written packet.
	packet_notify *get_current_write_packet_notify()
	{
		return _notify_queue_tail;
	}
	
	bool window_full()
	{
		return _notify_queue_head && (_last_send_sequence - _notify_queue_head->sequence >= torque_packet_window_size);
	}
			
			
	/// Clears out the pending notify list.
	void _clear_all_packet_notifies() 
	{
		while(_notify_queue_head)
			on_packet_notify(0, false);
	}
	
	bool is_connection_host()
	{
	}
	
	bool is_connection_initiator()
	{
	}
	
	net_connection()
	{
		_round_trip_time = 0;
		_send_delay_credit = time(0);
		_last_update_time = time(0);
		_notify_queue_head = _notify_queue_tail = 0;
		_local_rate.max_recv_bandwidth = default_fixed_bandwidth;
		_local_rate.max_send_bandwidth = default_fixed_bandwidth;
		_local_rate.min_packet_recv_period = default_fixed_send_period;
		_local_rate.min_packet_send_period = default_fixed_send_period;
		
		_remote_rate = _local_rate;
		_local_rate_changed = true;
		compute_negotiated_rate();
		_last_send_sequence = 0;
	}
	
	net_interface *get_interface()
	{
		return _interface;
	}
	virtual ~net_connection()
	{
		_clear_all_packet_notifies();
		assert(_notify_queue_head == NULL);
	}
protected:
	enum rate_defaults {
		default_fixed_bandwidth = 2500, ///< The default send/receive bandwidth - 2.5 Kb per second.
		default_fixed_send_period = 96, ///< The default delay between each packet send - approx 10 packets per second.
		max_fixed_bandwidth = 65535, ///< The maximum bandwidth for a connection using the fixed rate transmission method.
		max_fixed_send_period = 2047, ///< The maximum period between packets in the fixed rate send transmission method.
	};
	enum {
		minimum_padding_bits = 32, ///< ask subclasses to reserve at least this much.
	};
	
	net_rate _local_rate; ///< Current communications rate negotiated for this connection.
	net_rate _remote_rate; ///< Maximum allowable communications rate for this connection.
	
	bool _local_rate_changed; ///< Set to true when the local connection's rate has changed.
	uint32 _current_packet_send_size; ///< Current size of each packet sent to the remote host.
	uint32 _current_packet_send_period; ///< Millisecond delay between sent packets.
	
	packet_notify *_notify_queue_head; ///< Linked list of structures representing the data in sent packets
	packet_notify *_notify_queue_tail; ///< Tail of the notify queue linked list.  New packets are added to the end of the tail.
	
	torque_connection _connection;
	time _last_packet_recv_time; ///< time of the receipt of the last data packet.
	float32 _round_trip_time; ///< Running average round trip time.
	time _send_delay_credit; ///< Metric to help compensate for irregularities on fixed rate packet sends.
	time _last_update_time;
	safe_ptr<net_interface> _interface;
	uint32 _last_send_sequence;
	time _last_received_send_delay;
};
