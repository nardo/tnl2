// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#if 0

void output_debug_string();
void debug_break();
void exit(int code);
void alert_ok(const char *window_title, const char *message);
void sleep(uint32 millisecond_count);

#elif defined(PLATFORM_WIN32)

void output_debug_string(const char *string)
{
	OutputDebugStringA(string);
}

void debug_break()
{
	DebugBreak();
}

void exit(int code)
{
	ExitProcess(code);
}

void alert_ok(const char *window_title, const char *message)
{
	ShowCursor(true);
	MessageBoxA(NULL, message, window_title, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OK);
}

void sleep(uint32 millisecond_count)
{
	Sleep(millisecond_count);
}

#elif defined(PLATFORM_MAC_OSX)

#elif defined(PLATFORM_LINUX)

#endif