#pragma once
#include <vector>
#include <cstdio>
#include "../sdk/datatypes/color.h"

/*
 * NOTIFICATION TOAST SYSTEM
 * Displays notification toasts on screen with animations
 */

enum ENotificationType : int
{
	NOTIFY_DEFAULT = 0,
	NOTIFY_INFO,
	NOTIFY_SUCCESS,
	NOTIFY_WARNING,
	NOTIFY_ERROR
};

struct NotificationData_t
{
	int nType = NOTIFY_DEFAULT;
	char szBuffer[128] = {};
	float flCreationTime = 0.0f;

	Color GetTypeColor() const
	{
		switch (nType)
		{
		case NOTIFY_INFO:    return Color(100, 200, 255);
		case NOTIFY_SUCCESS: return Color(100, 255, 100);
		case NOTIFY_WARNING: return Color(255, 200, 50);
		case NOTIFY_ERROR:   return Color(255, 80, 80);
		default:             return Color(200, 200, 200);
		}
	}

	const char* GetIcon() const
	{
		switch (nType)
		{
		case NOTIFY_INFO:    return "[i]";
		case NOTIFY_SUCCESS: return "[+]";
		case NOTIFY_WARNING: return "[!]";
		case NOTIFY_ERROR:   return "[x]";
		default:             return "[*]";
		}
	}
};

namespace NOTIFY
{
	inline std::vector<NotificationData_t> vecNotifications;
	inline constexpr float MAX_TIME = 5.0f;

	void Push(const char* szText, int nType = NOTIFY_DEFAULT);
	void Render();
}
