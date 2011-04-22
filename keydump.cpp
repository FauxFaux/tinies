#pragma comment(lib, "user32.lib")
#pragma comment(lib, "dinput8.lib")

#include "defines.h"

#include <iostream>
#include <stdio.h>
#include <windows.h>

#include <InitGuid.h>
#define DIRECTINPUT_VERSION 0x800
#include <dinput.h>

void free_input(LPDIRECTINPUT8 lpdi)
{
	lpdi->Release();
}

void free_device(LPDIRECTINPUTDEVICE8 lpdide)
{
	lpdide->Unacquire();
	lpdide->Release();
}


struct cp
{
	char uns, sh;
	cp(char uns, char sh) : uns(uns), sh(sh) {}
	cp() : uns(0), sh(0) {}
	operator bool() { return uns != 0; }
};

cp key_mapping[256];

int main(int argc, wchar_t* argv[])
{
	try
	{
		HWND wnd;
		if (FAILED(wnd = CreateWindow(NULL, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL)))
			throw "Can't create message window";

		LPDIRECTINPUT8 di;
		if (FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&di, NULL)))
			throw "Can't create directinput";

		LPDIRECTINPUTDEVICE8 dev;

		if (FAILED(di->CreateDevice(GUID_SysKeyboard, &dev, NULL)))
			throw "Can't create device";

		if (FAILED(dev->SetDataFormat(&c_dfDIKeyboard)))
			throw "Can't set keyboard format";

		if (FAILED(dev->SetCooperativeLevel(wnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)))
			throw "Can't set cooperative level.";

		HANDLE ev = CreateEvent(NULL, FALSE, FALSE, NULL);

		const size_t buffer_size = 50;
		{
			DIPROPDWORD dipdw;
			dipdw.diph.dwSize = sizeof(DIPROPDWORD);
			dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
			dipdw.diph.dwObj = 0;
			dipdw.diph.dwHow = DIPH_DEVICE;
			dipdw.dwData = buffer_size;

			if (FAILED(dev->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)))
				throw "Couldn't set buffer size";
		}

		if (FAILED(dev->SetEventNotification(ev)))
			throw "Couldn't set event";

		if (FAILED(dev->Acquire()))
			throw "Failed to acquire";

		DIDEVICEOBJECTDATA buffer[buffer_size];
	
		while (WaitForSingleObject(ev, INFINITE) != WAIT_FAILED)
		{
			DWORD items = buffer_size;
			HRESULT hr;
			if (FAILED(hr = dev->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), buffer, &items, 0)))
				throw "Couldn't get data";
			for (size_t i = 0; i < items; ++i)
				if (buffer[i].dwData)
					std::cout << buffer[i].dwData << ", " << buffer[i].dwOfs << std::endl;
		}
		return 0;

	}
	catch (const char * c)
	{
		std::cout << "failure: " << c << std::endl;
	}
	catch (HRESULT hr)
	{
		std::cout << "failureno: " << hr << std::endl;
	}
	catch (...)
	{
		std::cout << "general failure" << std::endl;
	}

	return 7;
}

