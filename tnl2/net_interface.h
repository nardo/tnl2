
class net_connection;
class net_interface : public ref_object
{
public:
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
		torque_socket_set_challenge_response_data(_socket, the_data.get_next_byte_position(), the_data.get_buffer());
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

	hash_table_array<torque_connection, ref_ptr<net_connection> > _connection_table;
	typedef hash_table_array<torque_connection, ref_ptr<net_connection> >::pointer connection_pointer;
	struct connection_type_record
	{
		uint32 identifier;
		type_record *type;
	};
	
	array<connection_type_record> _connection_class_table;

	template<class connection_type> void add_connection_type(uint32 identifier)
	{
		type_record *the_type_record = get_global_type_record<connection_type>();
		for(uint32 i = 0; i < _connection_class_table.size(); i++)
		{
			assert(_connection_class_table[i].identifier != identifier);
			assert(_connection_class_table[i].type != the_type_record);
		}
		
		connection_type_record rec;
		rec.identifier = identifier;
		rec.type = the_type_record;
		
		_connection_class_table.push_back(rec);
	}
	
	type_record *find_connection_type(uint32 type_identifier)
	{
		for(uint32 i = 0; i < _connection_class_table.size(); i++)
			if(_connection_class_table[i].identifier == type_identifier)
				return _connection_class_table[i].type;
		return 0;
	}
	
	uint32 get_type_identifier(ref_ptr<net_connection> &connection)
	{
		type_record *rec = connection->get_type_record();
		for(uint32 i = 0; i < _connection_class_table.size(); i++)
			if(_connection_class_table[i].type == rec)
				return _connection_class_table[i].identifier;
		assert(false);
		return 0;
	}
	
	void _add_connection(ref_ptr<net_connection> &the_net_connection, torque_connection the_torque_connection)
	{
		the_net_connection->set_torque_connection(the_torque_connection);
		_connection_table.insert(the_torque_connection, the_net_connection);
	}
	
	void _process_challenge_response(torque_connection_challenge_response_event *event)
	{
		bit_stream challenge_response(event->challenge_response_data, event->challenge_response_data_size);
		byte_buffer_ptr public_key = new byte_buffer(event->public_key, event->public_key_size);
		
		ref_ptr<net_connection> *the_connection = _connection_table.find(event->connection).value();
		if(the_connection)
		{
			(*the_connection)->set_connection_state(net_connection::state_requesting_connection);
			(*the_connection)->on_challenge_response(challenge_response, public_key);
		}
	}
		
	void _process_connection_requested(torque_connection_requested_event *event)
	{
		bit_stream key_stream(event->public_key, event->public_key_size);
		bit_stream request_stream(event->connection_request_data, event->connection_request_data_size);
		uint8 response_buffer[torque_max_status_datagram_size];
		bit_stream response_stream(response_buffer, torque_max_status_datagram_size);
		
		uint32 type_identifier;
		core::read(request_stream, type_identifier);
		type_record *rec = find_connection_type(type_identifier);
		if(!rec)
		{
			torque_connection_reject(event->connection, 0, response_stream.get_buffer());
		}
		net_connection *allocated = (net_connection *) operator new(rec->size);
		rec->construct_object(allocated);
		ref_ptr<net_connection> the_connection = allocated;
		if(the_connection->read_connect_request(request_stream, response_stream))
		{
			_add_connection(the_connection, event->connection);
			torque_connection_accept(event->connection, response_stream.get_byte_position(), response_stream.get_buffer());
		}
		else
			torque_connection_reject(event->connection, response_stream.get_next_byte_position(), response_stream.get_buffer());
	}
	
	void _process_arranged_connection_request(torque_connection_arranged_connection_request_event *event)
	{
		
	}

	void _process_connection_accepted(torque_connection_accepted_event *event)
	{
		bit_stream connection_accept_stream(event->connection_accept_data, event->connection_accept_data_size);
		
		ref_ptr<net_connection> *the_connection = _connection_table.find(event->connection).value();
		if(the_connection)
		{
			(*the_connection)->set_connection_state(net_connection::state_accepted);
			(*the_connection)->on_connection_accepted(connection_accept_stream);
			if((*the_connection)->get_connection_state() == net_connection::state_accepted)
			{
				(*the_connection)->set_connection_state(net_connection::state_established);
				(*the_connection)->on_connection_established();
			}
		}
	}
	
	void _process_connection_rejected(torque_connection_rejected_event *event)
	{
		bit_stream connection_rejected_stream(event->connection_reject_data, event->connection_reject_data_size);
		connection_pointer p = _connection_table.find(event->connection);
		
		ref_ptr<net_connection> *the_connection = p.value();
		if(the_connection)
		{
			(*the_connection)->set_connection_state(net_connection::state_rejected);
			(*the_connection)->on_connection_rejected(connection_rejected_stream);
			p.remove();
		}
	}
	
	void _process_connection_timed_out(torque_connection_timed_out_event *event)
	{
		connection_pointer p = _connection_table.find(event->connection);
		
		ref_ptr<net_connection> *the_connection = p.value();
		if(the_connection)
		{
			(*the_connection)->set_connection_state(net_connection::state_timed_out);
			(*the_connection)->on_connection_timed_out();
			p.remove();
		}
	}
	
	void _process_connection_disconnected(torque_connection_disconnected_event *event)
	{
		bit_stream connection_disconnected_stream(event->connection_disconnected_data, event->connection_disconnected_data_size);
		connection_pointer p = _connection_table.find(event->connection);
		
		ref_ptr<net_connection> *the_connection = p.value();
		if(the_connection)
		{
			(*the_connection)->set_connection_state(net_connection::state_disconnected);
			(*the_connection)->on_connection_disconnected(connection_disconnected_stream);
			p.remove();
		}
	}
	
	void _process_connection_established(torque_connection_established_event *event)
	{
		ref_ptr<net_connection> *the_connection = _connection_table.find(event->connection).value();
		if(the_connection)
		{
			(*the_connection)->set_connection_state(net_connection::state_established);
			(*the_connection)->on_connection_established();
		}
	}
	
	void _process_connection_packet(torque_connection_packet_event *event)
	{
		ref_ptr<net_connection> *the_connection = _connection_table.find(event->connection).value();
		if(the_connection)
		{
			bit_stream packet(event->data, event->data_size);
			(*the_connection)->on_packet(event->packet_sequence, packet);
		}
	}
	
	void _process_connection_packet_notify(torque_connection_packet_notify_event *event)
	{
		ref_ptr<net_connection> *the_connection = _connection_table.find(event->connection).value();
		if(the_connection)
			(*the_connection)->on_packet_notify(event->send_sequence, event->delivered);
	}
	
	void _process_socket_packet(torque_socket_packet_event *event)
	{
		
	}
	
	void connect(SOCKADDR *connect_address, ref_ptr<net_connection> &the_connection)
	{
		uint8 connect_buffer[torque_max_status_datagram_size];
		bit_stream connect_stream(connect_buffer, sizeof(connect_buffer));
		
		uint32 type_identifier = get_type_identifier(the_connection);
		core::write(connect_stream, type_identifier);
		the_connection->write_connect_request(connect_stream);

		torque_socket_connect(_socket, connect_address, connect_stream.get_next_byte_position(), connect_buffer);
	}
	torque_socket _socket;
};

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
	
	net_connection()
	{
	}
	virtual ~net_connection()
	{
	}
	
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
	
	virtual void on_connection_accepted(bit_stream &accept_stream)
	{
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
	
	virtual void on_packet(uint32 sequence, bit_stream &data)
	{
	}
	
	virtual void on_packet_notify(uint32 send_sequence, bool recvd)
	{
	}
	
	void set_torque_connection(torque_connection connection)
	{
		_connection = connection;
	}
protected:
	torque_connection _connection;
};