#include <OpenGL/gl.h>
#include <math.h>
#include <stdio.h>

#include <iostream>

// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "tomcrypt.h"
#include "core/platform.h"
#include "torque_sockets/torque_socket_event.h"

namespace core
{
	#include "core/core.h"
	
	uint32 hash_buffer(const void *buffer, uint32 len)
	{
		uint8 *buf = (uint8 *) buffer;
		uint32 result = 0;
		while(len--)
			result = ((result << 8) | (result >> 24)) ^ uint32(*buf++);
		return result;
	}
	int _log_index = 0;

	void logprintf(const char *format, ...)
	{
		char buffer[4096];
		uint32 bufferStart = 0;
		va_list s;
		va_start( s, format );
		int32 len = vsnprintf(buffer + bufferStart, sizeof(buffer) - bufferStart, format, s);
		printf("LOG %d: %s\n", _log_index, buffer);
		va_end(s);
		fflush(stdout);
	}
		
	template <typename signature> uint32 hash_method(signature the_method)
	{
		return hash_buffer((void *) &the_method, sizeof(the_method));
	}
	struct net {
		#include "torque_sockets/torque_sockets.h"
	};
};

//#include "torque_sockets_implementation.h"

namespace core
{
	struct tnl : net {	
		#include "tnl2.h"
	};

	struct tnl_test : tnl
	{
		struct position {
			unit_float<12> x, y;
		};
		#include "test_player.h"
		#include "test_building.h"
		#include "test_connection.h"
		#include "test_game.h"
		#include "test_net_interface.h"
	};

	tnl_test::test_game *game[2] = { 0, 0 };
};

using namespace core;

void restart_games(bool game_1_is_server, bool game_2_is_server)
{
	ltc_mp = ltm_desc;
	if(game[0])
		delete game[0];
	if(game[1])
		delete game[1];

	SOCKADDR interface_bind_address1, interface_bind_address2, ping_address;
	net::address a;
	a.set_port(28000);
	a.to_sockaddr(&ping_address);
	if(game_1_is_server)
		a.to_sockaddr(&interface_bind_address1);
	else
	{
		a.set_port(0);
		a.to_sockaddr(&interface_bind_address1);
	}
	a.set_port(28001);
	if(game_2_is_server)
		a.to_sockaddr(&interface_bind_address2);
	{
		a.set_port(0);
		a.to_sockaddr(&interface_bind_address2);
	}
	game[0] = new tnl_test::test_game(game_1_is_server, interface_bind_address1, ping_address);
	game[1] = new tnl_test::test_game(game_2_is_server, interface_bind_address2, ping_address);
}

void click_game(int game_index, float x, float y)
{
	tnl_test::position p;
	p.x = x;
	p.y = y;
	if(game[game_index])
		game[game_index]->move_my_player_to(p);
}

void tick_games()
{
	core::_log_index = 0;
	if(game[0])
		game[0]->tick();
	core::_log_index = 1;
	if(game[1])
		game[1]->tick();
}

/// renderFrame is called by the platform windowing code to notify the game
/// that it should render the current world using the specified window area.
void render_game_scene(int game_index)
{
	if(game[game_index])
		game[game_index]->render_frame();
}

