#include <OpenGL/gl.h>
#include <math.h>


#include <iostream>

// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "torque_sockets.h"
#include "tomcrypt.h"
#include "platform/platform.h"
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
	
	template <typename signature> uint32 hash_method(signature the_method)
	{
		return hash_buffer((void *) &the_method, sizeof(the_method));
	}
	
	
	
	struct net {
#include "net/net.h"
	};
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
};

struct Position
{
    float x, y;
};

/// renderFrame is called by the platform windowing code to notify the game
/// that it should render the current world using the specified window area.
void render_game_scene()
{
	glClearColor(1, 1, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glMatrixMode(GL_MODELVIEW);
	
	// first, render the alpha blended circle around the player,
	// to show the scoping range.
	
        if(true) //_client_player)
	{
                Position p;
                p.x = 0.7; p.y = 0.3; //_client_player->renderPos;
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBegin(GL_POLYGON);
		glColor4f(0.5f, 0.5f, 0.5f, 0.65f);
                for(float r = 0; r < 3.1415 * 2; r += 0.1f)
		{
			glVertex2f(p.x + 0.25f * cos(r), p.y + 0.25f * sin(r));
		}
		
		glEnd();
		glDisable(GL_BLEND);
	}
        /*
	// then draw all the _buildings.
	for(int32 i = 0; i < _buildings.size(); i++)
	{
		building *b = _buildings[i];
		glBegin(GL_POLYGON);
		glColor3f(1, 0, 0);
		glVertex2f(b->upperLeft.x, b->upperLeft.y);
		glVertex2f(b->lowerRight.x, b->upperLeft.y);
		glVertex2f(b->lowerRight.x, b->lowerRight.y);
		glVertex2f(b->upperLeft.x, b->lowerRight.y);
		glEnd();
	}
	
	// last, draw all the _players in the game.
	for(int32 i = 0; i < _players.size(); i++)
	{
		player *p = _players[i];
		glBegin(GL_POLYGON);
		glColor3f(0,0,0);
		
		glVertex2f(p->renderPos.x - 0.012f, p->renderPos.y - 0.012f);
		glVertex2f(p->renderPos.x + 0.012f, p->renderPos.y - 0.012f);
		glVertex2f(p->renderPos.x + 0.012f, p->renderPos.y + 0.012f);
		glVertex2f(p->renderPos.x - 0.012f, p->renderPos.y + 0.012f);
		glEnd();
		
		glBegin(GL_POLYGON);
		switch(p->myPlayerType)
		{
			case player::PlayerTypeAI:
			case player::PlayerTypeAIDummy:
				glColor3f(0, 0, 1);
				break;
			case player::PlayerTypeClient:
				glColor3f(0.5, 0.5, 1);
				break;
			case player::PlayerTypeMyClient:
				glColor3f(1, 1, 1);
				break;
		}
		glVertex2f(p->renderPos.x - 0.01f, p->renderPos.y - 0.01f);
		glVertex2f(p->renderPos.x + 0.01f, p->renderPos.y - 0.01f);
		glVertex2f(p->renderPos.x + 0.01f, p->renderPos.y + 0.01f);
		glVertex2f(p->renderPos.x - 0.01f, p->renderPos.y + 0.01f);
		
		glEnd();
        }*/
}

