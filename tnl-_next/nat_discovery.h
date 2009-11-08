// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.
enum {
	nat_discovery_protocol_identifier = 0xBAADB33F,
};

// nat_discovery consists of a simple message reflection server process running on several distinct computers, and a client library that queries those servers to determine the client's public IP address and what type of NAT, if any it is behind.

struct nat_discovery_packet
{
	// nat_discovery packets are of the form
	// uint32 nat_discovery_protocol_identifier
	// uint32 code
	// address reference_address
	// uint32 magic_number

	uint32 protocol_id;
	uint32 code;
	address reference_address;
	uint32 magic_number;

	enum code_types
	{
		request_public_ip,
		public_ip_response,
		initiate_connection_request,
		initiate_connection_response,
		initiate_connection_punch,
	};

	nat_discovery_packet()
	{
		protocol_id = nat_discovery_protocol_identifier;
	}

	bool read_packet(byte *buffer, uint32 buffer_size)
	{
		byte_stream_fixed packet_stream(buffer, buffer_size);
		if(!read(packet_stream, protocol_id) || protocol_id != nat_discovery_protocol_identifier)
			return false;

		read(packet_stream, code);
		read(packet_stream, reference_address);
		return read(packet_stream, magic_number);
	}

	uint32 write_packet(byte *buffer, uint32 buffer_size)
	{
		byte_stream_fixed packet_stream(buffer, buffer_size);
		write(packet_stream, protocol_id);
		write(packet_stream, code);
		write(packet_stream, reference_address);
		write(packet_stream, magic_number);
		return packet_stream.get_position();
	}
};


/// the client_harness class encapsulates the NAT discovery and the (pre) connection process.
/// Client applications must be modified as follows:
/// in the main loop, the process_harness method should be called with the bound udp_socket
/// that will be used for client->client communication.  When any inbound packets are received
/// on that udp_socket, the client should first call filter_incoming_packet.  If this method
/// returns true, the client harness "ate" the packet and no further processing on that
/// packet should be done.
///
/// When the client/plugin code is ready to begin NAT discovery it should call begin_discovery
/// with the known IP addresses of two nat_discovery servers.  At some later point, the discovery_result
/// method will be called (which should be overridden by the client's own subclass of client_harness)
/// if begin_discovery is called more than once before the discovery is completed, only the most recent call will be honored.
///
/// When the client/plugin code is ready to begin a connection from the udp_socket, it should call
/// begin_remote_connection, with a connection sequence id (for future identification), the
/// public IP of the remote host and the internal LAN ip of the remote host.  At some later point
/// the remote_connection_result virtual method will be called, with either a failure code or
/// the correct IP address that the client code should initiate a connection to.

struct client_harness
{
	// The client harness processes two kinds of requests: IP discovery and connection creation.

	// state for tracking a discovery request
	bool _is_requesting_discovery;
	address _server1, _server2, _public_address1, _public_address2;
	uint32 _discovery_request_count;
	bool _discovery1_received, _discovery2_received;
	time _last_discovery_request_time;
	uint32 _magic_number;

	enum {
		discovery_timeout = 1000,
		discovery_try_count = 3,
		connection_timeout = 1000,
		connection_try_count = 5,
	};

	client_harness()
	{
		_is_requesting_discovery = false;
		_connection_list = 0;
	}

	void begin_discovery(udp_socket &the_socket, address server1, address server2)
	{
		_server1 = server1;
		_server2 = server2;
		_discovery1_received = _discovery2_received = false;
		_discovery_request_count = 0;
		_last_discovery_request_time = time::get_current();

		_magic_number = uint32(_last_discovery_request_time.get_milliseconds());
		_is_requesting_discovery = true;
		_send_discovery_packet(the_socket);
	}

	void _send_discovery_packet(udp_socket &the_socket)
	{
		nat_discovery_packet p;
		p.code = nat_discovery_packet::request_public_ip;
		p.magic_number = _magic_number;
		_discovery_request_count++;			
		byte buffer[udp_socket::max_datagram_size];
		uint32 size = p.write_packet(buffer, sizeof(buffer));

		if(!_discovery1_received)
		{
			printf("Client: Sending discovery packet %d to server 1\n", _discovery_request_count);
			the_socket.send_to(_server1, buffer, size);
		}
		if(!_discovery2_received)
		{
			printf("Client: Sending discovery packet %d to server 2\n", _discovery_request_count);
			the_socket.send_to(_server2, buffer, size);
		}
	}
		
	void process_harness(udp_socket &the_socket)
	{
		time current_time = time::get_current();
		if(_is_requesting_discovery)
		{
			if(current_time - _last_discovery_request_time > discovery_timeout)
			{
				if(_discovery_request_count > discovery_try_count)
				{
					_is_requesting_discovery = false;
					array<address> if_addrs;
					address::get_interface_addresses(if_addrs);
					printf("discovery timed out.\n");
					discovery_result(nat_type_internal_games_only_timeout, if_addrs);
				}
				else
				{
					_last_discovery_request_time = current_time;
					_send_discovery_packet(the_socket);
				}
			}
		}
		for(pending_connection **walk = &_connection_list; *walk; )
		{
			pending_connection *c = *walk;
			if(current_time - c->last_send_time > connection_timeout)
			{
				if(c->send_count > connection_try_count)
				{
					// this timed out; if it was the initiator, let the system know, otherwise just silently fail
					if(c->is_initiator)
					{
						printf("pending connection request timed out.\n");
						remote_connection_result(c->connection_sequence, remote_connection_timed_out, address());
					}
					*walk = c->next_pending;
					delete c;
				}
				else
				{
					uint32 code = c->is_initiator ? nat_discovery_packet::initiate_connection_request : nat_discovery_packet::initiate_connection_punch;
					_send_connection_packet(the_socket, current_time, code, c);
					walk = &c->next_pending;
				}
			}
		}
	}
	
	bool filter_incoming_packet(udp_socket &the_socket, address sender_address, byte *packet_buffer, uint32 packet_size)
	{
		nat_discovery_packet the_packet;
		if(!the_packet.read_packet(packet_buffer, packet_size))
			return false;

		if(_is_requesting_discovery && the_packet.magic_number == _magic_number && the_packet.code == nat_discovery_packet::public_ip_response)
		{
			if(!_discovery1_received && sender_address == _server1)
			{
				_public_address1 = the_packet.reference_address;
				_discovery1_received = true;
				string addr_string = _public_address1.to_string();
				printf("received discovery packet from server1: addr = %s\n", addr_string.c_str());
			}
			else if(!_discovery2_received && sender_address == _server2)
			{
				_public_address2 = the_packet.reference_address;
				_discovery2_received = true;
				string addr_string = _public_address2.to_string();
				printf("received discovery packet from server2: addr = %s\n", addr_string.c_str());
			}
			if(_discovery1_received && _discovery2_received)
			{
				discovery_result_code result;
				_is_requesting_discovery = false;
				array<address> address_list;
				address::get_interface_addresses(address_list);

				if(_public_address1 == _public_address2)
				{
					printf("discovery done, addresses match! whee!\n");
					result = nat_type_internet_games_playable;
					address_list.push_back(_public_address1);
				}
				else
				{
					printf("discovery done, not permissive firewall\n");
					result = nat_type_internal_games_only;
				}

				discovery_result(result, address_list);
			}
		}
		// now loop through the pending connections:
		for(pending_connection **walk = &_connection_list; *walk; )
		{
			// make sure the sender address was on our incoming list:
			pending_connection *c = *walk;
			uint32 i;
			for(i = 0; i < c->possible_addresses.size(); i++)
				if(sender_address == c->possible_addresses[i])
					break;

			if(the_packet.magic_number == c->connection_sequence && i != c->possible_addresses.size())
			{
				if(c->is_initiator && the_packet.code == nat_discovery_packet::initiate_connection_response)
				{
					printf("connection response received - connection established.\n");
					// we got our response packet; start the connection and pull this one from the list:
					*walk = c->next_pending;
					remote_connection_result(c->connection_sequence, remote_connection_success, sender_address);
					delete c;
					continue;
				}
				else
				{
					printf("connection request received - sending response.\n");

					// bounce back a connection_response packet if this is a connection request
					if(!c->is_initiator && the_packet.code == nat_discovery_packet::initiate_connection_request)
						_send_connection_packet(the_socket, time::get_current(), nat_discovery_packet::initiate_connection_response, c);
				}
			}
			walk = &c->next_pending;
		}
		return true;
	}

	struct pending_connection
	{
		bool is_initiator;
		time last_send_time;
		uint32 send_count;
		array<address> possible_addresses;
		uint32 connection_sequence;
		pending_connection *next_pending;
	};
	pending_connection *_connection_list;

	// the remote connection is a two-phase send/ack process - first both sides (initiator and not)
	// send out code 2 packets to all the possible addresses the other side might be at.
	// if no packet is received, time out
	// when the initiator receives a code 2, it sends a code 3 to that host.  When a host 
	void begin_remote_connection(udp_socket &the_socket, bool is_initiator, uint32 connection_sequence, array<address> &possible_address_list)
	{
		pending_connection *pending = new pending_connection;
		pending->is_initiator = is_initiator;
		pending->send_count = 0;
		pending->possible_addresses = possible_address_list;
		pending->connection_sequence = connection_sequence;
		pending->next_pending = _connection_list;
		_connection_list = pending;
		uint32 code = is_initiator ? nat_discovery_packet::initiate_connection_request : nat_discovery_packet::initiate_connection_punch;

		_send_connection_packet(the_socket, time::get_current(), code, pending);
	}
	
	void _send_connection_packet(udp_socket &the_socket, time current_time, uint32 code, pending_connection *conn)
	{
		conn->last_send_time = current_time;
		nat_discovery_packet p;
		p.code = code;
		p.magic_number = _magic_number;
		byte buffer[udp_socket::max_datagram_size];
		for(uint32 i = 0; i < conn->possible_addresses.size(); i++)
		{
			p.reference_address = conn->possible_addresses[i];
			string addr_string = p.reference_address.to_string();
			uint32 size = p.write_packet(buffer, sizeof(buffer));
			the_socket.send_to(conn->possible_addresses[i], buffer, size);

			printf("sending connection packet %d to %s\n", p.code, addr_string.c_str());
		}
	}

	enum discovery_result_code
	{
		nat_type_internet_games_playable,
		nat_type_internal_games_only,
		nat_type_internal_games_only_timeout,
	};
	virtual void discovery_result(discovery_result_code code, array<address> &address_list) = 0;

	enum remote_connection_result_code
	{
		remote_connection_timed_out,
		remote_connection_success
	};

	virtual void remote_connection_result(uint32 connection_sequence, remote_connection_result_code code, address remote_address) = 0;
};

struct application
{
	virtual void process() = 0;
};

struct server : application
{
	udp_socket _socket;
	server(address interface_address)
	{
		_socket.bind(interface_address, false);
	}

	void process()
	{
		address sender_address;
		uint32 packet_size;
		byte buffer[udp_socket::max_datagram_size];
		while(_socket.recv_from(&sender_address, buffer, sizeof(buffer), &packet_size) != udp_socket::no_incoming_packets_available)
		{
			nat_discovery_packet p;
			address send_to_address;
			string addr_string;

			if(!p.read_packet(buffer, packet_size) || p.code != nat_discovery_packet::request_public_ip)
				continue;

			p.code = nat_discovery_packet::public_ip_response;
			send_to_address = sender_address;
			p.reference_address = sender_address;

			addr_string = sender_address.to_string();
			printf("received ip request from %s\n", addr_string.c_str());
			// now set up a packet to send out:
			_socket.send_to(send_to_address, buffer, p.write_packet(buffer, sizeof(buffer)));
		}
	}
};

struct test_client : application
{
	struct test_harness : public client_harness
	{
		test_client *_client;

		test_harness(test_client *the_client)
		{
			_client = the_client;
		}

		virtual void discovery_result(discovery_result_code code, array<address> &address_list)
		{

		}

		virtual void remote_connection_result(uint32 connection_sequence, remote_connection_result_code code, address remote_address)
		{

		}
	};

	udp_socket _socket;
	address _server1, _server2;
	test_harness _harness;

	test_client(address server1, address server2) : _harness(this)
	{
		_socket.bind(address(), false);
		_server1 = server1;
		_server2 = server2;
	}

	void process()
	{
		// first process the harness (this would be in the main loop of the app)
		_harness.process_harness(_socket);

		// then process incoming packets on the socket:
		address sender_address;
		uint32 packet_size;
		byte buffer[udp_socket::max_datagram_size];
		while(_socket.recv_from(&sender_address, buffer, sizeof(buffer), &packet_size) != udp_socket::no_incoming_packets_available)
		{
			if(!_harness.filter_incoming_packet(_socket, sender_address, buffer, packet_size))
			{
				// if the harness didn't filter the packet, it would be processed by the application here.
			}
		}
	}
};

int run(int argc, const char **argv)
{
	array<application *> applications;

	for(int i = 1; i < argc;)
	{
		if(!strcmp(argv[i], "server"))
		{
			if(i + 1 < argc)
			{
				address if_addr(argv[i+1]);
				application *s = new server(if_addr);
				applications.push_back(s);
			}
			i+= 2;
		}
		else if(!strcmp(argv[i], "client"))
		{
			if(i + 2 < argc)
			{
				address server1(argv[i+1]);
				address server2(argv[i+2]);
				applications.push_back(new test_client(server1, server2));
			}
			i += 3;
		}
	}

	sockets_unit_test();

	for(;;)
	{
		for(uint32 i = 0; i < applications.size(); i++)
			applications[i]->process();
		sleep(1);
	}

	return 0;
}

