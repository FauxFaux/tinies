// Rei Resurreccion, CodeProject
BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam) {
	HWND child = FindWindowEx(hwnd, NULL, L"SHELLDLL_DefView", NULL);
	if (NULL != child) {
		HWND desktop = FindWindowEx(child, NULL, L"SysListView32", NULL);
		if (NULL != desktop) {
			*(reinterpret_cast<HWND*>(lParam)) = desktop;
			return FALSE;
		}
	}
	return TRUE;
}

HWND getDesktopWindow() {
	HWND desktop = NULL;
	EnumWindows(enumWindowsProc, (LPARAM)&desktop);
	return desktop;
}
