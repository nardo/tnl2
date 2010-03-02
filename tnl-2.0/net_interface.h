
class net_connection;
class net_interface
{
	net_interface(const SOCKADDR *bind_address)
	{
		_socket = torque_socket_create(bind_address);		
	}
	
	virtual ~net_interface()
	{
		torque_socket_destroy(_socket);
	}
	
	void set_private_key(asymmetric_key_ptr the_key)
	{
		byte_buffer_ptr private_key = the_key->get_private_key();
		torque_socket_set_private_key(_socket, private_key->get_buffer_size(), private_key->get_buffer());
	}
	
	void set_challenge_response_data(bit_stream &the_data)
	{
		torque_socket_set_challenge_response_data(_socket, the_data.get_byte_position(), the_data.get_buffer());
	}
	
	void allow_incoming_connections(bool allow)
	{
		torque_socket_allow_incoming_connections(_socket, allow);
	}
	
	bool does_allow_incoming_connections()
	{
		return torque_socket_does_allow_incoming_connections(_socket);
	}
	
	void allow_arranged_connections(bool allow)
	{
		torque_socket_allow_arranged_connections(_socket, allow);
	}
	
	bool does_allow_arranged_connections()
	{
		return torque_socket_does_allow_arranged_connections(_socket);
	}

	virtual net_connection *on_connection_request(SOCKADDR *remote_address, bit_stream &public_key, bit_stream &request_data, bit_stream &response_data) = 0;
	
	void process_socket()
	{
		torque_socket_event *event;
		while((event = torque_socket_get_next_event(_socket)) != NULL)
		{
			switch(event->event_type)
			{
					
				case torque_socket_event::torque_connection_challenge_response_event_type:
					_process_challenge_response((torque_connection_challenge_response_event *) event);
					break;
				case torque_socket_event::torque_connection_requested_event_type:
					_process_connection_requested((torque_connection_requested_event *) event);
					break;
				case torque_socket_event::torque_connection_arranged_connection_request_event_type:
					_process_arranged_connection_request((torque_connection_arranged_connection_request_event *) event);
					break;
				case torque_socket_event::torque_connection_accepted_event_type:
					_process_connection_accepted((torque_connection_accepted_event *) event);
					break;
				case torque_socket_event::torque_connection_rejected_event_type:
					_process_connection_rejected((torque_connection_rejected_event *) event);
					break;
				case torque_socket_event::torque_connection_timed_out_event_type:
					_process_connection_timed_out((torque_connection_timed_out_event *) event);
					break;
				case torque_socket_event::torque_connection_disconnected_event_type:
					_process_connection_disconnected((torque_connection_disconnected_event *) event);
					break;
				case torque_socket_event::torque_connection_established_event_type:
					_process_connection_established((torque_connection_established_event *) event);
					break;
				case torque_socket_event::torque_connection_packet_event_type:
					_process_connection_packet((torque_connection_packet_event *) event);
					break;
				case torque_socket_event::torque_connection_packet_notify_event_type:
					_process_connection_packet_notify((torque_connection_packet_notify_event *) event);
					break;
				case torque_socket_event::torque_socket_packet_event_type:
					_process_socket_packet((torque_socket_packet_event *) event);
					break;
			}
		}
	}
	void _process_challenge_response(torque_connection_challenge_response_event *event)
	{
		
	}
	
	void _process_connection_requested(torque_connection_requested_event *event)
	{
		bit_stream key_stream(event->public_key, event->public_key_size);
		bit_stream request_stream(event->connection_request_data, event->connection_request_data_size);
		uint8 response_buffer[torque_max_status_datagram_size];
		bit_stream response_stream(response_buffer, torque_max_status_datagram_size);
		net_connection *the_connection = on_connection_request(&event->source_address, key_stream, request_stream, response_stream);
		if(the_connection)
		{
			_add_connection(the_connection, event->the_connection);
			torque_connection_accept(event->the_connection, response_stream.get_byte_position(), response_stream.get_buffer());
		}
		else
			torque_connection_reject(event->the_connection, response_stream.get_byte_position(), response_stream.get_buffer());
	}
	
	void _process_arranged_connection_request(torque_connection_arranged_connection_request_event *event)
	{
		
	}

	void _process_connection_accepted(torque_connection_accepted_event *event)
	{
		
	}
	
	void _process_connection_rejected(torque_connection_rejected_event *event)
	{
		
	}
	
	void _process_connection_timed_out(torque_connection_timed_out_event *event)
	{
		
	}
	
	void _process_connection_disconnected(torque_connection_disconnected_event *event)
	{
		
	}
	
	void _process_connection_established(torque_connection_established_event *event)
	{
		
	}
	
	void _process_connection_packet(torque_connection_packet_event *event)
	{
		
	}
	
	void _process_connection_packet_notify(torque_connection_packet_notify_event *event)
	{
		
	}
	
	void _process_socket_packet(torque_socket_packet_event *event)
	{
		
	}
	
	
	void connect(SOCKADDR *connect_address)
	{
		
	}
	torque_socket _socket;
};

class net_connection
{
	net_connection();
	virtual ~net_connection();
	
	void connect(net_interface *the_interface, const SOCKADDR *remote_address);
	
};