// used: GET_X_LPARAM, GET_Y_LPARAM
#include <windowsx.h>

#include "input.h"

// used: L_PRINT
#include "log.h"
#include "xorstr.h"

// used: WndProc hook function
#include "../core/hooks.h"

// ---------------------------------------------------------------
// EnumWindows callback to find the main game window
// ---------------------------------------------------------------
static BOOL CALLBACK EnumWindowsCallback(HWND hHandle, LPARAM lParam)
{
	const auto IsMainWindow = [hHandle]()
	{
		return GetWindow(hHandle, GW_OWNER) == nullptr &&
			IsWindowVisible(hHandle) && hHandle != GetConsoleWindow();
	};

	DWORD nPID = 0;
	GetWindowThreadProcessId(hHandle, &nPID);

	if (GetCurrentProcessId() != nPID || !IsMainWindow())
		return TRUE;

	*reinterpret_cast<HWND*>(lParam) = hHandle;
	return FALSE;
}

// ---------------------------------------------------------------
// Setup — find window and hook WndProc
// ---------------------------------------------------------------
bool IPT::Setup()
{
	// enumerate windows to find the game's main window
	while (hWindow == nullptr)
	{
		EnumWindows(::EnumWindowsCallback, reinterpret_cast<LPARAM>(&hWindow));
		::Sleep(200U);
	}

	L_PRINT(LOG_INFO) << _XS("game window found: ") << reinterpret_cast<const void*>(hWindow);

	// install our WndProc
	pOldWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(H::WndProc)));
	if (pOldWndProc == nullptr)
	{
		L_PRINT(LOG_ERROR) << _XS("failed to install WndProc hook");
		return false;
	}

	L_PRINT(LOG_INFO) << _XS("WndProc hook installed");
	return true;
}

// ---------------------------------------------------------------
// Destroy — restore original WndProc
// ---------------------------------------------------------------
void IPT::Destroy()
{
	if (pOldWndProc != nullptr)
	{
		SetWindowLongPtrW(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pOldWndProc));
		pOldWndProc = nullptr;
	}

	L_PRINT(LOG_INFO) << _XS("WndProc hook restored");
}

// ---------------------------------------------------------------
// OnWndProc — process input messages and track key states
// ---------------------------------------------------------------
bool IPT::OnWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int nKey = 0;
	KeyState_t state = KEY_STATE_NONE;

	switch (uMsg)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (wParam < 256U)
		{
			nKey = static_cast<int>(wParam);
			state = KEY_STATE_DOWN;
		}
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if (wParam < 256U)
		{
			nKey = static_cast<int>(wParam);
			state = KEY_STATE_UP;
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		nKey = VK_LBUTTON;
		state = (uMsg == WM_LBUTTONUP) ? KEY_STATE_UP : KEY_STATE_DOWN;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
		nKey = VK_RBUTTON;
		state = (uMsg == WM_RBUTTONUP) ? KEY_STATE_UP : KEY_STATE_DOWN;
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		nKey = VK_MBUTTON;
		state = (uMsg == WM_MBUTTONUP) ? KEY_STATE_UP : KEY_STATE_DOWN;
		break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
		nKey = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
		state = (uMsg == WM_XBUTTONUP) ? KEY_STATE_UP : KEY_STATE_DOWN;
		break;
	default:
		return false;
	}

	// track released state as a transition from DOWN -> UP
	if (state == KEY_STATE_UP && arrKeyState[nKey] == KEY_STATE_DOWN)
		arrKeyState[nKey] = KEY_STATE_RELEASED;
	else
		arrKeyState[nKey] = state;

	return true;
}
