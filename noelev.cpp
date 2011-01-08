#include <windows.h>
#include <stdio.h>

int wmain(int argc, wchar_t *argv[])
{
	if (argc != 2)
	{
		printf("Usage: executable-name");
		return 6;
	}

	FARPROC addr = GetProcAddress(GetModuleHandle(L"kernel32"), "QueryDosDeviceA");
	if (!addr)
	{
		printf("Couldn't get QueryDosDeviceA address: %x", GetLastError());
		return 1;
	}

	unsigned char stuffs[2] = {};

	HANDLE us = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, GetCurrentProcessId() );
	if (!us)
	{
		printf("Couldn't open current process: %x", GetLastError());
		return 2;
	}

	char *target = (char*)(addr)+0x290;

	if (!ReadProcessMemory(us, target, stuffs, 2, NULL))
	{
		printf("Couldn't read us: %x", GetLastError());
		return 3;
	}

	if (stuffs[0] != 0x75 || stuffs[1] != 0x24)
	{
		printf("Check bytes didn't match: 75 24 != %x %x", stuffs[0], stuffs[1]);
		return 4;
	}

	char nops[] = { (char)0x90, (char)0x90 };

	if (!WriteProcessMemory(us, target, nops, 2, NULL))
	{
		printf("Couldn't patch: %x", GetLastError());
		return 5;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	if (!CreateProcess(NULL, argv[1], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		printf("Creation failed: %x", GetLastError());
		return 7;
	}
}
