
class net_connection;
class net_object;

class net_interface : public ref_object
{
	friend class net_object;
	
	typedef hash_table_array<torque_connection_id, ref_ptr<net_connection> >::pointer connection_pointer;
	struct connection_type_record
	{
		uint32 identifier;
		type_record *type;
	};
	
public:
	void set_key_pair(net::asymmetric_key_ptr the_key)
	{
		byte_buffer_ptr key_buffer = the_key->get_private_key();
		torque_socket_set_key_pair(_socket, key_buffer->get_buffer_size(), key_buffer->get_buffer());
		
		//byte_buffer_ptr private_key = the_key->get_private_key();
		//torque_socket_set_private_key(_socket, private_key->get_buffer_size(), private_key->get_buffer());
	}
	
	void set_challenge_response_data(byte_buffer_ptr data)
	{
		torque_socket_set_challenge_response(_socket, data->get_buffer_size(), data->get_buffer());
	}
	
	void set_allows_connections(bool allow)
	{
		torque_socket_allow_incoming_connections(_socket, allow);
	}
	
	void process_socket()
	{
		torque_socket_event *event;
		while((event = torque_socket_get_next_event(_socket)) != NULL)
		{
			logprintf("Processing event of type %d, connection_index = %d, size = %d", event->event_type, event->connection, event->data_size);
			switch(event->event_type)
			{
					
				case torque_connection_challenge_response_event_type:
					_process_challenge_response(event);
					break;
				case torque_connection_requested_event_type:
					_process_connection_requested(event);
					break;
				case torque_connection_arranged_connection_request_event_type:
					_process_arranged_connection_request(event);
					break;
				case torque_connection_accepted_event_type:
					_process_connection_accepted(event);
					break;
				case torque_connection_timed_out_event_type:
					_process_connection_timed_out(event);
					break;
				case torque_connection_disconnected_event_type:
					_process_connection_disconnected(event);
					break;
				case torque_connection_established_event_type:
					_process_connection_established(event);
					break;
				case torque_connection_packet_event_type:
					_process_connection_packet(event);
					break;
				case torque_connection_packet_notify_event_type:
					_process_connection_packet_notify(event);
					break;
				case torque_socket_packet_event_type:
					_process_socket_packet(event);
					break;
			}
		}
	}
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
	
	void collapse_dirty_list()
	{
		for(net_object *obj = _dirty_list_head._next_dirty_list; obj != &_dirty_list_tail; )
		{
			net_object *next = obj->_next_dirty_list;
			uint32 or_mask = obj->_dirty_mask_bits;
			
			obj->_next_dirty_list = NULL;
			obj->_prev_dirty_list = NULL;
			obj->_dirty_mask_bits = 0;
			
			if(or_mask)
			{
				for(ghost_info *walk = obj->_first_object_ref; walk; walk = walk->next_object_ref)
				{
					if(!walk->update_mask)
					{
						walk->update_mask = or_mask;
						walk->connection->ghost_push_non_zero(walk);
					}
					else
						walk->update_mask |= or_mask;
				}
			}
			obj = next;
		}
		_dirty_list_head._next_dirty_list = &_dirty_list_tail;
		_dirty_list_tail._prev_dirty_list = &_dirty_list_head;
	}
	void add_to_dirty_list(net_object *obj)
	{
		assert(obj->_next_dirty_list == 0);
		assert(obj->_prev_dirty_list == 0);
		obj->_next_dirty_list = _dirty_list_head._next_dirty_list;
		obj->_next_dirty_list->_prev_dirty_list = obj;
		_dirty_list_head._next_dirty_list = obj;
		obj->_prev_dirty_list = &_dirty_list_head;
	}
	
	net::time get_process_start_time()
	{
		return _process_start_time;
	}
	void check_for_packet_sends()
	{
		_process_start_time = net::time::get_current();
		collapse_dirty_list();
		for(uint32 i = 0; i < _connection_table.size(); i++)
		{
			net_connection *the_connection = *_connection_table[i].value();
			if(the_connection->get_connection_state() == net_connection::state_established)
				the_connection->check_packet_send(false, get_process_start_time());
		}
	}
		
	void _add_connection(ref_ptr<net_connection> &the_net_connection, torque_connection_id the_torque_connection)
	{
		the_net_connection->set_torque_connection(the_torque_connection);
		_connection_table.insert(the_torque_connection, the_net_connection);
	}
	
	void _process_challenge_response(torque_socket_event *event)
	{
		bit_stream challenge_response(event->data, event->data_size);
		byte_buffer_ptr public_key = new byte_buffer(event->public_key, event->public_key_size);
		connection_pointer p = _connection_table.find(event->connection);
		ref_ptr<net_connection> *the_connection = p.value();
		logprintf("Got a challenge response -- %d", bool(p));
		if(the_connection)
		{
			(*the_connection)->set_connection_state(net_connection::state_requesting_connection);
			(*the_connection)->on_challenge_response(challenge_response, public_key);
			torque_socket_accept_challenge(_socket, event->connection);
		}
	}
		
	void _process_connection_requested(torque_socket_event *event)
	{
		bit_stream key_stream(event->public_key, event->public_key_size);
		bit_stream request_stream(event->data, event->data_size);
		uint8 response_buffer[torque_sockets_max_status_datagram_size];
		bit_stream response_stream(response_buffer, torque_sockets_max_status_datagram_size);
		
		uint32 type_identifier;
		core::read(request_stream, type_identifier);
		type_record *rec = find_connection_type(type_identifier);
		if(!rec)
		{
			torque_socket_close_connection(_socket, event->connection, 0, 0);
		}
		net_connection *allocated = (net_connection *) operator new(rec->size);
		rec->construct_object(allocated);
		ref_ptr<net_connection> the_connection = allocated;
		the_connection->set_interface(this);

		if(the_connection->read_connect_request(request_stream, response_stream))
		{
			_add_connection(the_connection, event->connection);
			torque_socket_accept_connection(_socket, event->connection);
		}
		else
			torque_socket_close_connection(_socket, event->connection, response_stream.get_next_byte_position(), response_stream.get_buffer() );
	}
	
	void _process_arranged_connection_request(torque_socket_event *event)
	{
		
	}

	void _process_connection_accepted(torque_socket_event *event)
	{
		net::packet_stream response_stream;

		connection_pointer p = _connection_table.find(event->connection);

		ref_ptr<net_connection> *the_connection = p.value();
		if(the_connection)
		{
			(*the_connection)->set_connection_state(net_connection::state_accepted);
		}
	}
	
	void _process_connection_rejected(torque_socket_event *event)
	{
		bit_stream connection_rejected_stream(event->data, event->data_size);
		connection_pointer p = _connection_table.find(event->connection);
		
		ref_ptr<net_connection> *the_connection = p.value();
		if(the_connection)
		{
			(*the_connection)->set_connection_state(net_connection::state_rejected);
			(*the_connection)->on_connection_rejected(connection_rejected_stream);
			p.remove();
		}
	}
	
	void _process_connection_timed_out(torque_socket_event *event)
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
	
	void _process_connection_disconnected(torque_socket_event *event)
	{
		bit_stream connection_disconnected_stream(event->data, event->data_size);
		connection_pointer p = _connection_table.find(event->connection);
		
		ref_ptr<net_connection> *the_connection = p.value();
		if(the_connection)
		{
			(*the_connection)->set_connection_state(net_connection::state_disconnected);
			(*the_connection)->on_connection_disconnected(connection_disconnected_stream);
			p.remove();
		}
	}
	
	void _process_connection_established(torque_socket_event *event)
	{
		ref_ptr<net_connection> *the_connection = _connection_table.find(event->connection).value();
		if(the_connection)
		{
			(*the_connection)->set_connection_state(net_connection::state_established);
			(*the_connection)->on_connection_established();
		}
	}
	
	void _process_connection_packet(torque_socket_event *event)
	{
		ref_ptr<net_connection> *the_connection = _connection_table.find(event->connection).value();
		if(the_connection)
		{
			bit_stream packet(event->data, event->data_size);
			(*the_connection)->on_packet(event->packet_sequence, packet);
		}
	}
	
	void _process_connection_packet_notify(torque_socket_event *event)
	{
		ref_ptr<net_connection> *the_connection = _connection_table.find(event->connection).value();
		if(the_connection)
			(*the_connection)->on_packet_notify(event->packet_sequence, event->delivered);
	}
	
	virtual void _process_socket_packet(torque_socket_event *event)
	{
		
	}
	
	void connect(const net::address &remote_host, ref_ptr<net_connection> &the_connection)
	{
		uint8 connect_buffer[torque_sockets_max_status_datagram_size];
		bit_stream connect_stream(connect_buffer, sizeof(connect_buffer));
		
		uint32 type_identifier = get_type_identifier(the_connection);
		core::write(connect_stream, type_identifier);
		the_connection->set_interface(this);
		
		the_connection->write_connect_request(connect_stream);

		sockaddr the_sockaddr;
		remote_host.to_sockaddr(&the_sockaddr);
		torque_connection_id connection_id = torque_socket_connect(_socket, &the_sockaddr, connect_stream.get_next_byte_position(), connect_stream.get_buffer());
		logprintf("opened connection id = %d", connection_id);
		_add_connection(the_connection, connection_id);
	}

	virtual ~net_interface()
	{
		collapse_dirty_list();
		_dirty_list_head._next_dirty_list = 0;
	}
	
	torque_socket_handle &get_socket()
	{
		return _socket;
	}

	net_interface(const net::address &bind_address)
	{
		sockaddr sa_bind_address;
		bind_address.to_sockaddr(&sa_bind_address);
		_socket = torque_socket_create(false, 0, 0);
		torque_socket_bind(_socket, &sa_bind_address);

		_dirty_list_head._next_dirty_list = &_dirty_list_tail;
		_dirty_list_tail._prev_dirty_list = &_dirty_list_head;
		_dirty_list_head._prev_dirty_list = 0;
		_dirty_list_tail._next_dirty_list = 0;
	}
protected:
	torque_socket_handle _socket;
	net::time _process_start_time;
	net_object _dirty_list_head;
	net_object _dirty_list_tail;	
	array<connection_type_record> _connection_class_table;
	hash_table_array<torque_connection_id, ref_ptr<net_connection> > _connection_table;
};

