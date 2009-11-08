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

#include "core/coreTypes.h"
#include "core/core.h"
#include "core/coreJournal.h"

#include <string.h>
#if defined (TORQUE_OS_XBOX)
#include <xtl.h>

#elif defined (TORQUE_OS_WIN32)
#include <windows.h>

#include <malloc.h>

#else

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#endif

#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include "core/coreLog.h"

namespace TNL {

#if defined (TORQUE_OS_XBOX)

void outputDebugString(const char *string)
{
   OutputDebugString(string);
}

void debugBreak()
{
   DebugBreak();
}

void forceQuit()
{
   logprintf("-Force Quit-");
   // Reboot!
   LD_LAUNCH_DASHBOARD LaunchData = { XLD_LAUNCH_DASHBOARD_MAIN_MENU };
   XLaunchNewImage( NULL, (LAUNCH_DATA*)&LaunchData );
}

U32 getRealMilliseconds()
{
   U32 tickCount;
   JOURNAL_READ_BLOCK ( getRealMilliseconds,
      JOURNAL_READ( (&tickCount) );
      return tickCount;
   )

   tickCount = GetTickCount();

   JOURNAL_WRITE_BLOCK ( getRealMilliseconds,
      JOURNAL_WRITE( (tickCount) );
   )
   return tickCount;
}


//--------------------------------------
void AlertOK(const char *windowTitle, const char *message)
{
//   ShowCursor(true);
//   MessageBox(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OK);
   TorqueLogMessageV(LogPlatform, ("AlertOK: %s - %s", message, windowTitle));
   return;
}

//--------------------------------------
bool AlertOKCancel(const char *windowTitle, const char *message)
{
//   ShowCursor(true);
//   return MessageBox(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OKCANCEL) == IDOK;
   TorqueLogMessageV(LogPlatform, ("AlertOKCancel: %s - %s", message, windowTitle));
   return false;
}

//--------------------------------------
bool AlertRetry(const char *windowTitle, const char *message)
{
//   ShowCursor(true);
//   return (MessageBox(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_RETRYCANCEL) == IDRETRY);
   TorqueLogMessageV(LogPlatform, ("AlertRetry: %s - %s", message, windowTitle));
   return false;
}


class WinTimer
{
   private:
      F64 mPeriod;
      bool mUsingPerfCounter;
   public:
      WinTimer()
      {
         S64 frequency;
         mUsingPerfCounter = QueryPerformanceFrequency((LARGE_INTEGER *) &frequency);
         mPeriod = 1000.0f / F64(frequency);
      }
      S64 getCurrentTime()
      {
         if(mUsingPerfCounter)
         {
            S64 value;
            QueryPerformanceCounter( (LARGE_INTEGER *) &value);
            return value;
         }
         else
         {
            return GetTickCount();
         }
      }
      F64 convertToMS(S64 delta)
      {
         if(mUsingPerfCounter)
            return mPeriod * F64(delta);
         else
            return F64(delta);
      }
};

static WinTimer gTimer;

S64 getHighPrecisionTimerValue()
{
   return gTimer.getCurrentTime();
}

F64 getHighPrecisionMilliseconds(S64 timerDelta)
{
   return gTimer.convertToMS(timerDelta);
}

void sleep(U32 msCount)
{
	// no need to sleep on the xbox...
}

#elif defined (TORQUE_OS_WIN32)

void outputDebugString(const char *string)
{
   OutputDebugString(string);
}

void debugBreak()
{
   DebugBreak();
}

void forceQuit()
{
   ExitProcess(1);
}

U32 getRealMilliseconds()
{
   U32 tickCount;
   JOURNAL_READ_BLOCK ( getRealMilliseconds,
      JOURNAL_READ( (&tickCount) );
      return tickCount;
   )

   tickCount = GetTickCount();

   JOURNAL_WRITE_BLOCK ( getRealMilliseconds,
      JOURNAL_WRITE( (tickCount) );
   )
   return tickCount;
}

class WinTimer
{
   private:
      F64 mPeriod;
      bool mUsingPerfCounter;
   public:
      WinTimer()
      {
         S64 frequency;
         mUsingPerfCounter = QueryPerformanceFrequency((LARGE_INTEGER *) &frequency);
         mPeriod = 1000.0f / F64(frequency);
      }
      S64 getCurrentTime()
      {
         if(mUsingPerfCounter)
         {
            S64 value;
            QueryPerformanceCounter( (LARGE_INTEGER *) &value);
            return value;
         }
         else
         {
            return GetTickCount();
         }
      }
      F64 convertToMS(S64 delta)
      {
         if(mUsingPerfCounter)
            return mPeriod * F64(delta);
         else
            return F64(delta);
      }
};

static WinTimer gTimer;

S64 getHighPrecisionTimerValue()
{
   S64 currentTime;
   JOURNAL_READ_BLOCK ( getHighPrecisionTimerValue,
      JOURNAL_READ( (&currentTime) );
      return currentTime;
   )

   currentTime = gTimer.getCurrentTime();

   JOURNAL_WRITE_BLOCK ( getHighPrecisionTimerValue,
      JOURNAL_WRITE( (currentTime) );
   )

   return currentTime;
}

F64 getHighPrecisionMilliseconds(S64 timerDelta)
{
   F64 timerValue;
   JOURNAL_READ_BLOCK ( getHighPrecisionMilliseconds,
      JOURNAL_READ( (&timerValue) );
      return timerValue;
   )

   timerValue = gTimer.convertToMS(timerDelta);

   JOURNAL_WRITE_BLOCK ( getHighPrecisionMilliseconds,
      JOURNAL_WRITE( (timerValue) );
   )

   return timerValue;
}

void sleep(U32 msCount)
{
   Sleep(msCount);
}

//--------------------------------------
void AlertOK(const char *windowTitle, const char *message)
{
   ShowCursor(true);
   MessageBox(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OK);
}

//--------------------------------------
bool AlertOKCancel(const char *windowTitle, const char *message)
{
   ShowCursor(true);
   return MessageBox(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OKCANCEL) == IDOK;
}

//--------------------------------------
bool AlertRetry(const char *windowTitle, const char *message)
{
   ShowCursor(true);
   return (MessageBox(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_RETRYCANCEL) == IDRETRY);
}

#else // osx and linux

void debugBreak()
{
   kill(getpid(), SIGTRAP);
}

void outputDebugString(const char *string)
{
   //printf("%s", string);
}

void forceQuit()
{
   debugBreak();
   exit(1);
}


U32 x86UNIXGetTickCount();
//--------------------------------------

U32 getRealMilliseconds()
{
   return x86UNIXGetTickCount();
}

static bool   sg_initialized = false;
static U32 sg_secsOffset  = 0;

U32 x86UNIXGetTickCount()
{
   // TODO: What happens when crossing a day boundary?
   //
   timeval t;

   if (sg_initialized == false) {
      sg_initialized = true;

      ::gettimeofday(&t, NULL);
      sg_secsOffset = t.tv_sec;
   }

   ::gettimeofday(&t, NULL);

   U32 secs  = t.tv_sec - sg_secsOffset;
   U32 uSecs = t.tv_usec;

   // Make granularity 1 ms
   return (secs * 1000) + (uSecs / 1000);
}

class UnixTimer
{
   public:
      UnixTimer()
      {
      }
      S64 getCurrentTime()
      {
         return x86UNIXGetTickCount();
      }
      F64 convertToMS(S64 delta)
      {
         return F64(delta);
      }
};

static UnixTimer gTimer;

S64 getHighPrecisionTimerValue()
{
   return gTimer.getCurrentTime();
}

F64 getHighPrecisionMilliseconds(S64 timerDelta)
{
   return gTimer.convertToMS(timerDelta);
}

void sleep(U32 msCount)
{
   usleep(msCount * 1000);
}

//--------------------------------------
void AlertOK(const char *windowTitle, const char *message)
{
//   ShowCursor(true);
//   MessageBox(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OK);
   TorqueLogMessageV(LogPlatform, ("AlertOK: %s - %s", message, windowTitle));
   return;
}

//--------------------------------------
bool AlertOKCancel(const char *windowTitle, const char *message)
{
//   ShowCursor(true);
//   return MessageBox(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OKCANCEL) == IDOK;
   TorqueLogMessageV(LogPlatform, ("AlertOKCancel: %s - %s", message, windowTitle));
   return false;
}

//--------------------------------------
bool AlertRetry(const char *windowTitle, const char *message)
{
//   ShowCursor(true);
//   return (MessageBox(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_RETRYCANCEL) == IDRETRY);
   TorqueLogMessageV(LogPlatform, ("AlertRetry: %s - %s", message, windowTitle));
   return false;
}

#endif

/*
char *strdup(const char *src)
{
   char *buffer = (char *) malloc(strlen(src) + 1);
   strcpy(buffer, src);
   return buffer;
}*/
bool atob(const char *str)
{
   return !stricmp(str, "true") || atof(str);
}

S32 dSprintf(char *buffer, U32 bufferSize, const char *format, ...)
{
   va_list args;
   va_start(args, format);
#ifdef TORQUE_COMPILER_VISUALC
   S32 len = _vsnprintf(buffer, bufferSize, format, args);
#else
   S32 len = vsnprintf(buffer, bufferSize, format, args);
#endif
   return (len);
}


S32 dVsprintf(char *buffer, U32 bufferSize, const char *format, void *arglist)
{
#ifdef TORQUE_COMPILER_VISUALC
   S32 len = _vsnprintf(buffer, bufferSize, format, (va_list) arglist);
#else
   S32 len = vsnprintf(buffer, bufferSize, format, (char *) arglist);
#endif
   return len;
}

};


#if defined (__GNUC__)

int stricmp(const char *str1, const char *str2)
{
   while(toupper(*str1) == toupper(*str2) && *str1)
   {
      str1++;
      str2++;
   }
   return (toupper(*str1) > toupper(*str2)) ? 1 : ((toupper(*str1) < toupper(*str2)) ? -1 : 0);
}

int strnicmp(const char *str1, const char *str2, unsigned int len)
{
   for(unsigned int i = 0; i < len; i++)
   {
      if(toupper(str1[i]) == toupper(str2[i]))
         continue;
      return (toupper(str1[i]) > toupper(str2[i])) ? 1 : ((toupper(str1[i]) < toupper(str2[i])) ? -1 : 0);
   }
   return 0;
}

#endif


