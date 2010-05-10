/// test_net_interface - the net_interface subclass used in TNLTest.  test_net_interface subclasses TNLTest to provide server pinging and response info packets.  When a client test_net_interface starts, it begins sending ping packets to the ping_addr address specified in the constructor.  When a server receives a GamePingRequest packet, it sends a GamePingResponse packet to the source address of the GamePingRequest.  Upon receipt of this response packet, the client attempts to connect to the server that returned the response.  When the connection or connection attempt to that server is terminated, the test_net_interface resumes pinging for available TNLTest servers. 
class test_net_interface : public net_interface
{
	typedef net_interface parent;
public:
	/// Constants used in this net_interface
	enum Constants {
		PingDelayTime = 2000, ///< Milliseconds to wait between sending GamePingRequest packets.
		GamePingRequest = net::torque_socket::first_valid_info_packet_id, ///< Byte value of the first byte of a GamePingRequest packet.
		GamePingResponse, ///< Byte value of the first byte of a GamePingResponse packet.
	};
	
	bool _pinging_servers; ///< True if this is a client that is pinging for active servers.
	net::time _last_ping_time; ///< The last platform time that a GamePingRequest was sent from this network interface.
	bool _is_server; ///< True if this network interface is a server, false if it is a client.
	
	safe_ptr<test_connection> _connection_to_server; ///< A safe pointer to the current connection to the server, if this is a client.
	SOCKADDR _ping_address; ///< Network address to ping in searching for a server.  This can be a specific host address or it can be a LAN broadcast address.
	test_game *_game; ///< The game object associated with this network interface.
	
	/// Constructor for this network interface, called from the constructor for test_game.  The constructor initializes and binds the network interface, as well as sets parameters for the associated game and whether or not it is a server.
	test_net_interface(test_game *the_game, bool server, torque_socket_interface *socket_interface, void *user_data) : net_interface(socket_interface, user_data)
	{
		_game = the_game;
		_is_server = server;
		_pinging_servers = !server;
		_last_ping_time = 0;
	}
	
	void set_ping_address(SOCKADDR &addr)
	{
		_ping_address = addr;		
	}

	test_game *get_game()
	{
		return _game;
	}
	
	/// handle_info_packet overrides the method in the net_interface class to handle processing of the GamePingRequest and GamePingResponse packet types.
	void _process_socket_packet(torque_socket_event *event)
	{
		net::packet_stream write_stream;
		core::uint8 packet_type = event->data[0];
		net::address from(event->source_address);
		string from_string = from.to_string();
		logprintf("%s - received socket packet, packet_type == %d.", from_string.c_str(), packet_type);
		if(packet_type == GamePingRequest && _is_server)
		{
			logprintf("%s - received ping.", from_string.c_str());
			// we're a server, and we got a ping packet from a client, so send back a GamePingResponse to let the client know it has found a server.
			core::write(write_stream, core::uint8(GamePingResponse));
			get_socket_interface()->send_to(_socket, &event->source_address, write_stream.get_byte_position(), write_stream.get_buffer());
			
			logprintf("%s - sending ping response.", from_string.c_str());
		}
		else if(packet_type == GamePingResponse && _pinging_servers)
		{
			// we were pinging servers and we got a response.  Stop the server pinging, and try to connect to the server.
			
			logprintf("%s - received ping response.", from_string.c_str());
			
			ref_ptr<net_connection> the_connection = new test_connection(true);
			net::address from(event->source_address);
			connect(from, the_connection); 
			logprintf("Connecting to server: %s", from_string.c_str());
			
			_pinging_servers = false;
		}
	}	
	
	/// send_ping sends a GamePingRequest packet to _ping_address of this test_net_interface.
	void send_ping()
	{
		net::packet_stream write_stream;
		
		core::write(write_stream, core::uint8(GamePingRequest));
		get_socket_interface()->send_to(_socket, &_ping_address, write_stream.get_next_byte_position(), write_stream.get_buffer());

		string ping_address_string = net::address(_ping_address).to_string();
		logprintf("%s - sending ping.", ping_address_string.c_str());
	}
	
	/// Tick checks to see if it is an appropriate time to send a ping packet, in addition to checking for incoming packets and processing connections.
	void tick()
	{
		net::time current_time = net::time::get_current();
		
		if(_pinging_servers && (_last_ping_time + PingDelayTime) < current_time)
		{
			_last_ping_time = current_time;
			send_ping();
		}
		process_socket();
		check_for_packet_sends();
	}
};

