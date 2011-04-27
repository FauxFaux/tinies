/*
 *  Inspiration and definitions from:
 *  http://code.google.com/p/pytify/
 *  and
 *  http://code.google.com/p/spotifycmd/
 *
 *  This project has no affiliation to spotify.com whatsoever!
 *
 *  Intended use is to control Spotify running under Linux with Wine.
 */

#undef _UNICODE
#undef UNICODE

#ifdef _MSC_VER
#	define SC_WIN
#endif

#ifdef SC_WIN
#	pragma comment(lib, "user32.lib")
#endif

#include <windows.h>
#include <string.h>

#define VERSION "0.5"

#define APPCOMMAND 0x0319

#define CMD_PLAYPAUSE 917504
#define CMD_MUTE 524288
#define CMD_VOLUMEDOWN 589824
#define CMD_VOLUMEUP 655360
#define CMD_STOP 851968
#define CMD_PREVIOUS 786432
#define CMD_NEXT 720896

#ifdef SC_WIN
#	define ALERT(x) MessageBox(0, x, "spotify-ctl", 0);
#else
#	include <stdio.h>
#	define ALERT(x) printf(x "\n");
#endif

#ifdef SC_WIN
#define argc 2
int CALLBACK WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR param, int show)
#else
#define param argv[1]
int main(int argc, char **argv)
#endif

{
	int cmd;
	HWND window_handle = FindWindow("SpotifyMainWindow", NULL);
	
	if (window_handle == NULL)
	{
		ALERT("Can not find spotify, is it running?");;
		return 1;
	}

	if (argc > 1 && !strcmp(param, "next"))
		cmd = CMD_NEXT;
	else if (argc > 1 && !strcmp(param, "prev"))
		cmd = CMD_PREVIOUS;
	else if (argc > 1 && !strcmp(param, "play"))
		cmd = CMD_PLAYPAUSE;
	else
	{
		ALERT("Usage: [prev|next|play]");
		return 2;
	}

	SendMessage(window_handle, APPCOMMAND, 0, cmd);
	return 0;
}

