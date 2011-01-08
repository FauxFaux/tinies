#include <windows.h>
#include <xinput.h>
#include <map>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND msgwnd = CreateWindow(L"STATIC", L"MouseXinput Window", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, GetModuleHandle(NULL), 0);

	if (msgwnd == NULL)
	{
		MessageBeep(MB_ICONERROR);
		DestroyWindow(msgwnd);
		return 1;
	}

	SetTimer(msgwnd, 1, 10, NULL);

	MSG msg;
	XINPUT_STATE xis = {0};

	enum Butts
	{
		bLeft, bRight, bMiddle,
		//bX1, bX2
		BUTT_COUNT
	};

	typedef std::map<Butts, bool> buttons;

	buttons last_frame;

	std::map<Butts, DWORD> onDown;
	onDown[bLeft] =  MOUSEEVENTF_LEFTDOWN;
	onDown[bRight] = MOUSEEVENTF_RIGHTDOWN;
	onDown[bMiddle] = MOUSEEVENTF_MIDDLEDOWN;

	std::map<Butts, DWORD> onUp;
	onUp[bLeft] =  MOUSEEVENTF_LEFTUP;
	onUp[bRight] = MOUSEEVENTF_RIGHTUP;
	onUp[bMiddle] = MOUSEEVENTF_MIDDLEUP;


	while(GetMessage(&msg, msgwnd, 0, 0))
	{
		if (msg.message == WM_TIMER)
		{
			XInputGetState(0, &xis);

			if( (xis.Gamepad.sThumbLX < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && 
				xis.Gamepad.sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) && 
				(xis.Gamepad.sThumbLY < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && 
				xis.Gamepad.sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) )
				xis.Gamepad.sThumbLX = xis.Gamepad.sThumbLY = 0;

			if( (xis.Gamepad.sThumbRX < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && 
				xis.Gamepad.sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) && 
				(xis.Gamepad.sThumbRY < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && 
				xis.Gamepad.sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) ) 
				xis.Gamepad.sThumbRX = xis.Gamepad.sThumbRY = 0;

			if (xis.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
				PostQuitMessage(0);

			INPUT inp = {0};
			inp.type = INPUT_MOUSE;

			inp.mi.dx = static_cast<int>(xis.Gamepad.sThumbLX / 32768.0 * 8.0 + xis.Gamepad.sThumbRX / 32768.0 * 32.0);
			inp.mi.dy = -static_cast<int>(xis.Gamepad.sThumbLY / 32768.0 * 8.0 + xis.Gamepad.sThumbRY / 32768.0 * 32.0);

			DWORD flags = MOUSEEVENTF_MOVE;

			buttons current;
			current[bLeft] = xis.Gamepad.wButtons & XINPUT_GAMEPAD_A || xis.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
			current[bRight] = xis.Gamepad.wButtons & XINPUT_GAMEPAD_B || xis.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
			current[bMiddle] = xis.Gamepad.wButtons & XINPUT_GAMEPAD_X;

			for (size_t i = 0; i < BUTT_COUNT; ++i)
				if (current[static_cast<Butts>(i)] && !last_frame[static_cast<Butts>(i)])
					flags |= onDown[static_cast<Butts>(i)];
				else if (!current[static_cast<Butts>(i)] && last_frame[static_cast<Butts>(i)])
					flags |= onUp[static_cast<Butts>(i)];

			//current[bX1] = xis.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
			//current[bX2] = xis.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;

			int scroll = xis.Gamepad.bRightTrigger - xis.Gamepad.bLeftTrigger;

			if (abs(scroll) > 10)
			{
				flags |= MOUSEEVENTF_WHEEL;
				inp.mi.mouseData = scroll / 10;
			}


			last_frame = current;

			inp.mi.dwFlags = flags;

			SendInput(1, &inp, sizeof(inp));
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return static_cast<int>(msg.wParam);
}
