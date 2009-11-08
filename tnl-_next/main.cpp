// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "sqlite/sqlite3.h"
#include "tomcrypt.h"
#include "platform/platform.h"

namespace nat_discovery
{
#include "core/core.h"
#include "sockets/sockets.h"
#include "crypto/crypto.h"
#include "nat_discovery.h"
};

int main(int argc, const char **argv)
{
	return nat_discovery::run(argc, argv);
}
