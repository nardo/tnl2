// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#if defined(_WIN32)
#include <windows.h>


#include <gl/gl.h>
#elif defined (__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "testGame.h"
#include <math.h>

namespace TorqueTest {

void TestGame::renderFrame(int width, int height)
{
   glClearColor(1, 1, 0, 0);
   glClear(GL_COLOR_BUFFER_BIT);

   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, 1, 1, 0, 0, 1);
   glMatrixMode(GL_MODELVIEW);

   // first, render the alpha blended circle around the player,
   // to show the scoping range.

   if(clientPlayer)
   {
      Position p = clientPlayer->renderPos;
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glBegin(GL_POLYGON);
      glColor4f(0.5f, 0.5f, 0.5f, 0.65f);
      for(Torque::F32 r = 0; r < 3.1415 * 2; r += 0.1f)
      {
         glVertex2f(p.x + 0.25f * cos(r), p.y + 0.25f * sin(r));
      }

      glEnd();
      glDisable(GL_BLEND);
   }

   // then draw all the buildings.
   for(Torque::S32 i = 0; i < buildings.size(); i++)
   {
      Building *b = buildings[i];
      glBegin(GL_POLYGON);
      glColor3f(1, 0, 0);
      glVertex2f(b->upperLeft.x, b->upperLeft.y);
      glVertex2f(b->lowerRight.x, b->upperLeft.y);
      glVertex2f(b->lowerRight.x, b->lowerRight.y);
      glVertex2f(b->upperLeft.x, b->lowerRight.y);
      glEnd();
   }

   // last, draw all the players in the game.
   for(Torque::S32 i = 0; i < players.size(); i++)
   {
      Player *p = players[i];
      glBegin(GL_POLYGON);
      glColor3f(0,0,0);

      glVertex2f(p->renderPos.x - 0.012f, p->renderPos.y - 0.012f);
      glVertex2f(p->renderPos.x + 0.012f, p->renderPos.y - 0.012f);
      glVertex2f(p->renderPos.x + 0.012f, p->renderPos.y + 0.012f);
      glVertex2f(p->renderPos.x - 0.012f, p->renderPos.y + 0.012f);
      glEnd();

      glBegin(GL_POLYGON);
      switch(p->myPlayerType)
      {
         case Player::PlayerTypeAI:
         case Player::PlayerTypeAIDummy:
            glColor3f(0, 0, 1);
            break;
         case Player::PlayerTypeClient:
            glColor3f(0.5, 0.5, 1);
            break;
         case Player::PlayerTypeMyClient:
            glColor3f(1, 1, 1);
            break;
      }
      glVertex2f(p->renderPos.x - 0.01f, p->renderPos.y - 0.01f);
      glVertex2f(p->renderPos.x + 0.01f, p->renderPos.y - 0.01f);
      glVertex2f(p->renderPos.x + 0.01f, p->renderPos.y + 0.01f);
      glVertex2f(p->renderPos.x - 0.01f, p->renderPos.y + 0.01f);

      glEnd();
   }
}

};
