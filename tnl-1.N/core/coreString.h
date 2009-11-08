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

#ifndef _CORE_STRING_H_
#define _CORE_STRING_H_

#include <string.h>

namespace TNL
{

class StringPtr
{
   char *mStringData;
   inline void alloc(const char *string)
   {
      if(string)
      {
         mStringData = new char[strlen(string) + 1];
         strcpy(mStringData, string);
      }
      else
         mStringData = NULL;
   }
public:
   StringPtr()
   {
      mStringData = NULL;
   }
   StringPtr(const char *string)
   {
      alloc(string);
   }
   StringPtr(const StringPtr &string)
   {
      alloc(string.mStringData);
   }
   ~StringPtr()
   {
      delete[] mStringData;
   }
   StringPtr &operator=(const StringPtr &ref)
   {
      delete[] mStringData;
      alloc(ref.mStringData);
      return *this;
   }
   StringPtr &operator=(const char *string)
   {
      delete[] mStringData;
      alloc(string);
      return *this;
   }
   operator const char *() const
   {
      if(mStringData)
         return mStringData;
      else
         return "";
   }
   operator bool () const
   {
      return mStringData != 0;
   }
   const char *getString() const
   {
      if(mStringData)
         return mStringData;
      else
         return "";
   }
};

};

#endif
