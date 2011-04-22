#pragma comment(lib, "user32.lib")
#include <windows.h>
#include <cstdio>
#include <tchar.h>

int _tmain(int argc, _TCHAR* argv[])
{
	HWND hw;
	hw=FindWindow(NULL,L"Automatic Updates");
	if ((long)hw)
	{
		if (argc>1)
		{
			ShowWindow(hw,SW_SHOW);
			printf("Found and made visible.\n");
		}
		else
		{
			ShowWindow(hw,SW_HIDE);
			printf("Found and hidden.\n");
		}
	}
	else
		printf("Not found, nothing done.\n");
	return 0;
}

