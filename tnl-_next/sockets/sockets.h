// Copyright Mark Frohnmayer and GarageGames.  See /license/info.txt in this distribution for licensing terms.

static bool sockets_init()
{
	static bool success = true;
#if defined PLATFORM_WIN32
	static bool initialized = false;

	if(!initialized)
	{
		initialized = true;
		WSADATA startup_wsa_data;
		success = !WSAStartup(0x0101, &startup_wsa_data);
	}
#endif
	return success;
}

#include "sockets/address.h"
#include "sockets/udp_socket.h"

static void sockets_unit_test()
{
	address_unit_test();
	udp_socket_unit_test();
}
