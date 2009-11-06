//-----------------------------------------------------------------------------------
//
//   Torque Network Library - ZAP example multiplayer vector graphics space game
//   Copyright (C) 2004 GarageGames.com, Inc.
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

#include "gameLoader.h"
#include "tnl.h"
#include "tnlLog.h"

#include <stdio.h>

using namespace TNL;
namespace Zap
{

enum {
   MaxArgc = 128,
   MaxArgLen = 100,
};

static char *argv[MaxArgc];
static char argv_buffer[MaxArgc][MaxArgLen];
static int argc;
static int argLen = 0;
static const char *argString;

inline char getNextChar()
{
   while(*argString == '\r')
      argString++;
   return *argString++;
}

inline void addCharToArg(char c)
{
   if(argc < MaxArgc && argLen < MaxArgLen-1)
   {
      argv[argc][argLen] = c;
      argLen++;
   }
}

inline void addArg()
{
   if(argc < MaxArgc)
   {
      argv[argc][argLen] = 0;
      argc++;
      argLen = 0;
   }
}

int LevelLoader::parseArgs(const char *string)
{
   int numObjects = 0;

   argc = 0;
   argLen = 0;
   argString = string;
   char c;

   for(U32 i = 0; i < MaxArgc; i++)
      argv[i] = argv_buffer[i];

stateEatingWhitespace:
   c = getNextChar();
   if(c == ' ' || c == '\t')
      goto stateEatingWhitespace;
   if(c == '\n' || !c)
      goto stateLineParseDone;
   if(c == '\"')
      goto stateReadString;
   if(c == '#')
      goto stateEatingComment;
stateAddCharToIdent:
   addCharToArg(c);
   c = getNextChar();
   if(c == ' ' || c == '\t')
   {
      addArg();
      goto stateEatingWhitespace;
   }
   if(c == '\n' || !c)
   {
      addArg();
      goto stateLineParseDone;
   }
   if(c == '\"')
   {
      addArg();
      goto stateReadString;
   }
   goto stateAddCharToIdent;
stateReadString:
   c = getNextChar();
   if(c == '\"')
   {
      addArg();
      goto stateEatingWhitespace;
   }
   if(c == '\n' || !c)
   {
      addArg();
      goto stateLineParseDone;
   }
   if(c == '\\')
   {
      c = getNextChar();
      if(c == 'n')
      {
         addCharToArg('\n');
         goto stateReadString;
      }
      if(c == 't')
      {
         addCharToArg('\t');
         goto stateReadString;
      }
      if(c == '\\')
      {
         addCharToArg('\\');
         goto stateReadString;
      }
      if(c == '\n' || !c)
      {
         addArg();
         goto stateLineParseDone;
      }
   }
   addCharToArg(c);
   goto stateReadString;
stateEatingComment:
   c = getNextChar();
   if(c != '\n' && c)
      goto stateEatingComment;
stateLineParseDone:
   if(argc)
      processLevelLoadLine(argc, (const char **) argv);
   argc = 0;
   argLen = 0;
   if(c)
      goto stateEatingWhitespace;
   return numObjects;
}      

void LevelLoader::initLevelFromFile(const char *file)
{
   FILE *f = fopen(file, "r");
   if(!f)
   {
      logprintf("Unable to open level file %s!!", file);
      return;
   }

   char fileData[32768];

   size_t bytesRead = fread(fileData, 1, sizeof(fileData), f);
   fileData[bytesRead] = 0;

   parseArgs(fileData);

   fclose(f);
}

};

