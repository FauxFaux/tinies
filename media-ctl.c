#pragma comment(lib, "user32.lib")

#include <windows.h>
#include <string.h>

int CALLBACK wWinMain(HINSTANCE hi, HINSTANCE hp, LPWSTR param, int show)
{
	WORD cmd;
	INPUT i[2];

	if (!wcscmp(param, L"next"))
		cmd = VK_MEDIA_NEXT_TRACK;
	else if (!wcscmp(param, L"prev"))
		cmd = VK_MEDIA_PREV_TRACK;
	else if (!wcscmp(param, L"play"))
		cmd = VK_MEDIA_PLAY_PAUSE;
	else
	{
		MessageBox(0, L"Usage: [prev|next|play]", L"media-ctl", MB_OK);
		return 2;
	}

	i[0].type = i[1].type = INPUT_KEYBOARD;
	i[0].ki.wVk = i[1].ki.wVk = cmd;
	i[0].ki.wScan = i[1].ki.wScan = 0;

	i[0].ki.dwFlags = KEYEVENTF_KEYUP;
	i[1].ki.dwFlags = KEYEVENTF_KEYUP;

	i[0].ki.time = i[1].ki.time = 0;
	i[0].ki.dwExtraInfo = i[1].ki.dwExtraInfo = 0;

	if (2 != SendInput(2, i, sizeof(INPUT)))
		printf("BOOM");
	return 0;
}
