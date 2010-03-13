/// The player is an example of an object that can move around and update its position to clients in the system.  Each client in the server controls a player instance constructed for it upon joining the _game.  Client users click on the _game window, which generates an event to the server to move that client's player to the desired point.
class test_game;
class player : public net_object
{
	typedef net_object parent;
public:
	declare_dynamic_class()

	position _start_pos, _end_pos; ///< All players move along a line from _start_pos to _end_pos.
	position _render_pos; ///< position at which to render the player - computed during update().
	float32 _t; ///< Parameter of how far the player is along the line from _start_pos to _end_pos.
	float32 _t_delta; ///< Change in _t per second (ie, velocity).
	test_game *_game; ///< The _game object this player is associated with
	
	/// Enumeration of possible player types in the game.
	enum player_type {
		player_type_ai, ///< The player is an AI controlled player on the server.
		player_type_ai_dummy, ///< The player is the ghost of an AI controlled player on the server.
		player_type_client, ///< The player is owned by a client.  If this is a ghost, it is owned by a user other than this client.
		player_type_my_client, ///< The player controlled by this client.
	};
	
	player_type _player_type; ///< What type of player is this?
	
	/// player constructor, assigns a random position in the playing field to the player, and if it is AI controlled will pick a first destination point.
	player(player_type pt = player_type_ai_dummy, random_generator *random_gen = 0)
	{
		// assign a random starting position for the player.
		if(random_gen)
		{
			_start_pos.x = random_gen->random_unit_float();
			_start_pos.y = random_gen->random_unit_float();			
		}
		else
		{
			_start_pos.x = _start_pos.y = 0;
		}
		_end_pos.x = _start_pos.x;
		_end_pos.y = _start_pos.y;
		_render_pos = _start_pos;
		
		// preset the movement parameter for the player to the end of the path
		_t = 1.0;
		
		_t_delta = 0;
		_player_type = pt;

		_game = NULL;
	}
	
	/// player destructor removes the player from the _game.
	~player()
	{
		if(!_game)
			return;
		
		// remove the player from the list of players in the _game.
		for( int32 i = 0; i < _game->_players.size(); i++)
		{
			if(_game->_players[i] == this)
			{
				_game->_players.erase_unstable(i);
				return;
			}
		}
	}
	
	
	/// State indexes
	enum  {
		initial_state = 1,  ///< This mask bit is never set explicitly, so it can be used for initialization data.
		position_state = 2, ///< This mask bit is set when the position information changes on the server.
	};
	
	/// perform_scope_query is called to determine which objects are "in scope" for the client that controls this player instance.  In the TNLTest program, all objects in a circle of radius 0.25 around the scope object are considered to be in scope.
	void perform_scope_query(ghost_connection *connection)
	{
		// find all the objects that are "in scope" - for the purposes of this test program, all buildings are considered to be in scope always, as well as all "players" in a circle of radius 0.25 around the scope object, or a radius squared of 0.0625
		
		for(int32 i = 0; i < _game->_buildings.size(); i++)
			connection->object_in_scope(_game->_buildings[i]);
		
		for(int32 i = 0; i < _game->_players.size(); i++)
		{
			position player_position = _game->_players[i]->_render_pos;
			float32 dx = player_position.x - _render_pos.x;
			float32 dy = player_position.y - _render_pos.y;
			float32 dist_squared = dx * dx + dy * dy;
			if(dist_squared < 0.0625)
				connection->object_in_scope(_game->_players[i]);
		}
	}

	static void register_class(type_database &the_database)
	{
		tnl_begin_class(the_database, player, net_object, true);
		tnl_compound_slot(the_database, player, _start_pos, position_state);
		tnl_compound_slot(the_database, player, _end_pos, position_state);
		tnl_slot(the_database, player, _t, position_state);
		tnl_slot(the_database, player, _t_delta, position_state);
		tnl_end_class(the_database);
	}
	
	/// server_set_position is called on the server when it receives notice from a client to change the position of the player it controls.  server_set_position will call setMaskBits(PositionMask) to notify the network system that this object has changed state.
	void server_set_position(position start_pos, position end_pos, float32 t, float32 t_delta)
	{
		// update the instance variables of the object
		_start_pos = start_pos;
		_end_pos = end_pos;
		_t = t;
		_t_delta = t_delta;
		
		// notify the network system that the position state of this object has changed:
		set_dirty_state(position_state);
		
		// // call a quick RPC to all the connections that have this object in scope
		//rpcPlayerDidMove(end_pos.x, end_pos.y);
	}
	void on_ghost_update(uint32 mask_bits)
	{
		logprintf("Got ghost update (%g, %g)->(%g, %g), %g, %g", float(_start_pos.x), float(_start_pos.y), float(_end_pos.x), float(_end_pos.y), _t, _t_delta);
		update(0, 0);
	}

	/// Move this object along its path.
	///
	/// If it hits the end point, and it's an AI, it will generate a new destination.
	void update(float32 time_delta, random_generator *random_gen)
	{
		_t += _t_delta * time_delta;
		if(_t >= 1.0)
		{
			_t = 1.0;
			_t_delta = 0;
			_render_pos = _end_pos;
			// if this is an AI player on the server, 
			if(_player_type == player_type_ai && random_gen)
			{
				_start_pos = _render_pos;
				_t = 0;
				_end_pos.x = random_gen->random_unit_float();
				_end_pos.y = random_gen->random_unit_float();
				_t_delta = 0.2f + random_gen->random_unit_float() * 0.1f;
				set_dirty_state(position_state); // notify the network system that the network state has been updated
			}
		}
		_render_pos.x = _start_pos.x + (_end_pos.x - _start_pos.x) * _t;
		_render_pos.y = _start_pos.y + (_end_pos.y - _start_pos.y) * _t;
	}
	
	/// on_ghost_available is called on the server when it knows that this player has been constructed on the specified client as a result of being "in scope".  In TNLTest this method call is used to test the per-ghost targeted RPC functionality of NetObject subclasses by calling rpcPlayerIsInScope on the player ghost on the specified connection.
	void on_ghost_available(ghost_connection *the_connection)
	{
		if(the_connection->get_scope_object() == this)
		{
			// once the scope object is successfully ghosted to the client, send and rpc with the ghost index to that connection, so it knows which player it controls.
			//rpc(&ghost_connection::rpc_end_ghosting);
			the_connection->rpc(&test_connection::rpc_set_control_object, enumeration<1024>(the_connection->get_ghost_index(this)));

		}
		// this function is called every time a ghost of this object is known to be available on a given client. we'll use this to demonstrate targeting a NetObject RPC to a specific connection.  Normally a RPC method marked as RPCToGhost will be broadcast to ALL ghosts of the object currently in scope.
		
		// first we construct an event that will represent the RPC call... the RPC macros create a RPCNAME_construct function that returns a NetEvent that encapsulates the RPC.
		// TNL::NetEvent *theRPCEvent = TNL_RPC_CONSTRUCT_NETEVENT(this, rpcPlayerIsInScope, (_render_pos.x, _render_pos.y));
		
		// then we can just post the event to whatever connections we want to have receive the message.  In the case of NetObject RPCs, if the source object is not ghosted, the RPC will just be silently dropped.
		// the_connection->postNetEvent(theRPCEvent);
	}
	
	/// on_ghost_add is called for every NetObject on the client after the ghost has been constructed and its initial unpackUpdate is called.  A return value of false from this function would indicate than an error had occured, notifying the network system that the connection should be terminated.
	bool on_ghost_add(ghost_connection *the_connection)
	{
		add_to_game(((test_net_interface *) the_connection->get_interface())->_game);
		return true;
	}
		
	/// Adds this player to the list of player instances in the specified _game.
	void add_to_game(test_game *the_game)
	{
		// add the player to the list of players in the _game.
		the_game->_players.push_back(this);
		_game = the_game;
		
		if(_player_type == player_type_my_client)
		{
			// set the client player for the _game for drawing the scoping radius circle.
			_game->_client_player = this;
		}
	}
	
	/*
	/// rpcPlayerWillMove is used in TNLTest to demonstrate ghost->parent NetObject RPCs.
	TNL_DECLARE_RPC(rpcPlayerWillMove,  (TNL::StringPtr testString))
	{
		TNL::logprintf("Expecting a player move from the connection: %s", testString.getString());
	}
	
	/// rpcPlayerDidMove is used in TNLTest to demostrate a broadcast RPC from the server to all of the ghosts on clients scoping this object.  It also demonstrates the usage of the Float<> template argument, in this case using 6 bits for each X and Y position.  Float arguments to RPCs are between 0 and 1.
	TNL_DECLARE_RPC(rpcPlayerDidMove,   (TNL::Float<6> x, TNL::Float<6> y))
	{
		float32 fx = x, fy = y;
		TNL::logprintf("A player moved to %g, %g", fx, fy);
	}
	
	/// rpcPlayerIsInScope is the RPC method called by on_ghost_available to demonstrate targeted NetObject RPCs.  on_ghost_available uses the TNL_RPC_CONSTRUCT_NETEVENT macro to construct a NetEvent from the RPC invocation and then posts it only to the client for which the object just came into scope.
	TNL_DECLARE_RPC(rpcPlayerIsInScope, (TNL::Float<6> x, TNL::Float<6> y))
	{
		float32 fx = x, fy = y;
		TNL::logprintf("A player is now in scope at %g, %g", fx, fy);
	}
	
	/// This macro invocation declares the player class to be known to the TNL class management system.
	TNL_DECLARE_CLASS(player);*/
};

