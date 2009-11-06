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

#include "gameConnection.h"

#include "game.h"
#include "UIGame.h"
#include "UIMenus.h"
#include "UIInstructions.h"
#include "gameType.h"
#include "lpc10.h"
#include "tnlEndian.h"
#include "ship.h"

#include <stdarg.h>
#include "glutInclude.h"
#include <ctype.h>
#include <stdio.h>
#include "gameObjectRender.h"

namespace Zap
{

GameUserInterface gGameUserInterface;
Color gGlobalChatColor(0.9, 0.9, 0.9);
Color gTeamChatColor(0, 1, 0);

GameUserInterface::GameUserInterface()
{
   mCurrentMode = PlayMode;
   mInScoreboardMode = false;
#if 0 //defined(TNL_OS_XBOX)
   mFPSVisible = true;
#else
   mFPSVisible = false;
#endif
   mFrameIndex = 0;
   for(U32 i = 0; i < FPSAvgCount; i++)
      mIdleTimeDelta[i] = 50;

   mChatLastBlinkTime = 0;
   memset(mChatBuffer, 0, sizeof(mChatBuffer));
   mChatBlink = false;

   for(U32 i = 0; i < MessageDisplayCount; i++)
      mDisplayMessage[i][0] = 0;

   mGotControlUpdate = false;
}

void GameUserInterface::displayMessage(Color theColor, const char *format, ...)
{
   for(S32 i = MessageDisplayCount - 1; i > 0; i--)
   {
      strcpy(mDisplayMessage[i], mDisplayMessage[i-1]);
      mDisplayMessageColor[i] = mDisplayMessageColor[i-1];
   }
   va_list args;
   va_start(args, format);
   dVsprintf(mDisplayMessage[0], sizeof(mDisplayMessage[0]), format, args);
   mDisplayMessageColor[0] = theColor;

   mDisplayMessageTimer = DisplayMessageTimeout;
}

void GameUserInterface::idle(U32 timeDelta)
{
   if(timeDelta > mDisplayMessageTimer)
   {
      mDisplayMessageTimer = DisplayMessageTimeout;
      for(S32 i = MessageDisplayCount - 1; i > 0; i--)
      {
         strcpy(mDisplayMessage[i], mDisplayMessage[i-1]);
         mDisplayMessageColor[i] = mDisplayMessageColor[i-1];
      }

      mDisplayMessage[0][0] = 0;
   }
   else
      mDisplayMessageTimer -= timeDelta;
   if(mCurrentMode == ChatMode)
   {
      mChatLastBlinkTime += timeDelta;
      if(mChatLastBlinkTime > ChatBlinkTime)
      {
         mChatBlink = !mChatBlink;
         mChatLastBlinkTime = 0;
      }
   }
   else if(mCurrentMode == VChatMode)
      mVChat.idle(timeDelta);
   else if(mCurrentMode == LoadoutMode)
      mLoadout.idle(timeDelta);
   mVoiceRecorder.idle(timeDelta);
   mIdleTimeDelta[mFrameIndex % FPSAvgCount] = timeDelta;
   mFrameIndex++;
}

#ifdef TNL_OS_WIN32
extern void checkMousePos(S32 maxdx, S32 maxdy);
#endif

void GameUserInterface::render()
{
   glColor3f(0.0, 0.0, 0.0);
   if(!gClientGame->isConnectedToServer())
   {
      glColor3f(1,1,1);
      drawCenteredString(290, 30, "Connecting to server...");

      if(gClientGame->getConnectionToServer())
         drawCenteredString(330, 16, gConnectStatesTable[gClientGame->getConnectionToServer()->getConnectionState()]);
      drawCenteredString(370, 20, "Press <ESC> to abort");
   }

   gClientGame->render();

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   renderReticle();

   renderMessageDisplay();
   renderCurrentChat();

   mVoiceRecorder.render();
   if(mFPSVisible)
   {
      U32 sum = 0;
      for(U32 i = 0; i < FPSAvgCount; i++)
         sum += mIdleTimeDelta[i];
      drawStringf(canvasWidth - horizMargin - 170, vertMargin, 30, 
            "%0.2g %0.2g %4.2f fps", mCurrentMove.right - mCurrentMove.left,
                               mCurrentMove.down - mCurrentMove.up, 
                               (1000 * FPSAvgCount) / F32(sum));
   }
   if(mCurrentMode == VChatMode)
      mVChat.render();
   else if(mCurrentMode == LoadoutMode)
      mLoadout.render();

   GameType *theGameType = gClientGame->getGameType();

   if(theGameType)
      theGameType->renderInterfaceOverlay(mInScoreboardMode);

#if 0
   // some code for outputting the position of the ship for finding good spawns
   GameConnection *con = gClientGame->getConnectionToServer();

   if(con)
   {
      GameObject *co = con->getControlObject();
      if(co)
      {
         Point pos = co->getActualPos() * F32(1 / 300.0f);
         drawStringf(10, 550, 30, "%0.2g, %0.2g", pos.x, pos.y);
      }
   }

   if(mGotControlUpdate)
   {
      drawString(710, 10, 30, "CU");
   }
#endif
}

void GameUserInterface::renderReticle()
{
   // draw the reticle

   if(OptionsMenuUserInterface::joystickType == -1)
   {
#if 0 // TNL_OS_WIN32
      Point realMousePoint = mMousePoint;
      if(!OptionsMenuUserInterface::controlsRelative)
      {
         F32 len = mMousePoint.len();
         checkMousePos(windowWidth * 100 / canvasWidth,
                     windowHeight * 100 / canvasHeight);

         if(len > 100)
            realMousePoint *= 100 / len;
      }
#endif
      Point offsetMouse = mMousePoint + Point(canvasWidth / 2, canvasHeight / 2);

      glEnable(GL_BLEND);
      glColor4f(0,1,0, 0.7);
      glBegin(GL_LINES);

      glVertex2f(offsetMouse.x - 15, offsetMouse.y);
      glVertex2f(offsetMouse.x + 15, offsetMouse.y);
      glVertex2f(offsetMouse.x, offsetMouse.y - 15);
      glVertex2f(offsetMouse.x, offsetMouse.y + 15);

      if(offsetMouse.x > 30)
      {
         glColor4f(0,1,0, 0);
         glVertex2f(0, offsetMouse.y);
         glColor4f(0,1,0, 0.7);
         glVertex2f(offsetMouse.x - 30, offsetMouse.y);
      }
      if(offsetMouse.x < canvasWidth - 30)
      {
         glColor4f(0,1,0, 0.7);
         glVertex2f(offsetMouse.x + 30, offsetMouse.y);
         glColor4f(0,1,0, 0);
         glVertex2f(canvasWidth, offsetMouse.y);
      }
      if(offsetMouse.y > 30)
      {
         glColor4f(0,1,0, 0);
         glVertex2f(offsetMouse.x, 0);
         glColor4f(0,1,0, 0.7);
         glVertex2f(offsetMouse.x, offsetMouse.y - 30);
      }
      if(offsetMouse.y < canvasHeight - 30)
      {
         glColor4f(0,1,0, 0.7);
         glVertex2f(offsetMouse.x, offsetMouse.y + 30);
         glColor4f(0,1,0, 0);
         glVertex2f(offsetMouse.x, canvasHeight);
      }

      glEnd();
      glDisable(GL_BLEND);
   }
}

void GameUserInterface::renderMessageDisplay()
{
   glColor3f(1,1,1);

   U32 y = UserInterface::vertMargin;
   for(S32 i = 3; i >= 0; i--)
   {
      if(mDisplayMessage[i][0])
      {
         glColor(mDisplayMessageColor[i]);

         drawString(UserInterface::horizMargin, y, 20, mDisplayMessage[i]);
         y += 24;
      }
   }
}

void GameUserInterface::renderCurrentChat()
{   
   if(mCurrentMode == ChatMode)
   {
      const char *promptStr;
      if(mCurrentChatType == TeamChat)
      {
         glColor3f(gTeamChatColor.r,
                   gTeamChatColor.g,
                   gTeamChatColor.b);

         promptStr = "(Team): ";
      }
      else
      {
         glColor3f(gGlobalChatColor.r,
                   gGlobalChatColor.g,
                   gGlobalChatColor.b);

         promptStr = "(Global): ";
      }

      U32 width = getStringWidth(20, promptStr);

      drawString(UserInterface::horizMargin, UserInterface::vertMargin + 95, 20, promptStr);
      drawString(UserInterface::horizMargin + width, UserInterface::vertMargin + 95, 20, mChatBuffer);

      if(mChatBlink)
         drawString(UserInterface::horizMargin + width + getStringWidth(20, mChatBuffer, mChatCursorPos), UserInterface::vertMargin + 95, 20, "_");
   }
}

void GameUserInterface::onMouseDragged(S32 x, S32 y)
{
   onMouseMoved(x, y);
}

void GameUserInterface::onMouseMoved(S32 x, S32 y)
{
   S32 xp = S32(x - windowWidth / 2);
   S32 yp = S32(y - windowHeight / 2);
   
   mMousePoint = Point(x - windowWidth / 2, y - windowHeight / 2);
   mMousePoint.x = mMousePoint.x * canvasWidth / windowWidth;
   mMousePoint.y = mMousePoint.y * canvasHeight / windowHeight;
   mCurrentMove.angle = atan2(mMousePoint.y, mMousePoint.x);
}

void GameUserInterface::onMouseDown(S32 x, S32 y)
{
   mCurrentMove.fire = true;
}

void GameUserInterface::onMouseUp(S32 x, S32 y)
{
   mCurrentMove.fire = false;
}

void GameUserInterface::onRightMouseDown(S32 x, S32 y)
{
   mCurrentMove.module[1] = true;
}

void GameUserInterface::onRightMouseUp(S32 x, S32 y)
{
   mCurrentMove.module[1] = false;
}

void GameUserInterface::enterVChat(bool fromController)
{
   UserInterface::playBoop();
   mVChat.show(fromController);
   mCurrentMode = VChatMode;
}

void GameUserInterface::enterLoadout(bool fromController)
{
   UserInterface::playBoop();
   mLoadout.show(fromController);
   mCurrentMode = LoadoutMode;
}

void GameUserInterface::onControllerButtonDown(U32 buttonIndex)
{
   if(buttonIndex == 6)
      mCurrentMove.module[1] = true;
   else if(buttonIndex == 7)
      mCurrentMove.module[0] = true;
   else
   {
      if(mCurrentMode == PlayMode)
      {
         switch(buttonIndex)
         {
            case 0:
            {
               GameType *g = gClientGame->getGameType();
               if(g)
                  g->c2sAdvanceWeapon();
               break;
            }
            case 1:
               gClientGame->zoomCommanderMap();
               break;
            case 2:
               enterVChat(true);
               break;
            case 3:
               enterLoadout(true);
               break;
            case 4:
            {
               mInScoreboardMode = true;
               GameType *g = gClientGame->getGameType();
               if(g)
                  g->c2sRequestScoreboardUpdates(true);
               break;
            }
            case 5:
               mVoiceRecorder.start();
               break;
            default:
               logprintf("button down: %d", buttonIndex);
               break;
         }
      }
      else if(mCurrentMode == VChatMode)
      {
         mVChat.processKey(buttonIndex);
      }
      else if(mCurrentMode == LoadoutMode)
      {
         mLoadout.processKey(buttonIndex);
      }
   }
}

void GameUserInterface::onControllerButtonUp(U32 buttonIndex)
{
   if(buttonIndex == 6)
      mCurrentMove.module[1] = false;
   else if(buttonIndex == 7)
      mCurrentMove.module[0] = false;
   else
   {
      if(mCurrentMode == PlayMode)
      {
         switch(buttonIndex)
         {
            case 4:
            {
               mInScoreboardMode = false;
               GameType *g = gClientGame->getGameType();
               if(g)
                  g->c2sRequestScoreboardUpdates(false);
               break;
            }
            case 5:
               mVoiceRecorder.stop();
               break;
         }
      }
   }
}

void GameUserInterface::onModifierKeyDown(U32 key)
{
   if(mCurrentMode == LoadoutMode || 
      mCurrentMode == PlayMode)
   {
      if(key == 0)
      {
         mCurrentMove.module[1] = true;
      }
   }
}

void GameUserInterface::onModifierKeyUp(U32 key)
{
   if(mCurrentMode == LoadoutMode || 
      mCurrentMode == PlayMode)
   {
      if(key == 0)
      {
         mCurrentMove.module[1] = false;
      }
   }
}

void GameUserInterface::onSpecialKeyDown(U32 key)
{
   if(key == GLUT_KEY_F1)
      gInstructionsUserInterface.activate();
}

void GameUserInterface::onKeyDown(U32 key)
{
   if(mCurrentMode == LoadoutMode)
      if(mLoadout.processKey(key))
         return;
   if(mCurrentMode == LoadoutMode || 
      mCurrentMode == PlayMode)
   {
      mCurrentChatType = GlobalChat;

      // the following keys are allowed in both play mode
      // and in loadout or engineering menu modes if not used in the menu
      // menu
      switch(toupper(key))
      {
         case '\t':
         {
            mInScoreboardMode = true;
            GameType *g = gClientGame->getGameType();
            if(g)
               g->c2sRequestScoreboardUpdates(true);
            break;
         }
         case 'P':
            mFPSVisible = !mFPSVisible;
            break;
         case 'W':
            mCurrentMove.up = 1.0;
            break;
         case 'A':
            mCurrentMove.left = 1.0;
            break;
         case 'S':
            mCurrentMove.down = 1.0;
            break;
         case 'D':
            mCurrentMove.right = 1.0;
            break;
         case ' ':
            mCurrentMove.module[0] = true;
            break;
         case 'E':
            {
               GameType *g = gClientGame->getGameType();
               if(g)
                  g->c2sAdvanceWeapon();
               break;
            }
         case 27:
            if(!gClientGame->isConnectedToServer())
            {
               endGame();
               gMainMenuUserInterface.activate();
            }
            else
               gGameMenuUserInterface.activate();
            break;
         case 'C':
            gClientGame->zoomCommanderMap();
            break;
         case 'R':
            mVoiceRecorder.start();
            break;
         default:
         {
            if(mCurrentMode == LoadoutMode)
               break;
            // the following keys are only allowed in play mode
            switch(toupper(key))
            {
               case 'T':
                  mCurrentChatType = TeamChat;
               case 'G':
                  
                  mChatLastBlinkTime = 0;
                  mChatBlink = true;
                  mCurrentMode = ChatMode;
                  mCurrentMove.up = 
                     mCurrentMove.left = 
                     mCurrentMove.right = 
                     mCurrentMove.down = 0;
                  break;
               case 'V':
                  enterVChat(false);
                  break;
               case 'Q':
                  enterLoadout(false);
                  break;

            }
         }
      }
   }
   else if(mCurrentMode == ChatMode)
   {
      if(key == '\r')
         issueChat();
      else if(key == 8 || key == 127)
      {
         // backspace key
         if(mChatCursorPos > 0)
         {
            mChatCursorPos--;
            for(U32 i = mChatCursorPos; mChatBuffer[i]; i++)
               mChatBuffer[i] = mChatBuffer[i+1];
         }
      }
      else if(key == 27)
      {
         cancelChat();
      }
      else
      {
         for(U32 i = sizeof(mChatBuffer) - 2; i > mChatCursorPos; i--)
            mChatBuffer[i] = mChatBuffer[i-1];
         if(mChatCursorPos < sizeof(mChatBuffer) - 2)
         {
            mChatBuffer[mChatCursorPos] = key;
            mChatCursorPos++;
         }
      }
   }
   else if(mCurrentMode == VChatMode)
      mVChat.processKey(key);
}

void GameUserInterface::onKeyUp(U32 key)
{
   if(mCurrentMode == LoadoutMode || 
      mCurrentMode == PlayMode)
   {
      switch(toupper(key))
      {
         case '\t':
         {
            mInScoreboardMode = false;
            GameType *g = gClientGame->getGameType();
            if(g)
               g->c2sRequestScoreboardUpdates(false);
            break;
         }
         case 'W':
            mCurrentMove.up = 0;
            break;
         case 'A':
            mCurrentMove.left = 0;
            break;
         case 'S':
            mCurrentMove.down = 0;
            break;
         case 'D':
            mCurrentMove.right = 0;
            break;

         case ' ':
            mCurrentMove.module[0] = false;
            break;
         case 'R':
            mVoiceRecorder.stop();
            break;
      }
   }
   else if(mCurrentMode == ChatMode)
   {


   }
}

Move *GameUserInterface::getCurrentMove()
{
   if(!OptionsMenuUserInterface::controlsRelative)
      return &mCurrentMove;
   else
   {
      mTransformedMove = mCurrentMove;

      Point moveDir(mCurrentMove.right - mCurrentMove.left,
                    mCurrentMove.up - mCurrentMove.down);

      Point angleDir(cos(mCurrentMove.angle), sin(mCurrentMove.angle));

      Point rightAngleDir(-angleDir.y, angleDir.x);
      Point newMoveDir = angleDir * moveDir.y + rightAngleDir * moveDir.x;

      if(newMoveDir.x > 0)
      {
         mTransformedMove.right = newMoveDir.x;
         mTransformedMove.left = 0;
      }
      else
      {
         mTransformedMove.right = 0;
         mTransformedMove.left = -newMoveDir.x;
      }
      if(newMoveDir.y > 0)
      {
         mTransformedMove.down = newMoveDir.y;
         mTransformedMove.up = 0;
      }
      else
      {
         mTransformedMove.down = 0;
         mTransformedMove.up = -newMoveDir.y;
      }
      if(mTransformedMove.right > 1)
         mTransformedMove.right = 1;
      if(mTransformedMove.left > 1)
         mTransformedMove.left = 1;
      if(mTransformedMove.up > 1)
         mTransformedMove.up = 1;
      if(mTransformedMove.down > 1)
         mTransformedMove.down = 1;

      return &mTransformedMove;
   }
}

void GameUserInterface::cancelChat()
{
   memset(mChatBuffer, 0, sizeof(mChatBuffer));
   mChatCursorPos = 0;
   mCurrentMode = PlayMode;
}

void GameUserInterface::issueChat()
{
   if(mChatBuffer[0])
   {
      GameType *gt = gClientGame->getGameType();
      if(gt)
         gt->c2sSendChat(mCurrentChatType == GlobalChat, mChatBuffer);
   }
   cancelChat();
}

GameUserInterface::VoiceRecorder::VoiceRecorder()
{
   mRecordingAudio = false;
   mMaxAudioSample = 0;
   mMaxForGain = 0;
   mVoiceEncoder = new LPC10VoiceEncoder;
}

void GameUserInterface::VoiceRecorder::idle(U32 timeDelta)
{
   if(mRecordingAudio)
   {
      if(mVoiceAudioTimer.update(timeDelta))
      {
         mVoiceAudioTimer.reset(VoiceAudioSampleTime);
         process();
      }
   }
}

void GameUserInterface::VoiceRecorder::render()
{
   if(mRecordingAudio)
   {
      F32 amt = mMaxAudioSample / F32(0x7FFF);
      U32 totalLineCount = 50;

      glColor3f(1, 1 ,1);
      glBegin(GL_LINES);
      glVertex2f(10, 130);
      glVertex2f(10, 145);
      glVertex2f(10 + totalLineCount * 2, 130);
      glVertex2f(10 + totalLineCount * 2, 145);

      F32 halfway = totalLineCount * 0.5;
      F32 full = amt * totalLineCount;
      for(U32 i = 1; i < full; i++)
      {
         if(i < halfway)
            glColor3f(i / halfway, 1, 0);
         else
            glColor3f(1, 1 - (i - halfway) / halfway, 0);
         
         glVertex2f(10 + i * 2, 130);
         glVertex2f(10 + i * 2, 145);
      }
      glEnd();
   }
}

void GameUserInterface::VoiceRecorder::start()
{
   if(!mRecordingAudio)
   {
      mRecordingAudio = SFXObject::startRecording();
      if(!mRecordingAudio)
         return;

      mUnusedAudio = new ByteBuffer(0);
      mRecordingAudio = true;
      mMaxAudioSample = 0;
      mVoiceAudioTimer.reset(FirstVoiceAudioSampleTime);

      // trim the start of the capture buffer:
      SFXObject::captureSamples(mUnusedAudio);
      mUnusedAudio->resize(0);
   }
}

void GameUserInterface::VoiceRecorder::stop()
{
   if(mRecordingAudio)
   {
      process();

      mRecordingAudio = false;
      SFXObject::stopRecording();
      mVoiceSfx = NULL;
      mUnusedAudio = NULL;
   }
}

void GameUserInterface::VoiceRecorder::process()
{
   U32 preSampleCount = mUnusedAudio->getBufferSize() / 2;
   SFXObject::captureSamples(mUnusedAudio);

   U32 sampleCount = mUnusedAudio->getBufferSize() / 2;
   if(sampleCount == preSampleCount)
      return;

   S16 *samplePtr = (S16 *) mUnusedAudio->getBuffer();
   mMaxAudioSample = 0;

   for(U32 i = preSampleCount; i < sampleCount; i++)
   {
      if(samplePtr[i] > mMaxAudioSample)
         mMaxAudioSample = samplePtr[i];
      else if(-samplePtr[i] > mMaxAudioSample)
         mMaxAudioSample = -samplePtr[i];
   }
   mMaxForGain = U32(mMaxForGain * 0.95f);
   S32 boostedMax = mMaxAudioSample + 2048;
   if(boostedMax > mMaxForGain)
      mMaxForGain = boostedMax;
   if(mMaxForGain > MaxDetectionThreshold)
   {
      // apply some gain to the buffer:
      F32 gain = 0x7FFF / F32(mMaxForGain);
      for(U32 i = preSampleCount; i < sampleCount; i++)
      {
         F32 sample = gain * samplePtr[i];
         if(sample > 0x7FFF)
            samplePtr[i] = 0x7FFF;
         else if(sample < -0x7FFF)
            samplePtr[i] = -0x7FFF;
         else
            samplePtr[i] = S16(sample);
      }
      mMaxAudioSample = U32(mMaxAudioSample * gain);
   }

   ByteBufferPtr sendBuffer = mVoiceEncoder->compressBuffer(mUnusedAudio);

   if(sendBuffer.isValid())
   {
      GameType *gt = gClientGame->getGameType();
      if(gt)
         gt->c2sVoiceChat(OptionsMenuUserInterface::echoVoice, sendBuffer);
   }
}

};
