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

#include <windows.h>
#include <stdio.h>

#define VERSION "0.5"

#define APPCOMMAND 0x0319

#define CMD_PLAYPAUSE 917504
#define CMD_MUTE 524288
#define CMD_VOLUMEDOWN 589824
#define CMD_VOLUMEUP 655360
#define CMD_STOP 851968
#define CMD_PREVIOUS 786432
#define CMD_NEXT 720896

int main(int argc, char **argv)
{
	HWND window_handle = FindWindow("SpotifyMainWindow", NULL);
	
	if (window_handle == NULL)
	{
		printf("Can not find spotify, is it running?\n");;
		return 1;
	}

	int cmd;
	if (argc > 1 && !strcmp(argv[1], "next"))
		cmd = CMD_NEXT;
	else if (argc > 1 && !strcmp(argv[1], "prev"))
		cmd = CMD_PREVIOUS;
	else if (argc > 1 && !strcmp(argv[1], "play"))
		cmd = CMD_PLAYPAUSE;
	else
	{
		printf("Usage: %s [prev|next|play]\n", argv[0]);
		return 2;
	}

	SendMessage(window_handle, APPCOMMAND, 0, cmd);
	return 0;
}

