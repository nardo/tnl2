//-----------------------------------------------------------------------------------
//
//   Torque Network Library - Master Server
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

#include <stdio.h>
#include <stdlib.h>
#include "tnl.h"
#include "tnlLog.h"
using namespace TNL;

extern U32 gMasterPort;

extern Vector<char *> MOTDTypeVec;
extern Vector<char *> MOTDStringVec;

void processConfigLine(int argc, const char **argv)
{
   if(!stricmp(argv[0], "port") && argc > 1)
   {
      gMasterPort = atoi(argv[1]);
   }
   else if(!stricmp(argv[0], "motd") && argc > 2)
   {
      char *type = strdup(argv[1]);
      char *message = strdup(argv[2]);

      MOTDTypeVec.push_back(type);
      MOTDStringVec.push_back(message);
   }
}

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

int parseArgs(const char *string)
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
      processConfigLine(argc, (const char **) argv);
   argc = 0;
   argLen = 0;
   if(c)
      goto stateEatingWhitespace;
   return numObjects;
}      

void readConfigFile()
{
   FILE *f = fopen("master.cfg", "r");
   if(!f)
   {
      logprintf("Unable to open config file.");
      return;
   }
   for(S32 i = 0; i < MOTDTypeVec.size();i++)
   {
      free(MOTDTypeVec[i]);
      free(MOTDStringVec[i]);
   }
   MOTDTypeVec.clear();
   MOTDStringVec.clear();

   char fileData[32768];

   size_t bytesRead = fread(fileData, 1, sizeof(fileData), f);
   fileData[bytesRead] = 0;

   parseArgs(fileData);

   fclose(f);
}

