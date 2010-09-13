// cl /MT shiftfocus.cpp user32.lib

#define UNICODE

#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND msgwnd = CreateWindow(L"STATIC", L"Shiftfocus window", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, GetModuleHandle(NULL), 0);

#	define MAKE(x) (RegisterHotKey(msgwnd, x, MOD_CONTROL | MOD_WIN, x) == 0)
	if ((msgwnd == NULL) || MAKE(VK_LEFT)
						 || MAKE(VK_RIGHT)
						 || MAKE(VK_UP)
						 || MAKE(VK_DOWN))
	{
		MessageBox(msgwnd, L"Unable to register hotkeys, exiting.", L"Error", MB_ICONERROR | MB_OK);
		DestroyWindow(msgwnd);
		return 1;
	}

	MSG msg;
	while(GetMessage(&msg, msgwnd, 0, 0))
	{
		if (msg.message == WM_HOTKEY)
		{
			RECT loc = {};
			HWND fg = GetForegroundWindow();
			HWND desktop = FindWindow(L"Progman", NULL);
			if (NULL != desktop)
				desktop = FindWindowEx(desktop, NULL, L"SHELLDLL_DefView", NULL);
			if (NULL != desktop)
				desktop = FindWindowEx(desktop, NULL, L"SysListView32", NULL);
			GetWindowRect(fg, &loc);
		
			POINT p = {};
			for (
				int distance = 10;
				MonitorFromPoint(p, MONITOR_DEFAULTTONULL);
				distance += 10) {
				
				switch (msg.wParam)
				{
#					define HALF(x,y) (((y)-(x))/2+(x))
					case VK_LEFT:
						p.x=loc.left-distance;
						p.y=HALF(loc.top,loc.bottom);
						break;
					case VK_RIGHT:
						p.x=loc.right+distance;
						p.y=HALF(loc.top,loc.bottom);
						break;
					case VK_UP:
						p.y=loc.top-distance;
						p.x=HALF(loc.left,loc.right);
						break;
					case VK_DOWN:
						p.y=loc.bottom+distance;
						p.x=HALF(loc.left,loc.right);
						break;
					default:
						MessageBeep(MB_ICONERROR);
				}

				HWND n = WindowFromPoint(p);
				if (NULL == n)
					MessageBeep(MB_ICONASTERISK);
				else
				{
					if (desktop == n)
						continue;

					if (!SetForegroundWindow(n))
						MessageBeep(MB_ICONERROR);
					break;
				}
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}
