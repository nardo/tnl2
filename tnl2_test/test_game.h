/// The test_game class manages a TNLTest client or server instance.  test_game maintains a list of all the player and building objects in the playing area, and interfaces with the specific platform's windowing system to respond to user input and render the current game display.
class test_net_interface;
class test_game {
public:
	array<player *> _players; ///< vector of player objects in the game
	array<building *> _buildings; ///< vector of _buildings in the game
	bool _is_server; ///< was this game created to be a server?
	test_net_interface *_net_interface; ///< network interface for this game
	time _last_time; ///< last time that tick() was called
	safe_ptr<player> _server_player; ///< the player that the server controls
	safe_ptr<player> _client_player; ///< the player that this client controls, if this game is a client
	random_generator _random;
	
	/// Constructor for test_game, determines whether this game will be a client or a server, and what addresses to bind to and ping.  If this game is a server, it will construct 50 random _buildings and 15 random AI _players to populate the "world" with.  test_game also constructs an AsymmetricKey to demonstrate establishing secure connections with clients and servers.
	test_game(bool server, SOCKADDR &interface_bind_address, SOCKADDR &ping_address)
	{
		_is_server = server;
		_net_interface = new test_net_interface(this, _is_server, interface_bind_address, ping_address);
		//TNL::AsymmetricKey *theKey = new TNL::AsymmetricKey(32);
		//_net_interface->setPrivateKey(theKey);
		//_net_interface->setRequiresKeyExchange(true);
		
		_last_time = time::get_current();
		
		if(_is_server)
		{
			// generate some _buildings and AIs:
			for(int32 i = 0; i < 50; i ++)
			{
				building *the_building = new building;
				the_building->add_to_game(this);
			}
			for(int32 i = 0; i < 15; i ++)
			{
				player *ai_player = new player(player::player_type_ai);
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
		
	/// Called periodically by the platform windowing code, tick will update all the _players in the simulation as well as tick() the game's network interface.
	void tick()
	{
		time current_time = time::get_current();
		if(current_time == _last_time)
			return;
		
		float32 time_delta = (current_time - _last_time).get_milliseconds() / 1000.0f;
		for(int32 i = 0; i < _players.size(); i++)  
			_players[i]->update(time_delta, _random);
		_net_interface->tick();
		_last_time = current_time;
	}
	
	/// renderFrame is called by the platform windowing code to notify the game
	/// that it should render the current world using the specified window area.
	void renderFrame()
	{
		glClearColor(1, 1, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		// first, render the alpha blended circle around the player,
		// to show the scoping range.
		
		if(_client_player)
		{
			position p = _client_player->_render_pos;
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBegin(GL_POLYGON);
			glColor4f(0.5f, 0.5f, 0.5f, 0.65f);
			for(float32 r = 0; r < 3.1415 * 2; r += 0.1f)
			{
				glVertex2f(p.x + 0.25f * cos(r), p.y + 0.25f * sin(r));
			}
			
			glEnd();
			glDisable(GL_BLEND);
		}
		
		// then draw all the _buildings.
		for(int32 i = 0; i < _buildings.size(); i++)
		{
			building *b = _buildings[i];
			glBegin(GL_POLYGON);
			glColor3f(1, 0, 0);
			glVertex2f(b->upper_left.x, b->upper_left.y);
			glVertex2f(b->lower_right.x, b->upper_left.y);
			glVertex2f(b->lower_right.x, b->lower_right.y);
			glVertex2f(b->upper_left.x, b->lower_right.y);
			glEnd();
		}
		
		// last, draw all the _players in the game.
		for(int32 i = 0; i < _players.size(); i++)
		{
			player *p = _players[i];
			glBegin(GL_POLYGON);
			glColor3f(0,0,0);
			
			glVertex2f(p->_render_pos.x - 0.012f, p->_render_pos.y - 0.012f);
			glVertex2f(p->_render_pos.x + 0.012f, p->_render_pos.y - 0.012f);
			glVertex2f(p->_render_pos.x + 0.012f, p->_render_pos.y + 0.012f);
			glVertex2f(p->_render_pos.x - 0.012f, p->_render_pos.y + 0.012f);
			glEnd();
			
			glBegin(GL_POLYGON);
			switch(p->_player_type)
			{
				case player::player_type_ai:
				case player::player_type_ai_dummy:
					glColor3f(0, 0, 1);
					break;
				case player::player_type_client:
					glColor3f(0.5, 0.5, 1);
					break;
				case player::player_type_my_client:
					glColor3f(1, 1, 1);
					break;
			}
			glVertex2f(p->_render_pos.x - 0.01f, p->_render_pos.y - 0.01f);
			glVertex2f(p->_render_pos.x + 0.01f, p->_render_pos.y - 0.01f);
			glVertex2f(p->_render_pos.x + 0.01f, p->_render_pos.y + 0.01f);
			glVertex2f(p->_render_pos.x - 0.01f, p->_render_pos.y + 0.01f);
			
			glEnd();
		}
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
			logprintf("posting new position (%g, %g) to server", new_position.x, new_position.y);
			//if(!_client_player.is_null())
			//	_client_player->rpcPlayerWillMove("Whee! Foo!");
			//_net_interface->connection_to_server->rpcSetPlayerPos(new_position.x, new_position.y);
		}
	}
};