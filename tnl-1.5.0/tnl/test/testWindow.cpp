//-----------------------------------------------------------------------------------
//
//   Torque Network Library - TNLTest example program
//   Copyright (C) 2004 GarageGames.com, Inc.
//   Portions of this file (c) 1994 Andrew Davison
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


#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "testGame.h"
#include "../tnl/tnlLog.h"

#include "../glut/glutInclude.h"

static const char *localBroadcastAddress = "IP:broadcast:28999";
static const char *localHostAddress = "IP:localhost:28999";

void createGameClient(bool pingLocalHost = false)
{
   delete clientGame;
   delete serverGame;

   // create a client, by default pinging the LAN broadcast on port 28999
   clientGame = new TestGame(false,TNL::Address(TNL::IPProtocol, TNL::Address::Any, 0),TNL::Address(pingLocalHost ? localHostAddress : localBroadcastAddress));
   serverGame = NULL;
}

void createGameServer()
{
   delete clientGame;
   delete serverGame;

   clientGame = NULL;
   // create a server, listening on port 28999.
   serverGame = new TestGame(true,TNL::Address(TNL::IPProtocol,TNL::Address::Any, 28999),TNL::Address(localBroadcastAddress));
}

void createGameClientServer()
{
   delete clientGame;
   delete serverGame;

   // construct two local games, one client and one server, and have the client game ping the localhost
   // loopback address.
   clientGame = new TestGame(false,TNL::Address(TNL::IPProtocol, TNL::Address::Any, 0),TNL::Address(localHostAddress));
   serverGame = new TestGame(true,TNL::Address(TNL::IPProtocol,TNL::Address::Any, 28999),TNL::Address(localBroadcastAddress));

   // If the following line is commented out, the local games will create a normal (network) connection.
   // the createLocalConnection call constructs a short-circuit connection that works without
   // network access at all.
   clientGame->createLocalConnection(serverGame);
}

void onExit()
{
   delete clientGame;
   delete serverGame;
}


int gWindowWidth = 400;
int gWindowHeight = 400;

void reshape(int nw, int nh)
{
   gWindowWidth = nw;
   gWindowHeight = nh;
}

void mouse(int button, int state, int x, int y)
{
   static int mouseState=0;

   if(!state) // mouse up
   {
      Position p;
      p.x = x / TNL::F32(gWindowWidth);
      p.y = y / TNL::F32(gWindowHeight);

      if(clientGame)
         clientGame->moveMyPlayerTo(p);
      else if(serverGame)
         serverGame->moveMyPlayerTo(p);
   }
}


void idle()
{
   if(clientGame)
      clientGame->tick();
   if(serverGame)
      serverGame->tick();
   glutPostRedisplay();
}

void display()
{
   if(clientGame)
      clientGame->renderFrame(gWindowWidth, gWindowHeight);
   else if(serverGame)
      serverGame->renderFrame(gWindowWidth, gWindowHeight);
   glFlush();
   glutSwapBuffers();
}

void menu(int value)
{
   switch(value)
   {
      case 1:
         createGameClient(false);
         break;
      case 2:
         createGameServer();
         break;
      case 3:
         createGameClientServer();
         break;
      case 4:
         createGameClient(true);
         break;
   }
}

#include <stdio.h>
class StdoutLogConsumer : public TNL::LogConsumer
{
public:
   void logString(const char *string)
   {
      printf("%s\r\n", string);
   }
} gStdoutLogConsumer;

int main(int argc, char **argv)
{
   TNLLogEnable(LogGhostConnection, true);
   createGameClient();
   glutInitWindowSize(gWindowWidth, gWindowHeight);
   glutInit(&argc, argv);

   glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
   glutCreateWindow("TNLTest - Right Button for Menu");
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutMouseFunc(mouse);
   glutIdleFunc(idle);
   atexit(onExit);

   glutCreateMenu(menu);
   glutAddMenuEntry("Restart as client", 1);
   glutAddMenuEntry("Restart as server", 2);
   glutAddMenuEntry("Restart as client/server", 3);
   glutAddMenuEntry("Restart as client pinging localhost", 4);
   glutAttachMenu( GLUT_RIGHT_BUTTON );

   glutMainLoop();
}

