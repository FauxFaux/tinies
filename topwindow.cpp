#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")

int main() {
	WCHAR text[MAX_PATH];
	while (true) {
		HWND hwnd = GetForegroundWindow();
		if (!GetWindowText(hwnd, text, MAX_PATH))
			text[0] = 0;

		wprintf(L"hWnd = 0x%08x, %s\n", hwnd, text);
		Sleep(200);
	}
}
