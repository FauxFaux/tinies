/* Tray code modified from http://damb.dk */ 

#ifndef WINVER
#define WINVER 0x0600
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif						

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0600
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0700
#endif

#define NTDDI_VERSION NTDDI_VISTA
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1

#include <iostream>
#include <string>
#include <algorithm>
#include <windows.h>
#include <vector>
#include <set>
#include <utility>
#include <sstream>

const size_t MAX = 0x1000;

typedef std::set<std::wstring> wstrset_t;
typedef std::pair<wstrset_t, wstrset_t> doublepair_t;
doublepair_t getOnOff()
{
	doublepair_t pairs;
	WCHAR str[MAX];
	DWORD l = GetLogicalDriveStrings(MAX - 1, &str[0]);
	if (0 == l)
		return pairs;

	for (DWORD i = 0; i < l; ++i)
	{
		WCHAR buf[MAX];
		wcscpy(buf, L"\\\\.\\");
		const size_t len = wcslen(&str[i]);
		str[i+len-1] = 0;
		wcscat(buf, &str[i]);
		HANDLE h = CreateFile(buf, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		i += len;
		BOOL b;
		if (h != INVALID_HANDLE_VALUE && GetDevicePowerState(h, &b))
			if (b)
				pairs.first.insert(&str[i]);
			else
				pairs.second.insert(&str[i]);
		CloseHandle(h);
	}
	return pairs;
}


const UINT trayID = 123;
const UINT TRAY_MESSAGE = WM_APP + trayID;
const UINT IDM_TIMER = 1002;

const wchar_t className[] = L"PowerStatusClass";
const wchar_t trayTip[] = L"PowerStatus tray notification";
HICON HIcon;

doublepair_t previous = getOnOff();

void add(std::wstringstream &o, const wstrset_t &st)
{
	if (st.empty())
		o << L"None.";
	else
		for (wstrset_t::const_iterator it = st.begin(); it != st.end(); ++it)
			o << *it << " ";
}

std::wstring stringify()
{
	std::wstringstream wss;
	doublepair_t curr = getOnOff();

	wss << L"On: ";  add(wss, curr.first);  wss << "\n";
	wss << L"Off: "; add(wss, curr.second); wss << "\n";

	return wss.str();
}

void SetBaloonTip(HWND hwndDlg, const std::wstring &title, const std::wstring &body)
{
	NOTIFYICONDATA  NotifyIconData;
	memset(&NotifyIconData, 0, sizeof(NOTIFYICONDATA));
	NotifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	NotifyIconData.uID = trayID;
	NotifyIconData.hWnd = hwndDlg;
	NotifyIconData.hIcon = HIcon;
	NotifyIconData.uFlags = NIF_INFO | NIF_REALTIME;
	NotifyIconData.uTimeout = 10000;
	NotifyIconData.uCallbackMessage = TRAY_MESSAGE;
	wcscpy_s(NotifyIconData.szInfoTitle, title.c_str());
	wcscpy_s(NotifyIconData.szInfo, body.c_str());
	wcscpy_s(NotifyIconData.szTip, trayTip);
	Shell_NotifyIcon(NIM_MODIFY, (NOTIFYICONDATA *)&NotifyIconData);
}

wstrset_t newItems(const wstrset_t &old, const wstrset_t &now)
{
	wstrset_t ret;
	for (wstrset_t::const_iterator it = now.begin(); it != now.end(); ++it)
		if (old.find(*it) == old.end())
			ret.insert(*it);
	return ret;
}

void add(std::wstringstream &o, const bool singular, const wchar_t *msg)
{
	o << (singular ? L"is" : L"are") << L" now "<< msg << ".\n";
}

LRESULT CALLBACK DialogProc(HWND hwndDlg,
							UINT msg,
							WPARAM wParam,
							LPARAM lParam)
{
	switch(msg)
	{
	case TRAY_MESSAGE:
		if(lParam == WM_LBUTTONUP)
			SetBaloonTip(hwndDlg, L"Drive statuses:", stringify());
		else if (lParam == WM_RBUTTONUP)
			PostQuitMessage(0);
		break;
	case WM_TIMER:
		{
			const doublepair_t curr = getOnOff();
			if (curr != previous)
			{
				wstrset_t nowOn = newItems(previous.first, curr.first),
					nowOff = newItems(previous.second, curr.second);
				std::wstringstream wss;
				if (!nowOn.empty())
				{
					add(wss, nowOn);
					add(wss, nowOn.size() == 1, L"on");
				}
				if (!nowOff.empty())
				{
					add(wss, nowOff);
					add(wss, nowOff.size() == 1, L"off");
				}
				SetBaloonTip(hwndDlg, L"Drive status change:", wss.str());
				previous = curr;
			}
		}
		break;
	}
	return DefWindowProc(hwndDlg, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, INT)
{
	HMODULE shell32 = LoadLibraryEx(L"shell32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
	HIcon = LoadIcon(shell32, MAKEINTRESOURCE(8));
	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = DialogProc;
	wc.hInstance = hInstance;
	wc.hbrBackground = (HBRUSH )(COLOR_BTNFACE + 1);
	wc.lpszClassName = className;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	if(!RegisterClass(&wc))
		return FALSE;
	HWND WindowHandle = CreateWindowW(className, L"Powerstatus", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, hInstance, 0);
	{
		NOTIFYICONDATA NotifyIconData;
		memset(&NotifyIconData, 0, sizeof(NotifyIconData));
		NotifyIconData.cbSize = sizeof(NotifyIconData);
		NotifyIconData.uID = trayID;
		NotifyIconData.hWnd = WindowHandle;
		NotifyIconData.hIcon = HIcon;
		NotifyIconData.uFlags = NIF_ICON | NIF_MESSAGE;
		NotifyIconData.uCallbackMessage = WM_APP + 123;
		wcscpy_s(NotifyIconData.szTip, trayTip);
		Shell_NotifyIcon(NIM_ADD, &NotifyIconData);
	}

	SetTimer(WindowHandle, IDM_TIMER, 5000, 0);

	MSG Msg;

	while(GetMessage(&Msg, WindowHandle, 0, 0xFFFF) > 0)
	{
		if(!IsDialogMessage(WindowHandle, &Msg))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	return 0;
}
