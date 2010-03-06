#include <OpenGL/gl.h>
#include <math.h>
#include <stdio.h>

#include <iostream>

// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "tomcrypt.h"
#include "platform/platform.h"
#include "torque_sockets_ref.h"

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
	void logprintf(const char *format, ...)
	{
		char buffer[4096];
		uint32 bufferStart = 0;
		va_list s;
		va_start( s, format );
		int32 len = vsnprintf(buffer + bufferStart, sizeof(buffer) - bufferStart, format, s);
		printf("LOG: %s\n", buffer);
		va_end(s);
	}
		
	template <typename signature> uint32 hash_method(signature the_method)
	{
		return hash_buffer((void *) &the_method, sizeof(the_method));
	}
	struct net {
		#include "net/net.h"
	};
};

#include "torque_sockets_implementation.h"

namespace core
{
	struct tnl : net {	
		#include "tnl2.h"
	};
	
	struct tnl_test : tnl
	{
		struct position {
			float x, y;
		};
		#include "test_player.h"
		#include "test_building.h"
		#include "test_connection.h"
		#include "test_game.h"
		#include "test_net_interface.h"
	};

	tnl_test::test_game *server_game = 0;
	tnl_test::test_game *client_game = 0;
};

using namespace core;

void init_game()
{
	SOCKADDR interface_bind_address, ping_address;
	net::address a;
	a.to_sockaddr(&interface_bind_address);
	a.to_sockaddr(&ping_address);
	client_game = new tnl_test::test_game(true, interface_bind_address, ping_address);
}

void click_game(float x, float y)
{

}

void tick_game()
{
	client_game->tick();
}

/// renderFrame is called by the platform windowing code to notify the game
/// that it should render the current world using the specified window area.
void render_game_scene()
{
	client_game->render_frame();
}

