// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.
#include <math.h>
#include <stdio.h>
#include <iostream>
#include "tomcrypt.h"
#include "core/platform.h"
#include "torque_sockets/torque_sockets_c_api.h"

namespace core
{
	#include "core/core.h"
	struct net {
		#include "torque_sockets/torque_sockets.h"
	};
};
namespace core
{
	struct tnl {
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
		#if defined(GL_VERSION_1_4)
			#include "test_game_render_frame_open_gl.h"
		#elif defined(GL_ES_VERSION_2_0)
			#include "test_game_render_frame_gl_es_2.h"
		#endif 
	};

};
using namespace core;
