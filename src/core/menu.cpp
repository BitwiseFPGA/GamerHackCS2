#include "menu.h"
#include "config.h"
#include "variables.h"
#include "../utilities/render.h"
#include "../utilities/input.h"
#include "../sdk/datatypes/color.h"
#include "../sdk/common.h"

#include <imgui.h>

#include <string>
#include <vector>
#include <cstdio>
#include <algorithm>

// ---------------------------------------------------------------
// tab identifiers
// ---------------------------------------------------------------
enum EMenuTab : int
{
	TAB_AIMBOT = 0,
	TAB_VISUALS,
	TAB_MISC,
	TAB_INVENTORY,
	TAB_CONFIG,
	TAB_COUNT
};

// ---------------------------------------------------------------
// internal state
// ---------------------------------------------------------------
static char szConfigName[128] = {};
static std::vector<std::string> vecConfigList;
static bool bStyleInitialized = false;
static bool bWaitingForKey = false;
static int* pKeyTarget = nullptr;
static int nWaitFrames = 0;

// ---------------------------------------------------------------
// key name lookup (cycling buffer for safety)
// ---------------------------------------------------------------
static const char* GetKeyName(int vkCode)
{
	static char szBufs[4][16];
	static int nIdx = 0;
	char* buf = szBufs[nIdx++ & 3];

	switch (vkCode)
	{
	case 0:            return "None";
	case VK_LBUTTON:   return "Mouse1";
	case VK_RBUTTON:   return "Mouse2";
	case VK_MBUTTON:   return "Mouse3";
	case VK_XBUTTON1:  return "Mouse4";
	case VK_XBUTTON2:  return "Mouse5";
	case VK_SPACE:     return "Space";
	case VK_SHIFT:     return "Shift";
	case VK_LSHIFT:    return "LShift";
	case VK_RSHIFT:    return "RShift";
	case VK_CONTROL:   return "Ctrl";
	case VK_MENU:      return "Alt";
	case VK_TAB:       return "Tab";
	case VK_CAPITAL:   return "CapsLock";
	case VK_ESCAPE:    return "Esc";
	default:
		if (vkCode >= 'A' && vkCode <= 'Z')
		{
			buf[0] = static_cast<char>(vkCode);
			buf[1] = '\0';
			return buf;
		}
		if (vkCode >= '0' && vkCode <= '9')
		{
			buf[0] = static_cast<char>(vkCode);
			buf[1] = '\0';
			return buf;
		}
		if (vkCode >= VK_F1 && vkCode <= VK_F12)
		{
			snprintf(buf, sizeof(szBufs[0]), "F%d", vkCode - VK_F1 + 1);
			return buf;
		}
		snprintf(buf, sizeof(szBufs[0]), "0x%02X", vkCode);
		return buf;
	}
}

// =================================================================
// style setup
// =================================================================
void MENU::SetupStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();

	// sizing
	style.WindowPadding    = ImVec2(12.0f, 12.0f);
	style.FramePadding     = ImVec2(8.0f, 4.0f);
	style.ItemSpacing      = ImVec2(8.0f, 6.0f);
	style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
	style.ScrollbarSize    = 12.0f;
	style.GrabMinSize      = 8.0f;

	// borders
	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize  = 0.0f;
	style.PopupBorderSize  = 1.0f;

	// rounding
	style.WindowRounding    = 8.0f;
	style.FrameRounding     = 4.0f;
	style.PopupRounding     = 4.0f;
	style.ScrollbarRounding = 4.0f;
	style.GrabRounding      = 4.0f;
	style.TabRounding       = 4.0f;

	// colors — dark theme with purple-blue accent
	ImVec4* c = style.Colors;

	const ImVec4 accent   = ImVec4(0.40f, 0.30f, 0.85f, 1.00f);
	const ImVec4 accentDk = ImVec4(0.30f, 0.22f, 0.65f, 1.00f);
	const ImVec4 accentLt = ImVec4(0.55f, 0.45f, 0.95f, 1.00f);

	c[ImGuiCol_WindowBg]             = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);
	c[ImGuiCol_ChildBg]              = ImVec4(0.06f, 0.06f, 0.08f, 0.50f);
	c[ImGuiCol_PopupBg]              = ImVec4(0.08f, 0.08f, 0.10f, 0.96f);
	c[ImGuiCol_Border]               = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
	c[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	c[ImGuiCol_FrameBg]              = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
	c[ImGuiCol_FrameBgHovered]       = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
	c[ImGuiCol_FrameBgActive]        = ImVec4(0.20f, 0.18f, 0.28f, 1.00f);
	c[ImGuiCol_TitleBg]              = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
	c[ImGuiCol_TitleBgActive]        = accentDk;
	c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.06f, 0.06f, 0.08f, 0.50f);
	c[ImGuiCol_MenuBarBg]            = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	c[ImGuiCol_ScrollbarBg]          = ImVec4(0.06f, 0.06f, 0.08f, 0.50f);
	c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
	c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.28f, 0.35f, 1.00f);
	c[ImGuiCol_ScrollbarGrabActive]  = accent;
	c[ImGuiCol_CheckMark]            = accentLt;
	c[ImGuiCol_SliderGrab]           = accent;
	c[ImGuiCol_SliderGrabActive]     = accentLt;
	c[ImGuiCol_Button]               = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
	c[ImGuiCol_ButtonHovered]        = ImVec4(0.20f, 0.18f, 0.28f, 1.00f);
	c[ImGuiCol_ButtonActive]         = accent;
	c[ImGuiCol_Header]               = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
	c[ImGuiCol_HeaderHovered]        = ImVec4(0.20f, 0.18f, 0.28f, 1.00f);
	c[ImGuiCol_HeaderActive]         = accent;
	c[ImGuiCol_Separator]            = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
	c[ImGuiCol_SeparatorHovered]     = accent;
	c[ImGuiCol_SeparatorActive]      = accentLt;
	c[ImGuiCol_ResizeGrip]           = ImVec4(0.20f, 0.20f, 0.25f, 0.25f);
	c[ImGuiCol_ResizeGripHovered]    = accent;
	c[ImGuiCol_ResizeGripActive]     = accentLt;
	c[ImGuiCol_Tab]                  = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
	c[ImGuiCol_TabHovered]           = accent;
	c[ImGuiCol_TabActive]            = accentDk;
	c[ImGuiCol_TabUnfocused]         = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	c[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
	c[ImGuiCol_Text]                 = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
	c[ImGuiCol_TextDisabled]         = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);
	c[ImGuiCol_TextSelectedBg]       = ImVec4(accent.x, accent.y, accent.z, 0.35f);

	bStyleInitialized = true;
}

// =================================================================
// custom widgets
// =================================================================
bool MENU::Checkbox(const char* label, bool* value)
{
	return ImGui::Checkbox(label, value);
}

bool MENU::SliderFloat(const char* label, float* value, float min, float max, const char* fmt)
{
	return ImGui::SliderFloat(label, value, min, max, fmt);
}

bool MENU::SliderInt(const char* label, int* value, int min, int max)
{
	return ImGui::SliderInt(label, value, min, max);
}

bool MENU::ColorEdit(const char* label, Color* color)
{
	if (!color)
		return false;

	float col[4];
	color->ToFloat4(col);

	if (ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
	{
		*color = Color::FromFloat4(col);
		return true;
	}
	return false;
}

bool MENU::Combo(const char* label, int* value, const char** items, int itemCount)
{
	return ImGui::Combo(label, value, items, itemCount);
}

bool MENU::KeyBind(const char* label, int* key)
{
	if (!key)
		return false;

	const bool bThisWaiting = (bWaitingForKey && pKeyTarget == key);

	char szBuf[64];
	if (bThisWaiting)
		snprintf(szBuf, sizeof(szBuf), "[...] ##kb_%s", label);
	else
		snprintf(szBuf, sizeof(szBuf), "[%s] ##kb_%s", GetKeyName(*key), label);

	ImGui::SameLine();
	if (ImGui::SmallButton(szBuf))
	{
		bWaitingForKey = true;
		pKeyTarget = key;
		nWaitFrames = 3;
		return false;
	}

	if (bThisWaiting)
	{
		// skip a few frames to avoid catching the click that started waiting
		if (nWaitFrames > 0)
		{
			nWaitFrames--;
			return false;
		}

		for (int i = 1; i < 256; i++)
		{
			if (i == VK_INSERT) continue;

			if (GetAsyncKeyState(i) & 0x8000)
			{
				*key = (i == VK_ESCAPE) ? 0 : i;
				bWaitingForKey = false;
				pKeyTarget = nullptr;
				return true;
			}
		}
	}

	return false;
}

bool MENU::Button(const char* label, const ImVec2& size)
{
	return ImGui::Button(label, size);
}

void MENU::Separator(const char* label)
{
	if (label)
		ImGui::SeparatorText(label);
	else
		ImGui::Separator();
}

void MENU::Tooltip(const char* text)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("%s", text);
}

// =================================================================
// tab rendering
// =================================================================
static void RenderAimbotTab()
{
	MENU::Checkbox("Enable##aim", &C::Get<bool>(aimbot_enabled));

	MENU::SliderFloat("FOV", &C::Get<float>(aimbot_fov), 0.0f, 30.0f, "%.1f");
	MENU::Tooltip("Aimbot field of view cone");

	MENU::SliderFloat("Smooth", &C::Get<float>(aimbot_smooth), 0.0f, 20.0f, "%.1f");
	MENU::Tooltip("Higher = slower aim correction");

	// bone selection — map combo index to game bone index
	static const char* boneNames[] = { "Head", "Neck", "Chest", "Stomach", "Pelvis" };
	static const int boneLUT[] = { 6, 5, 4, 2, 0 };

	int& bone = C::Get<int>(aimbot_bone);
	int currentSel = 0;
	for (int i = 0; i < 5; i++)
	{
		if (boneLUT[i] == bone) { currentSel = i; break; }
	}
	if (MENU::Combo("Target Bone", &currentSel, boneNames, 5))
		bone = boneLUT[currentSel];

	MENU::Checkbox("Visible Only", &C::Get<bool>(aimbot_visible_only));

	ImGui::Text("Aim Key:");
	MENU::KeyBind("aimkey", &C::Get<int>(aimbot_key));
}

static void RenderVisualsTab()
{
	MENU::Checkbox("Enable##esp", &C::Get<bool>(esp_enabled));

	MENU::Separator("Box ESP");
	MENU::Checkbox("Box", &C::Get<bool>(esp_box));

	if (C::Get<bool>(esp_box))
	{
		static const char* boxTypes[] = { "Normal", "Corner" };
		MENU::Combo("Box Type", &C::Get<int>(esp_box_type), boxTypes, 2);
		MENU::ColorEdit("T Color##box", &C::Get<Color>(esp_box_color_t));
		MENU::ColorEdit("CT Color##box", &C::Get<Color>(esp_box_color_ct));
	}

	MENU::Separator("Player Info");
	MENU::Checkbox("Name", &C::Get<bool>(esp_name));
	MENU::Checkbox("Health Bar", &C::Get<bool>(esp_health));
	MENU::Checkbox("Armor Bar", &C::Get<bool>(esp_armor));
	MENU::Checkbox("Weapon", &C::Get<bool>(esp_weapon));
	MENU::Checkbox("Skeleton", &C::Get<bool>(esp_skeleton));
	MENU::Checkbox("Snaplines", &C::Get<bool>(esp_snaplines));

	MENU::Separator("Glow");
	MENU::Checkbox("Enable##glow", &C::Get<bool>(glow_enabled));
	if (C::Get<bool>(glow_enabled))
	{
		MENU::ColorEdit("T Color##glow", &C::Get<Color>(glow_color_t));
		MENU::ColorEdit("CT Color##glow", &C::Get<Color>(glow_color_ct));
	}
}

static void RenderMiscTab()
{
	MENU::Separator("Movement");
	MENU::Checkbox("Bunny Hop", &C::Get<bool>(misc_bhop));
	MENU::Checkbox("Auto Strafe", &C::Get<bool>(misc_autostrafe));

	MENU::Separator("Visual");
	MENU::Checkbox("No Flash", &C::Get<bool>(misc_noflash));
	if (C::Get<bool>(misc_noflash))
		MENU::SliderFloat("Flash Alpha", &C::Get<float>(misc_noflash_alpha), 0.0f, 255.0f, "%.0f");

	MENU::Separator("Camera");
	MENU::Checkbox("Third Person", &C::Get<bool>(misc_thirdperson));
	if (C::Get<bool>(misc_thirdperson))
		MENU::SliderFloat("Distance", &C::Get<float>(misc_thirdperson_distance), 50.0f, 300.0f, "%.0f");

	MENU::SliderFloat("FOV Changer", &C::Get<float>(misc_fov_changer), 60.0f, 140.0f, "%.0f");

	MENU::Separator("Other");
	MENU::Checkbox("Watermark", &C::Get<bool>(misc_watermark));
}

static void RenderInventoryTab()
{
	MENU::Checkbox("Enable##inv", &C::Get<bool>(inventory_enabled));

	MENU::Separator();
	ImGui::TextDisabled("Skin changer configuration");
	ImGui::TextDisabled("Select a weapon to configure skins:");

	ImGui::BeginChild("##skinlist", ImVec2(0, 0), ImGuiChildFlags_Borders);
	{
		ImGui::TextDisabled("No weapons configured yet.");
		ImGui::TextDisabled("Weapon skin UI will be added here.");
	}
	ImGui::EndChild();
}

static void RenderConfigTab()
{
	static bool bNeedsRefresh = true;
	if (bNeedsRefresh)
	{
		vecConfigList = C::GetConfigList();
		bNeedsRefresh = false;
	}

	// config file list
	ImGui::BeginChild("##configlist", ImVec2(0, -80), ImGuiChildFlags_Borders);
	{
		for (size_t i = 0; i < vecConfigList.size(); i++)
		{
			if (ImGui::Selectable(vecConfigList[i].c_str()))
				strncpy_s(szConfigName, vecConfigList[i].c_str(), sizeof(szConfigName) - 1);
		}
	}
	ImGui::EndChild();

	ImGui::InputText("Config Name", szConfigName, sizeof(szConfigName));

	const float flBtnW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 3.0f) / 4.0f;

	if (MENU::Button("Save", ImVec2(flBtnW, 0)))
	{
		if (szConfigName[0] != '\0')
		{
			C::SaveFile(szConfigName);
			bNeedsRefresh = true;
		}
	}

	ImGui::SameLine();
	if (MENU::Button("Load", ImVec2(flBtnW, 0)))
	{
		if (szConfigName[0] != '\0')
			C::LoadFile(szConfigName);
	}

	ImGui::SameLine();
	if (MENU::Button("Delete", ImVec2(flBtnW, 0)))
	{
		if (szConfigName[0] != '\0')
		{
			C::DeleteFile(szConfigName);
			bNeedsRefresh = true;
		}
	}

	ImGui::SameLine();
	if (MENU::Button("Refresh", ImVec2(flBtnW, 0)))
		bNeedsRefresh = true;
}

// =================================================================
// main render entry point — called from Present hook
// =================================================================
void MENU::Render()
{
	if (!bIsOpen)
		return;

	if (!bStyleInitialized)
		SetupStyle();

	ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);

	constexpr ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar;

	if (!ImGui::Begin("GamerHack v1.0.0", &bIsOpen, flags))
	{
		ImGui::End();
		return;
	}

	if (ImGui::BeginTabBar("##tabs"))
	{
		if (ImGui::BeginTabItem("Aimbot"))
		{
			ImGui::BeginChild("##aimbot_c", ImVec2(0, 0), false);
			RenderAimbotTab();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Visuals"))
		{
			ImGui::BeginChild("##visuals_c", ImVec2(0, 0), false);
			RenderVisualsTab();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Misc"))
		{
			ImGui::BeginChild("##misc_c", ImVec2(0, 0), false);
			RenderMiscTab();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Inventory"))
		{
			ImGui::BeginChild("##inv_c", ImVec2(0, 0), false);
			RenderInventoryTab();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Config"))
		{
			ImGui::BeginChild("##cfg_c", ImVec2(0, 0), false);
			RenderConfigTab();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}
