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

#include "tnl.h"
#include "tnlRandom.h"
#include "tnlGhostConnection.h"
#include "tnlNetInterface.h"
#include "tnlJournal.h"

#include "glutInclude.h"
#include <stdarg.h>

using namespace TNL;
#include "UI.h"
#include "UIGame.h"
#include "UINameEntry.h" 
#include "UIMenus.h"
#include "UIEditor.h"
#include "game.h"
#include "gameNetInterface.h"
#include "masterConnection.h"
#include "sfx.h"
#include "sparkManager.h"
#include "input.h"

#ifdef TNL_OS_MAC_OSX
#include <unistd.h>
#endif

namespace Zap
{

bool gIsCrazyBot = false;
bool gQuit = false;
bool gIsServer = false;
const char *gHostName = "ZAP Game";
const char *gWindowTitle = "ZAP II - The Return";
U32 gMaxPlayers = 128;
U32 gSimulatedPing = 0;
F32 gSimulatedPacketLoss = 0;
bool gDedicatedServer = false;

const char *gMasterAddressString = "IP:master.opentnl.org:29005";
const char *gServerPassword = NULL;
const char *gAdminPassword = NULL;

Address gMasterAddress;
Address gConnectAddress;
Address gBindAddress(IPProtocol, Address::Any, 28000);

const char *gLevelList = "retrieve1.txt "
                         "retrieve2.txt "
                         "retrieve3.txt "
                         "football1.txt "
                         "football2.txt "
                         "football3.txt "
                         "football4.txt "
                         "football5.txt "
                         "rabbit1.txt "
                         "soccer1.txt "
                         "ctf1.txt "
                         "ctf2.txt "
                         "ctf3.txt "
                         "ctf4.txt "
                         "hunters1.txt "
                         "hunters2.txt "
                         "zm1.txt "
                         ;

class ZapJournal : public Journal
{
public:
   TNL_DECLARE_JOURNAL_ENTRYPOINT(reshape, (S32 newWidth, S32 newHeight));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(motion, (S32 x, S32 y));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(passivemotion, (S32 x, S32 y));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(key, (U8 key));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(keyup, (U8 key));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(modifierkey, (U32 modkey));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(modifierkeyup, (U32 modkey));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(mouse, (S32 button, S32 state, S32 x, S32 y));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(specialkey, (S32 key));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(specialkeyup, (S32 key));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(idle, (U32 timeDelta));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(display, ());
   TNL_DECLARE_JOURNAL_ENTRYPOINT(startup, (Vector<StringPtr> theArgv));
};

ZapJournal gZapJournal;

void reshape(int nw, int nh)
{
   gZapJournal.reshape(nw, nh); 
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, reshape,
    (S32 newWidth, S32 newHeight), (newWidth, newHeight))
{
  UserInterface::windowWidth = newWidth;
  UserInterface::windowHeight = newHeight;
}

void motion(int x, int y)
{
   gZapJournal.motion(x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, motion,
   (S32 x, S32 y), (x, y))
{
   if(gIsCrazyBot)
      return;

   if(UserInterface::current)
      UserInterface::current->onMouseDragged(x, y);
}

void passivemotion(int x, int y)
{
   gZapJournal.passivemotion(x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, passivemotion,
   (S32 x, S32 y), (x, y))
{
   if(gIsCrazyBot)
      return;

   if(UserInterface::current)
      UserInterface::current->onMouseMoved(x, y);
}

void key(unsigned char key, int x, int y)
{
   gZapJournal.key(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, key, (U8 key), (key))
{
   // check for ALT-ENTER
   if(key == '\r' && (glutGetModifiers() & GLUT_ACTIVE_ALT))
      gOptionsMenuUserInterface.toggleFullscreen();
   else if(UserInterface::current)
      UserInterface::current->onKeyDown(key);
}

void keyup(unsigned char key, int x, int y)
{
   gZapJournal.keyup(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, keyup, (U8 key), (key))
{
   if(UserInterface::current)
      UserInterface::current->onKeyUp(key);
}

void mouse(int button, int state, int x, int y)
{
   gZapJournal.mouse(button, state, x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, mouse,
   (S32 button, S32 state, S32 x, S32 y), (button, state, x, y))
{
   static int mouseState[2] = { 0, };
   if(!UserInterface::current)
      return;

   if(gIsCrazyBot)
      return;

   if(button == GLUT_LEFT_BUTTON)
   {
      if(state == 1 && !mouseState[0])
      {
         UserInterface::current->onMouseUp(x, y);
         mouseState[0] = 0;
      }
      else
      {
         mouseState[0] = state;
         UserInterface::current->onMouseDown(x, y);
      }
   }
   else if(button == GLUT_RIGHT_BUTTON)
   {
      if(state == 1 && !mouseState[1])
      {
         UserInterface::current->onRightMouseUp(x, y);
         mouseState[1] = 0;
      }
      else
      {
         mouseState[1] = state;
         UserInterface::current->onRightMouseDown(x, y);
      }
   }
}

void specialkey(int key, int x, int y)
{
   gZapJournal.specialkey(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, specialkey, (S32 key), (key))
{
   if(UserInterface::current)
      UserInterface::current->onSpecialKeyDown(key);
}

void specialkeyup(int key, int x, int y)
{
   gZapJournal.specialkeyup(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, specialkeyup, (S32 key), (key))
{
   if(UserInterface::current)
      UserInterface::current->onSpecialKeyUp(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, modifierkey, (U32 key), (key))
{
   if(UserInterface::current)
      UserInterface::current->onModifierKeyDown(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, modifierkeyup, (U32 key), (key))
{
   if(UserInterface::current)
      UserInterface::current->onModifierKeyUp(key);
}

inline U32 RandomInt(U32 range)
{
   return U32((U32(rand()) / (F32(RAND_MAX) + 1)) * range);
}

inline U32 RandomBool()
{
   return (U32(rand()) / (F32(RAND_MAX) + 1)) > 0.5f;
}

extern void getModifierState( bool &shiftDown, bool &controlDown, bool &altDown );

void idle()
{
   // ok, since GLUT is L4m3 as far as modifier keys, we're going
   // to have to do this ourselves on each platform:
   static bool gShiftDown = false;
   static bool gControlDown = false;
   static bool gAltDown = false;

   bool sd, cd, ad;
   getModifierState(sd, cd, ad);
   if(sd != gShiftDown)
   {
      if(sd)
         gZapJournal.modifierkey(0);
      else
         gZapJournal.modifierkeyup(0);
      gShiftDown = sd;
   }
   if(cd != gControlDown)
   {
      if(cd)
         gZapJournal.modifierkey(1);
      else
         gZapJournal.modifierkeyup(1);
   }
   if(ad != gAltDown)
   {
      if(ad)
         gZapJournal.modifierkey(2);
      else
         gZapJournal.modifierkeyup(2);
   }
   static S64 lastTimer = Platform::getHighPrecisionTimerValue();
   static F64 unusedFraction = 0;

   S64 currentTimer = Platform::getHighPrecisionTimerValue();

   F64 timeElapsed = Platform::getHighPrecisionMilliseconds(currentTimer - lastTimer) + unusedFraction;
   U32 integerTime = U32(timeElapsed);

   if(integerTime >= 10)
   {
      lastTimer = currentTimer;
      unusedFraction = timeElapsed - integerTime;

      gZapJournal.idle(integerTime);
   }

   // Make us move all crazy like...
   if(gIsCrazyBot)
   {
      gIsCrazyBot = false; // Reenable input events
      static S64 lastMove = Platform::getHighPrecisionTimerValue();
      static const char keyBuffer[] = "wasdwasdwasdwasdwasdwasdwasdwasdwasd\r\r\r\r\r\r\rabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 \r\t\n?{}_+-=[]\\";

      static Vector<S8> keyDownVector;

      F64 delta = Platform::getHighPrecisionMilliseconds(currentTimer - lastMove);
      if(delta > 5)
      {
         U32 rklen = (U32) strlen(keyBuffer);
         // generate some random keys:
         for(S32 i = 0; i < 32; i++)
         {
            bool found = false;
            S8 key = keyBuffer[RandomInt(rklen)];
            for(S32 i = 0; i < keyDownVector.size(); i++)
            {
               if(keyDownVector[i] == key)
               {
                  keyDownVector.erase_fast(i);
                  gZapJournal.keyup(key);
                  found = true;
                  break;
               }
            }
            if(!found)
            {
               keyDownVector.push_back(key);
               gZapJournal.key(key);
            }
         }

         // Do mouse craziness
         S32 x = RandomInt(800);
         S32 y = RandomInt(600);
         gZapJournal.passivemotion(x,y);
         gZapJournal.mouse(0, RandomBool(), x, y);
         gZapJournal.mouse(1, RandomBool(), x, y);
         gZapJournal.mouse(2, RandomBool(), x, y);
         lastMove = currentTimer;
      }
      gIsCrazyBot = true; // Reenable input events
   }



   // Sleep a bit so we don't saturate the system. For a non-dedicated server,
   // sleep(0) helps reduce the impact of OpenGL on windows.
   U32 sleepTime = 1;

   if(gClientGame) sleepTime = 0;
   if(gIsCrazyBot) sleepTime = 10;

   Platform::sleep(sleepTime);
   gZapJournal.processNextJournalEntry();
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, idle, (U32 integerTime), (integerTime))
{
   if(UserInterface::current)
      UserInterface::current->idle(integerTime);
   if(gClientGame)
      gClientGame->idle(integerTime);
   if(gServerGame)
      gServerGame->idle(integerTime);
   if(gClientGame)
      glutPostRedisplay();
}

void dedicatedServerLoop()
{
   for(;;)
      idle();
}

void display(void)
{
   gZapJournal.display();
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, display, (), ())
{
   glFlush();
   UserInterface::renderCurrent();

   // Render master connection state...
   if(gClientGame && gClientGame->getConnectionToMaster() 
      && gClientGame->getConnectionToMaster()->getConnectionState() != NetConnection::Connected)
   {
      glColor3f(1,1,1);
      UserInterface::drawStringf(10, 550, 15, "Master Server - %s", 
                                 gConnectStatesTable[gClientGame->getConnectionToMaster()->getConnectionState()]);

   }
   glutSwapBuffers();
}

#include <stdio.h>
class StdoutLogConsumer : public LogConsumer
{
public:
   void logString(const char *string)
   {
      printf("%s\n", string);
   }
} gStdoutLogConsumer;

class FileLogConsumer : public LogConsumer
{
private:
   FILE *f;
public:
   FileLogConsumer(const char* logFile="zap.log")
   {
      f = fopen(logFile, "w");
      logString("------ Zap Log File ------");
   }

   ~FileLogConsumer()
   {
      if(f)
         fclose(f);
   }

   void logString(const char *string)
   {
      if(f)
      {
         fprintf(f, "%s\n", string);
         fflush(f);
      }
   }
} gFileLogConsumer;

void hostGame(bool dedicated, Address bindAddress)
{
   gServerGame = new ServerGame(bindAddress, gMaxPlayers, gHostName);
   gServerGame->setLevelList(gLevelList);

   if(!dedicated)
      joinGame(Address(), false, true);

}


void joinGame(Address remoteAddress, bool isFromMaster, bool local)
{
   if(isFromMaster && gClientGame->getConnectionToMaster())
   {
      gClientGame->getConnectionToMaster()->requestArrangedConnection(remoteAddress);
      gGameUserInterface.activate();
   }
   else
   {
      GameConnection *theConnection = new GameConnection();
      gClientGame->setConnectionToServer(theConnection);

      const char *name = gNameEntryUserInterface.getText();
      if(!name[0])
         name = "Playa";

      theConnection->setClientName(name);
      theConnection->setSimulatedNetParams(gSimulatedPacketLoss, gSimulatedPing);

      if(local)
      {
         theConnection->connectLocal(gClientGame->getNetInterface(), gServerGame->getNetInterface());
         // set the local connection to be an admin
         theConnection->setIsAdmin(true);
         GameConnection *gc = (GameConnection *) theConnection->getRemoteConnectionObject();
         gc->setIsAdmin(true);
      }
      else
         theConnection->connect(gClientGame->getNetInterface(), remoteAddress);
      gGameUserInterface.activate();
   }
}

void endGame()
{
   if(gClientGame && gClientGame->getConnectionToMaster())
      gClientGame->getConnectionToMaster()->cancelArrangedConnectionAttempt();

   if(gClientGame && gClientGame->getConnectionToServer())
      gClientGame->getConnectionToServer()->disconnect("");
   delete gServerGame;
   gServerGame = NULL;
}

void onExit()
{
   endGame();
   SFXObject::shutdown();
   ShutdownJoystick();
   NetClassRep::logBitUsage();
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, startup, (Vector<StringPtr> argv), (argv))
{
   bool hasClient = true;
   bool hasServer = false;
   bool connectLocal = false;
   bool connectRemote = false;
   bool nameSet = false;
   bool hasEditor = false;

   S32 argc = argv.size();

   for(S32 i = 0; i < argc;i+=2)
   {
      bool hasAdditionalArg = (i != argc - 1);

      if(!stricmp(argv[i], "-server"))
      {
         hasServer = true;
         connectLocal = true;
         if(hasAdditionalArg)
            gBindAddress.set(argv[i+1]);
      }
      else if(!stricmp(argv[i], "-connect"))
      {
         connectRemote = true;
         if(hasAdditionalArg)
            gConnectAddress.set(argv[i+1]);
      }
      else if(!stricmp(argv[i], "-master"))
      {
         if(hasAdditionalArg)
            gMasterAddressString = argv[i+1];
      }
      else if(!stricmp(argv[i], "-joystick"))
      {
         if(hasAdditionalArg)
            OptionsMenuUserInterface::joystickType = atoi(argv[i+1]);
      }
      else if(!stricmp(argv[i], "-loss"))
      {
         if(hasAdditionalArg)
            gSimulatedPacketLoss = atof(argv[i+1]);
      }
      else if(!stricmp(argv[i], "-lag"))
      {
         if(hasAdditionalArg)
            gSimulatedPing = atoi(argv[i+1]);
      }
      else if(!stricmp(argv[i], "-dedicated"))
      {
         hasClient = false;
         hasServer = true;
         gDedicatedServer = true;
         if(hasAdditionalArg)
            gBindAddress.set(argv[i+1]);
      }
      else if(!stricmp(argv[i], "-name"))
      {
         if(hasAdditionalArg)
         {
            nameSet = true;
            gNameEntryUserInterface.setText(argv[i+1]);
         }
      }
      else if(!stricmp(argv[i], "-password"))
      {
         if(hasAdditionalArg)
            gServerPassword = strdup(argv[i+1]);
      }
      else if(!stricmp(argv[i], "-adminpassword"))
      {
         if(hasAdditionalArg)
            gAdminPassword = strdup(argv[i+1]);
      }
      else if(!stricmp(argv[i], "-levels"))
      {
         if(hasAdditionalArg)
            gLevelList = strdup(argv[i+1]);
      }
      else if(!stricmp(argv[i], "-hostname"))
      {
         if(hasAdditionalArg)
            gHostName = strdup(argv[i+1]);
      }
      else if(!stricmp(argv[i], "-maxplayers"))
      {
         if(hasAdditionalArg)
            gMaxPlayers = atoi(argv[i+1]);
      }
      else if(!stricmp(argv[i], "-window"))
      {
         i--;
         OptionsMenuUserInterface::fullscreen = false;
      }
      else if(!stricmp(argv[i], "-edit"))
      {
         if(hasAdditionalArg)
         {
            hasEditor = true;
            gEditorUserInterface.setEditName(argv[i+1]);
         }
      }
   }
   gMasterAddress.set(gMasterAddressString);
#ifdef ZAP_DEDICATED
   hasClient = false;
   hasServer = true;
   connectRemote = false;
   connectLocal = false;
#endif

   if(hasClient)
      gClientGame = new ClientGame(Address());

   if(hasEditor)
      gEditorUserInterface.activate();
   else
   {
      if(hasServer)
         hostGame(hasClient == false, gBindAddress);
      else if(connectRemote)
         joinGame(gConnectAddress, false);

      if(!connectLocal && !connectRemote)
      {
         if(!nameSet)
            gNameEntryUserInterface.activate();
         else
            gMainMenuUserInterface.activate();
      }
   }
}

};

using namespace Zap;
#ifdef TNL_OS_XBOX
int zapmain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
#ifdef TNL_OS_MAC_OSX
  char path[1024];
  strcpy(path, argv[0]);
  char *pos = strrchr(path, '/');
  *pos = 0;
  logprintf("Path = %s", path);
  chdir(path);
#endif

   //TNLLogEnable(LogConnectionProtocol, true);
   //TNLLogEnable(LogNetConnection, true);
   TNLLogEnable(LogNetInterface, true);
   TNLLogEnable(LogPlatform, true);
   TNLLogEnable(LogNetBase, true);

   for(S32 i = 0; i < argc;i++)
      logprintf(argv[i]);

   Vector<StringPtr> theArgv;

   for(S32 i = 1; i < argc; i++)
   {
      if(!stricmp(argv[i], "-crazybot"))
      {
         srand(Platform::getRealMilliseconds());
         gIsCrazyBot = true;
      }
      else if(!stricmp(argv[i], "-jsave"))
      {
         if(i != argc - 1)
         {
            gZapJournal.record(argv[i+1]);
            i++;
         }
      }
      else if(!stricmp(argv[i], "-jplay"))
      {
         if(i != argc - 1)
         {
            gZapJournal.load(argv[i+1]);
            i++;
         }
      }
      else
         theArgv.push_back(argv[i]);
   }

   gZapJournal.startup(theArgv);

   // we need to process the startup code if this is playing back
   // a journal.
   gZapJournal.processNextJournalEntry();

   if(gClientGame)
   {
      SFXObject::init();
      InitJoystick();
      OptionsMenuUserInterface::joystickType = autodetectJoystickType();

      glutInitWindowSize(800, 600);
      glutInit(&argc, argv);
      glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
      glutCreateWindow(gWindowTitle);
      glutDisplayFunc(display);
      glutReshapeFunc(reshape);
      glutPassiveMotionFunc(passivemotion);
      glutMotionFunc(motion);
      glutKeyboardFunc(key);
      glutKeyboardUpFunc(keyup);
      glutSpecialFunc(specialkey);
      glutSpecialUpFunc(specialkeyup);
      glutMouseFunc(mouse);
      glutIdleFunc(idle);

      glutSetCursor(GLUT_CURSOR_NONE);
      glMatrixMode(GL_PROJECTION);
      glOrtho(0, 800, 600, 0, 0, 1);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslatef(400, 300, 0);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glLineWidth(DefaultLineWidth);

      atexit(onExit);
      if(OptionsMenuUserInterface::fullscreen)
         glutFullScreen();

      glutMainLoop();
   }
   else
      dedicatedServerLoop();
   return 0;
}
