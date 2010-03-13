/// test_connection is the TNLTest connection class.
///
/// The test_connection class overrides particular methods for connection housekeeping, and for connected clients manages the transmission of client inputs to the server for player position updates.
///
/// When a client's connection to the server is lost, the test_connection notifies the network interface that it should begin searching for a new server to join.
class test_connection : public ghost_connection
{
	typedef ghost_connection parent;
public:
	declare_dynamic_class()

	test_connection(bool is_initiator = false) : parent(is_initiator)
	{
	}
	
	/// The player object associated with this connection.
	safe_ptr<player> _player;
	
	void on_connection_rejected(bit_stream &reject_stream)
	{
		((test_net_interface *) get_interface())->_pinging_servers = true;
	}
	
	/// on_connect_terminated is called when the connection request to the server is unable to complete due to server rejection, timeout or other error.  When a test_connection connect request to a server is terminated, the client's network interface is notified so it can begin searching for another server to connect to.
	void on_connection_timed_out()
	{
		on_connection_terminated(0);
	}
	
	void on_connection_disconnected(bit_stream &disconnected_stream)
	{
		on_connection_terminated(&disconnected_stream);
	}
	
	void on_connection_terminated(bit_stream *the_stream)
	{
		if(is_connection_initiator())
			((test_net_interface *) get_interface())->_pinging_servers = true;
		else
			delete (player*) _player;
	}
	
	string get_address_string()
	{
		
	}
	/// on_connection_established is called on both ends of a connection when the connection is established.  On the server this will create a player for this client, and set it as the client's scope object.  On both sides this will set the proper ghosting behavior for the connection (ie server to client).
	void on_connection_established()
	{
		// call the parent's on_connection_established.  by default this will set the initiator to be a connection to "server" and the non-initiator to be a connection to "client"
		parent::on_connection_established();
		
		// To see how this program performs with 50% packet loss, Try uncommenting the next line :)
		//setSimulatedNetParams(0.5, 0);
		
		set_type_database(((test_net_interface *) get_interface())->get_game()->get_type_database());
		if(is_connection_initiator())
		{
			set_ghost_from(false);
			set_ghost_to(true);
			logprintf("%d - connected to server.", _connection);
			((test_net_interface *) get_interface())->_connection_to_server = this;
		}
		else
		{
			// on the server, we create a player object that will be the scope object
			// for this client.
			_player = new player;
			_player->add_to_game(((test_net_interface *) get_interface())->_game);
			set_scope_object(_player);
			set_ghost_from(true);
			set_ghost_to(false);
			activate_ghosting();
			logprintf("%d - client connected.", _connection);
		}
	}
	
	
	/// is_data_to_transmit is called each time the connection is ready to send a packet.  If the NetConnection subclass has data to send it should return true.  In the case of a simulation, this should always return true.
	bool is_data_to_transmit()
	{
		// we always want packets to be sent.
		return true;
	}
	
	/*
	/// Remote function that client calls to set the position of the player on the server.
	TNL_IMPLEMENT_RPC(test_connection, rpcSetPlayerPos, 
					  (TNL::F32 x, TNL::F32 y), (x, y),
					  TNL::NetClassGroupGameMask, TNL::RPCGuaranteedOrdered, TNL::RPCDirClientToServer, 0)
	{
		Position newPosition;
		newPosition.x = x;
		newPosition.y = y;
		
		TNL::logprintf("%s - received new position (%g, %g) from client", 
					   getNetAddressString(), 
					   newPosition.x, newPosition.y);
		
		_player->serverSetPosition(_player->renderPos, newPosition, 0, 0.2f);
		
		// send an RPC back the other way!
		TNL::StringTableEntry helloString("Hello World!!");
		rpcGotPlayerPos(true, false, helloString, x, y);
	};
	
	
	/// Enumeration constant used to specify the size field of Float<> parameters in the rpcGotPlayerPos method.
	enum RPCEnumerationValues {
		PlayerPosReplyBitSize = 6, ///< Size, in bits of the floats the server sends to the client to acknowledge position updates.
	};
	
	/// Remote function the server sends back to the client when it gets a player position.  We only use 6 bits of precision on each float to illustrate the float compression that's possible using TNL's RPC.  This RPC also sends bool and StringTableEntry data, as well as showing the use of the TNL_DECLARE_RPC_ENUM macro.
	TNL_IMPLEMENT_RPC(test_connection, rpcGotPlayerPos,
					  (bool b1, bool b2, TNL::StringTableEntry string, TNL::Float<test_connection::PlayerPosReplyBitSize> x, TNL::Float<test_connection::PlayerPosReplyBitSize> y), (b1, b2, string, x, y),
					  TNL::NetClassGroupGameMask, TNL::RPCGuaranteedOrdered, TNL::RPCDirAny, 0)
	{
		TNL::F32 xv = x, yv = y;
		TNL::logprintf("Server acknowledged position update - %d %d %s %g %g", b1, b2, string.getString(), xv, yv);
	}
	
	
	/// TNL_DECLARE_NETCONNECTION is used to declare that test_connection is a valid connection class to the
	/// TNL network system.
	TNL_DECLARE_NETCONNECTION(test_connection);*/
	
};

