#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>

// forward declaration
struct ImVec2;

/*
 * INPUT SYSTEM
 * - window message capture via WndProc hook
 * - keyboard / mouse state tracking
 * - menu toggle logic
 */
namespace IPT
{
	using KeyState_t = std::uint8_t;

	enum EKeyState : KeyState_t
	{
		KEY_STATE_NONE,
		KEY_STATE_DOWN,
		KEY_STATE_UP,
		KEY_STATE_RELEASED
	};

	/* @section: values */
	inline HWND hWindow = nullptr;
	inline WNDPROC pOldWndProc = nullptr;
	inline KeyState_t arrKeyState[256] = {};

	/* @section: lifecycle */
	/// find game window and install WndProc hook
	bool Setup();
	/// restore original WndProc
	void Destroy();

	/* @section: callbacks */
	/// process input window message and save key states
	/// @returns: true if a key state was updated
	bool OnWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/* @section: key queries */
	/// @returns: true if key is currently held down
	[[nodiscard]] inline bool IsKeyDown(const std::uint32_t uKeyCode)
	{
		return arrKeyState[uKeyCode] == KEY_STATE_DOWN;
	}

	/// @returns: true if key was just released (rising edge, auto-resets)
	[[nodiscard]] inline bool IsKeyReleased(const std::uint32_t uKeyCode)
	{
		if (arrKeyState[uKeyCode] == KEY_STATE_RELEASED)
		{
			arrKeyState[uKeyCode] = KEY_STATE_UP;
			return true;
		}
		return false;
	}

	/// @returns: true if key is in the UP state
	[[nodiscard]] inline bool IsKeyUp(const std::uint32_t uKeyCode)
	{
		return arrKeyState[uKeyCode] == KEY_STATE_UP || arrKeyState[uKeyCode] == KEY_STATE_NONE;
	}
}
