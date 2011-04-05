#include "defines.h"
#include <windows.h>
#include <stdio.h>

int wmain(int argc, wchar_t *argv[])
{
	if (argc < 2)
	{
		printf("Usage: dll [dll...]");
		return 6;
	}

	for (int i = 1; i < argc; ++i) {
		HANDLE res;
		wprintf(L"%2i. Loading %s... ", i, argv[i]);
		res = LoadLibrary(argv[i]);
		if (NULL != res)
			wprintf(L"done.  ");
		else {
			LPVOID lpMsgBuf;
			DWORD err = GetLastError();
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				err,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf,
				0, NULL );
			wprintf(L"Error:\n     %d (0x%x): %s     ", err, err, lpMsgBuf);
			LocalFree(lpMsgBuf);
		}
		wprintf(L"Waiting a bit... ");
		Sleep(200);
		wprintf(L"done.\n");
	}
}
