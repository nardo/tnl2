/// The test_game class manages a TNLTest client or server instance.  test_game maintains a list of all the player and building objects in the playing area, and interfaces with the specific platform's windowing system to respond to user input and render the current game display.
class test_net_interface;
class test_game {
public:
	array<player *> _players; ///< vector of player objects in the game
	array<building *> _buildings; ///< vector of _buildings in the game
	bool _is_server; ///< was this game created to be a server?
	test_net_interface *_net_interface; ///< network interface for this game
	net::time _last_time; ///< last time that tick() was called
	safe_ptr<player> _server_player; ///< the player that the server controls
	safe_ptr<player> _client_player; ///< the player that this client controls, if this game is a client
	net::random_generator _random;
	context _context;
	type_database _type_database;
	
	enum {
		test_connection_identifier_token = 0xBEEF,
	};
	/// Constructor for test_game, determines whether this game will be a client or a server, and what addresses to bind to and ping.  If this game is a server, it will construct 50 random _buildings and 15 random AI _players to populate the "world" with.  test_game also constructs an AsymmetricKey to demonstrate establishing secure connections with clients and servers.
	test_game(bool server, SOCKADDR &interface_bind_address, SOCKADDR &ping_address) : _type_database(&_context)
	{
		_is_server = server;
		_net_interface = new test_net_interface(this, _is_server, interface_bind_address, ping_address);
		_net_interface->add_connection_type<test_connection>(test_connection_identifier_token);

		//TNL::AsymmetricKey *theKey = new TNL::AsymmetricKey(32);
		//_net_interface->setPrivateKey(theKey);
		//_net_interface->setRequiresKeyExchange(true);
		net::random_generator g;
		
		tnl_begin_class(_type_database, position, empty_type, false)
		tnl_slot(_type_database, position, x, 0)
		tnl_slot(_type_database, position, y, 0)
		tnl_end_class(_type_database)
		
		player::register_class(_type_database);
		building::register_class(_type_database);

		_last_time = net::time::get_current();
		
		if(_is_server)
		{
			// generate some _buildings and AIs:
			for(int32 i = 0; i < 50; i ++)
			{
				building *the_building = new building(&g);
				the_building->add_to_game(this);
			}
			for(int32 i = 0; i < 15; i ++)
			{
				player *ai_player = new player(player::player_type_ai, &g);
				ai_player->add_to_game(this);
			}
			_server_player = new player(player::player_type_my_client);
			_server_player->add_to_game(this);
		}
		
		logprintf("Created a %s...", (server ? "server" : "client"));
	}
	
	/// Destroys a game, freeing all player and building objects associated with it.
	~test_game()
	{
		delete _net_interface;
		for(int32 i = 0; i < _buildings.size(); i++)
			delete _buildings[i];
		for(int32 i = 0; i < _players.size(); i++)
			delete _players[i];
		
		logprintf("Destroyed a %s...", (this->_is_server ? "server" : "client"));
	}
	
	type_database *get_type_database()
	{
		return &_type_database;
	}
		
	/// Called periodically by the platform windowing code, tick will update all the _players in the simulation as well as tick() the game's network interface.
	void tick()
	{
		net::time current_time = net::time::get_current();
		if(current_time == _last_time)
			return;
		
		float32 time_delta = (current_time - _last_time).get_milliseconds() / 1000.0f;
		for(int32 i = 0; i < _players.size(); i++)  
			_players[i]->update(time_delta, &_random);
		_net_interface->tick();
		_last_time = current_time;
	}
	
	/// move_my_player_to is called by the platform windowing code in response to
	/// user input.
	void move_my_player_to(position new_position)
	{
		if(_is_server)
		{
			_server_player->server_set_position(_server_player->_render_pos, new_position, 0, 0.2f);
		}
		else if(!_net_interface->_connection_to_server.is_null())
		{
			((test_connection *) _net_interface->_connection_to_server)->rpc(&test_connection::rpc_move_my_player_to, new_position.x, new_position.y);
			logprintf("posting new position (%g, %g) to server", float32(new_position.x), float32(new_position.y));
			//if(!_client_player.is_null())
			//	_client_player->rpcPlayerWillMove("Whee! Foo!");
			//_net_interface->connection_to_server->rpcSetPlayerPos(new_position.x, new_position.y);
		}
	}
};
