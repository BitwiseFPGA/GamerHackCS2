#include "notify.h"
#include "easing.h"
#include <imgui.h>
#include <algorithm>

void NOTIFY::Push(const char* szText, int nType)
{
	NotificationData_t notif;
	notif.nType = nType;
	notif.flCreationTime = static_cast<float>(ImGui::GetTime());
	snprintf(notif.szBuffer, sizeof(notif.szBuffer), "%s", szText);
	vecNotifications.push_back(notif);
}

void NOTIFY::Render()
{
	if (vecNotifications.empty())
		return;

	const float flCurrentTime = static_cast<float>(ImGui::GetTime());
	ImDrawList* pDrawList = ImGui::GetForegroundDrawList();
	const ImVec2 vecScreenSize = ImGui::GetIO().DisplaySize;

	float flYOffset = 30.0f;

	for (size_t i = 0; i < vecNotifications.size();)
	{
		auto& notif = vecNotifications[i];
		const float flTimeDelta = flCurrentTime - notif.flCreationTime;

		if (flTimeDelta >= MAX_TIME)
		{
			vecNotifications.erase(vecNotifications.begin() + i);
			continue;
		}

		// animation: fade in (0-0.3s), stay, fade out (last 0.5s)
		float flAlpha = 1.0f;
		float flSlide = 0.0f;

		if (flTimeDelta < 0.3f)
		{
			const float t = flTimeDelta / 0.3f;
			flAlpha = static_cast<float>(EASING::OutCubic(t));
			flSlide = (1.0f - flAlpha) * -200.0f;
		}
		else if (flTimeDelta > MAX_TIME - 0.5f)
		{
			const float t = (flTimeDelta - (MAX_TIME - 0.5f)) / 0.5f;
			flAlpha = 1.0f - static_cast<float>(EASING::InCubic(t));
			flSlide = (1.0f - flAlpha) * -200.0f;
		}

		if (flAlpha <= 0.01f)
		{
			i++;
			continue;
		}

		// measure text
		const char* szIcon = notif.GetIcon();
		const Color colType = notif.GetTypeColor();

		char szFull[196];
		snprintf(szFull, sizeof(szFull), "%s %s", szIcon, notif.szBuffer);

		const ImVec2 textSize = ImGui::CalcTextSize(szFull);
		const float flPadX = 12.0f;
		const float flPadY = 6.0f;
		const float flBoxW = textSize.x + flPadX * 2.0f;
		const float flBoxH = textSize.y + flPadY * 2.0f;

		// position: left side of screen
		const float flX = 10.0f + flSlide;
		const float flY = flYOffset;

		// background
		const ImU32 colBg = IM_COL32(20, 20, 30, static_cast<int>(200.0f * flAlpha));
		const ImU32 colBorder = IM_COL32(colType.r, colType.g, colType.b, static_cast<int>(150.0f * flAlpha));

		pDrawList->AddRectFilled(
			ImVec2(flX, flY),
			ImVec2(flX + flBoxW, flY + flBoxH),
			colBg, 4.0f);

		// accent bar on left
		pDrawList->AddRectFilled(
			ImVec2(flX, flY),
			ImVec2(flX + 3.0f, flY + flBoxH),
			colBorder, 2.0f);

		// border
		pDrawList->AddRect(
			ImVec2(flX, flY),
			ImVec2(flX + flBoxW, flY + flBoxH),
			IM_COL32(40, 40, 50, static_cast<int>(100.0f * flAlpha)), 4.0f);

		// text
		const ImU32 colText = IM_COL32(220, 220, 230, static_cast<int>(255.0f * flAlpha));
		pDrawList->AddText(
			ImVec2(flX + flPadX, flY + flPadY),
			colText, szFull);

		flYOffset += flBoxH + 4.0f;
		i++;
	}
}
