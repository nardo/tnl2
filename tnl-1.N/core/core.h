//-----------------------------------------------------------------------------------
//
//   Torque Core Library
//   Copyright (C) 2005 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _CORE_H_
#define _CORE_H_

/// The Core namespace contains utility classes and base types used by Torque engine
/// components.
namespace TNL
{
};

#if defined(_DEBUG)
#include <crtdbg.h>
#endif

#ifndef _CORE_TYPES_H_
#include "core/coreTypes.h"
#endif

#ifndef _CORE_ASSERT_H_
#include "core/coreAssert.h"
#endif

#ifndef _CORE_ENDIAN_H_
#include "core/coreEndian.h"
#endif

#ifndef _CORE_PLATFORM_H_
#include "core/corePlatform.h"
#endif

#ifndef _CORE_HELPERS_H_
#include "core/coreHelpers.h"
#endif

#endif