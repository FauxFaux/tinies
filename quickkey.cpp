#include <windows.h>
#include <map>

using std::map;

map<int, HWND> keymap;

int inline mberr(HWND h, const wchar_t message[])
{
	MessageBox(h, message, L"Error", MB_ICONERROR | MB_OK);
	return 1;
}

int exiterror(const HWND& msgwnd)
{
	mberr(msgwnd, L"Unable to register hotkeys, exiting.");
	DestroyWindow(msgwnd);
	return 1;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND msgwnd = CreateWindow(L"STATIC", L"Quickkey window", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, GetModuleHandle(NULL), 0);

	if (msgwnd == NULL)
		return mberr(NULL, L"Can't create window");

	bool state = false;
	int counter = 0;
	for (char i = '0'; i<='9'; ++i)
		if ((RegisterHotKey(msgwnd, ++counter, MOD_WIN, i) == 0) || (RegisterHotKey(msgwnd, ++counter, MOD_CONTROL | MOD_WIN, i) == 0))
			return exiterror(msgwnd);

	MSG msg;
	while(GetMessage(&msg, msgwnd, 0, 0))
	{
		if (msg.message == WM_HOTKEY)
		{
			if (LOWORD(msg.lParam) != MOD_WIN)
				keymap[msg.wParam-1] = GetForegroundWindow();
			else
				SetForegroundWindow(keymap[msg.wParam]);

			MessageBeep(MB_OK);
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}
