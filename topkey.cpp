#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND msgwnd = CreateWindow(L"STATIC", L"Topkey window", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, GetModuleHandle(NULL), 0);

	if ((msgwnd == NULL) || (RegisterHotKey(msgwnd, 0, MOD_WIN, 'W') == 0)
						 || (RegisterHotKey(msgwnd, 1, MOD_CONTROL | MOD_WIN, 'W')) == 0)
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
			if (LOWORD(msg.lParam) == MOD_WIN)
			{
				HWND fg = GetForegroundWindow();
				HWND insertAfter;
				if (GetWindowLong(fg, GWL_EXSTYLE) & WS_EX_TOPMOST)
					insertAfter = HWND_NOTOPMOST;
				else
					insertAfter = HWND_TOPMOST;
				SetWindowPos(fg, insertAfter, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
			}
			else
				PostQuitMessage(0);

			MessageBeep(MB_OK);
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}
