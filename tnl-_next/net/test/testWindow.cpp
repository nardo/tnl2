// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "testGame.h"
#include "util/utilLog.h"
#undef _DLL
#include "../thirdparty/glut/glutInclude.h"

static const char *localBroadcastAddress = "IP:broadcast:28999";
static const char *localHostAddress = "IP:localhost:28999";

void createGameClient(bool pingLocalHost = false)
{
   delete clientGame;
   delete serverGame;

   // create a client, by default pinging the LAN broadcast on port 28999
   clientGame = new TestGame(false,Torque::NetAddress(Torque::NetAddress::IPProtocol, Torque::NetAddress::Any, 0),Torque::NetAddress(pingLocalHost ? localHostAddress : localBroadcastAddress));
   serverGame = NULL;
}

void createGameServer()
{
   delete clientGame;
   delete serverGame;

   clientGame = NULL;
   // create a server, listening on port 28999.
   serverGame = new TestGame(true,Torque::NetAddress(Torque::NetAddress::IPProtocol,Torque::NetAddress::Any, 28999),Torque::NetAddress(localBroadcastAddress));
}

void createGameClientServer()
{
   delete clientGame;
   delete serverGame;

   // construct two local games, one client and one server, and have the client game ping the localhost
   // loopback address.
   clientGame = new TestGame(false,Torque::NetAddress(Torque::NetAddress::IPProtocol, Torque::NetAddress::Any, 0),Torque::NetAddress(localHostAddress));
   serverGame = new TestGame(true,Torque::NetAddress(Torque::NetAddress::IPProtocol,Torque::NetAddress::Any, 28999),Torque::NetAddress(localBroadcastAddress));

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
      p.x = x / Torque::F32(gWindowWidth);
      p.y = y / Torque::F32(gWindowHeight);

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

TORQUE_STARTUPEXECUTE(SomeThunk)
{
   printf("TNLTest exercising the Thunk macro -- this should print before main().\n");
}
class StdoutLogConsumer : public Torque::LogConsumer
{
public:
   void logString(const Torque::String &string)
   {
      printf("%s\r\n", string.c_str());
   }
} gStdoutLogConsumer;

int main(int argc, char **argv)
{
   printf("First line of main() -- After the thunk!\n");
   TorqueLogEnable(LogNetBase, true);
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

