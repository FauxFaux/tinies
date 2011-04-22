#include <windows.h>
#include <stdio.h>

#define ERROH(msg) { \
	wprintf(L"Failed to " msg L" at line %d, " \
		msg L": 0x%x\n", __LINE__, GetLastError()); \
	return __LINE__; }

int stopDebugging(DWORD id) {
	if (!DebugActiveProcessStop(id))
		ERROH(L"stop debugging (oh shi-");
	return 0;
}
	
int CALLBACK wWinMain(
  __in  HINSTANCE hInstance,
  __in  HINSTANCE hPrevInstance,
  __in  LPWSTR lpCmdLine,
  __in  int nCmdShow
)
{
	const wchar_t path[] = L"libspotify.dll";
	const size_t path_len = wcslen(path);
	const size_t path_size = sizeof(wchar_t) * (path_len + 1);
	const HMODULE kernel32 = GetModuleHandle(L"kernel32");

	STARTUPINFO si = {0};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {0};
	wchar_t *p = _wcsdup(L"foobar2000.exe");
	if (!CreateProcess(NULL, p, NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS, 
			NULL, NULL, &si, &pi))
		ERROH(L"start foobar2000.exe");

	HANDLE thread = NULL;
	void *mem = NULL;
	while (true) {
		DEBUG_EVENT ev = {0};
		if (WaitForDebugEvent(&ev, INFINITE)) {
			switch (ev.dwDebugEventCode) {
				case CREATE_PROCESS_DEBUG_EVENT: {
					mem = VirtualAllocEx(pi.hProcess, NULL, path_size, MEM_COMMIT, PAGE_READWRITE);
					if (!mem)
						ERROH(L"allocate memory in foobar");
					if (!WriteProcessMemory(pi.hProcess, mem, (void*)path, path_size, NULL))
						ERROH(L"write memory in foobar");

					thread = CreateRemoteThread(pi.hProcess, NULL, 0, 
						(LPTHREAD_START_ROUTINE) GetProcAddress(kernel32, "LoadLibraryW"), 
						mem, 0, NULL);
					if (NULL == thread)
						ERROH(L"create thread in foobar");
#					if 0 // BROKENZ
						return stopDebugging(pi.dwProcessId);
#					endif				
				} break;
			}

			if (!ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE))
				ERROH(L"continue debug event");
		}

		if (NULL != thread)
			if (WAIT_OBJECT_0 == WaitForSingleObject(thread, 0)) {
				DWORD exit = -1;
				if (!GetExitCodeThread(thread, &exit))
					ERROH(L"get exit code from thread");
				
				if (exit) {
					wprintf(L"LoadLibrary failed in remote thread: 0x%x", exit);
					return __LINE__;
				}

				CloseHandle(thread);
				VirtualFreeEx(pi.hProcess, mem, path_size, MEM_RELEASE);

				return stopDebugging(pi.dwProcessId);
			}
	}
}
