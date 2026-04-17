#include "menu.h"
#include "config.h"
#include "variables.h"
#include "../utilities/render.h"
#include "../utilities/input.h"
#include "../utilities/notify.h"
#include "../utilities/easing.h"
#include "../sdk/datatypes/color.h"
#include "../sdk/common.h"

#include <algorithm>
#include <imgui.h>
#include <imgui_internal.h>

#include <string>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

// ---------------------------------------------------------------
// constants
// ---------------------------------------------------------------
static constexpr int MAX_PARTICLES = 120;
static constexpr float PARTICLE_CONNECT_DIST = 180.0f;
static constexpr float PARTICLE_RADIUS = 2.0f;
static constexpr float MENU_WIDTH = 640.0f;
static constexpr float MENU_HEIGHT = 480.0f;

// ---------------------------------------------------------------
// internal state
// ---------------------------------------------------------------
static char szConfigName[128] = {};
static std::vector<std::string> vecConfigList;
static bool bStyleInitialized = false;
static bool bWaitingForKey = false;
static int* pKeyTarget = nullptr;
static int nWaitFrames = 0;
static int nCurrentTab = 0;
static bool bParticlesInitialized = false;
static std::vector<Particle_t> vecParticles;
static float flLastFrameTime = 0.0f;

// ---------------------------------------------------------------
// animation handler
// ---------------------------------------------------------------
void AnimationHandler_t::Update(float flDeltaTime, float flDuration)
{
	if (flDuration <= 0.0f) flDuration = 0.2f;
	const float flStep = flDeltaTime / flDuration;

	if (bSwitch)
	{
		flValue += flStep;
		if (flValue > 1.0f) flValue = 1.0f;
	}
	else
	{
		flValue -= flStep;
		if (flValue < 0.0f) flValue = 0.0f;
	}
}

// ---------------------------------------------------------------
// particle system
// ---------------------------------------------------------------
static float RandFloat(float lo, float hi)
{
	return lo + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (hi - lo)));
}

void MENU::Particles::Initialize(const ImVec2& screenSize)
{
	srand(static_cast<unsigned>(time(nullptr)));
	vecParticles.clear();
	vecParticles.reserve(MAX_PARTICLES);

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		Particle_t p;
		p.vecPosition = ImVec2(RandFloat(0.0f, screenSize.x), RandFloat(0.0f, screenSize.y));
		p.vecVelocity = ImVec2(RandFloat(-30.0f, 30.0f), RandFloat(-30.0f, 30.0f));
		vecParticles.push_back(p);
	}
	bParticlesInitialized = true;
}

void MENU::Particles::Render(ImDrawList* pDrawList, const ImVec2& screenSize, float flAlpha)
{
	if (!bParticlesInitialized || vecParticles.empty())
		Initialize(screenSize);

	const float flDelta = ImGui::GetIO().DeltaTime;
	const ImU32 colParticle = IM_COL32(200, 200, 220, static_cast<int>(100.0f * flAlpha));

	for (size_t i = 0; i < vecParticles.size(); i++)
	{
		auto& p = vecParticles[i];

		// update position
		p.vecPosition.x += p.vecVelocity.x * flDelta;
		p.vecPosition.y += p.vecVelocity.y * flDelta;

		// bounce off edges
		if (p.vecPosition.x < 0.0f) { p.vecPosition.x = 0.0f; p.vecVelocity.x *= -1.0f; }
		if (p.vecPosition.y < 0.0f) { p.vecPosition.y = 0.0f; p.vecVelocity.y *= -1.0f; }
		if (p.vecPosition.x > screenSize.x) { p.vecPosition.x = screenSize.x; p.vecVelocity.x *= -1.0f; }
		if (p.vecPosition.y > screenSize.y) { p.vecPosition.y = screenSize.y; p.vecVelocity.y *= -1.0f; }

		pDrawList->AddCircleFilled(p.vecPosition, PARTICLE_RADIUS, colParticle);

		// draw connections to nearby particles
		for (size_t j = i + 1; j < vecParticles.size(); j++)
		{
			const auto& other = vecParticles[j];
			const float dx = p.vecPosition.x - other.vecPosition.x;
			const float dy = p.vecPosition.y - other.vecPosition.y;
			const float distSq = dx * dx + dy * dy;

			if (distSq < PARTICLE_CONNECT_DIST * PARTICLE_CONNECT_DIST)
			{
				const float dist = std::sqrtf(distSq);
				const float lineAlpha = (1.0f - dist / PARTICLE_CONNECT_DIST) * flAlpha;
				const ImU32 colLine = IM_COL32(150, 150, 170, static_cast<int>(60.0f * lineAlpha));
				pDrawList->AddLine(p.vecPosition, other.vecPosition, colLine, 1.0f);
			}
		}
	}
}

// ---------------------------------------------------------------
// key name lookup
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
		if (vkCode >= 'A' && vkCode <= 'Z') { buf[0] = static_cast<char>(vkCode); buf[1] = '\0'; return buf; }
		if (vkCode >= '0' && vkCode <= '9') { buf[0] = static_cast<char>(vkCode); buf[1] = '\0'; return buf; }
		if (vkCode >= VK_F1 && vkCode <= VK_F12) { snprintf(buf, 16, "F%d", vkCode - VK_F1 + 1); return buf; }
		snprintf(buf, 16, "0x%02X", vkCode); return buf;
	}
}

// =================================================================
// style setup with dynamic accent colors
// =================================================================
static void UpdateStyleColors()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* c = style.Colors;

	const Color& acc = C::Get<Color>(menu_accent_color);
	const Color& accH = C::Get<Color>(menu_accent_hover);
	const Color& accA = C::Get<Color>(menu_accent_active);

	const ImVec4 accent   = ImVec4(acc.BaseFloat(0), acc.BaseFloat(1), acc.BaseFloat(2), 1.0f);
	const ImVec4 accentDk = ImVec4(accent.x * 0.75f, accent.y * 0.75f, accent.z * 0.75f, 1.0f);
	const ImVec4 accentLt = ImVec4(accA.BaseFloat(0), accA.BaseFloat(1), accA.BaseFloat(2), 1.0f);
	const ImVec4 accentHv = ImVec4(accH.BaseFloat(0), accH.BaseFloat(1), accH.BaseFloat(2), 1.0f);

	c[ImGuiCol_TitleBgActive]        = accentDk;
	c[ImGuiCol_ScrollbarGrabActive]  = accent;
	c[ImGuiCol_CheckMark]            = accentLt;
	c[ImGuiCol_SliderGrab]           = accent;
	c[ImGuiCol_SliderGrabActive]     = accentLt;
	c[ImGuiCol_ButtonHovered]        = accentHv;
	c[ImGuiCol_ButtonActive]         = accent;
	c[ImGuiCol_HeaderHovered]        = accentHv;
	c[ImGuiCol_HeaderActive]         = accent;
	c[ImGuiCol_SeparatorHovered]     = accent;
	c[ImGuiCol_SeparatorActive]      = accentLt;
	c[ImGuiCol_ResizeGripHovered]    = accent;
	c[ImGuiCol_ResizeGripActive]     = accentLt;
	c[ImGuiCol_TabHovered]           = accent;
	c[ImGuiCol_TabActive]            = accentDk;
	c[ImGuiCol_TextSelectedBg]       = ImVec4(accent.x, accent.y, accent.z, 0.35f);
}

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

	// base colors — dark theme
	ImVec4* c = style.Colors;
	c[ImGuiCol_WindowBg]             = ImVec4(0.07f, 0.07f, 0.09f, 0.96f);
	c[ImGuiCol_ChildBg]              = ImVec4(0.06f, 0.06f, 0.08f, 0.60f);
	c[ImGuiCol_PopupBg]              = ImVec4(0.08f, 0.08f, 0.10f, 0.96f);
	c[ImGuiCol_Border]               = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
	c[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	c[ImGuiCol_FrameBg]              = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
	c[ImGuiCol_FrameBgHovered]       = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
	c[ImGuiCol_FrameBgActive]        = ImVec4(0.18f, 0.16f, 0.24f, 1.00f);
	c[ImGuiCol_TitleBg]              = ImVec4(0.05f, 0.05f, 0.07f, 1.00f);
	c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.05f, 0.05f, 0.07f, 0.50f);
	c[ImGuiCol_MenuBarBg]            = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
	c[ImGuiCol_ScrollbarBg]          = ImVec4(0.05f, 0.05f, 0.07f, 0.50f);
	c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
	c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.28f, 0.35f, 1.00f);
	c[ImGuiCol_Button]               = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
	c[ImGuiCol_Header]               = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
	c[ImGuiCol_Separator]            = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
	c[ImGuiCol_ResizeGrip]           = ImVec4(0.20f, 0.20f, 0.25f, 0.25f);
	c[ImGuiCol_Tab]                  = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
	c[ImGuiCol_TabUnfocused]         = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
	c[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
	c[ImGuiCol_Text]                 = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
	c[ImGuiCol_TextDisabled]         = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);

	UpdateStyleColors();
	bStyleInitialized = true;
}

// =================================================================
// custom widgets
// =================================================================
bool MENU::Checkbox(const char* label, bool* value)
{
	return ImGui::Checkbox(label, value);
}

bool MENU::SliderFloat(const char* label, float* value, float min, float max, const char* fmt, int flags)
{
	return ImGui::SliderFloat(label, value, min, max, fmt, static_cast<ImGuiSliderFlags>(flags));
}

bool MENU::SliderInt(const char* label, int* value, int min, int max)
{
	return ImGui::SliderInt(label, value, min, max);
}

bool MENU::ColorEdit(const char* label, Color* color)
{
	if (!color) return false;
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
	if (value && itemCount > 0)
		*value = std::clamp(*value, 0, itemCount - 1);
	return ImGui::Combo(label, value, items, itemCount);
}

bool MENU::KeyBind(const char* label, int* key)
{
	if (!key) return false;
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
		if (nWaitFrames > 0) { nWaitFrames--; return false; }
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
	if (label) ImGui::SeparatorText(label);
	else ImGui::Separator();
}

void MENU::Tooltip(const char* text)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("%s", text);
}

// =================================================================
// sidebar tab definitions
// =================================================================
struct TabDef
{
	const char* szLabel;
	void (*pRender)();
};

// forward declarations for tab render functions
static void RenderAimbotTab();
static void RenderRagebotTab();
static void RenderVisualsTab();
static void RenderMiscTab();
static void RenderInventoryTab();
static void RenderConfigTab();
static void RenderSettingsTab();

static const TabDef g_tabs[] = {
	{ "Aimbot",    RenderAimbotTab },
	{ "Ragebot",   RenderRagebotTab },
	{ "Visuals",   RenderVisualsTab },
	{ "Misc",      RenderMiscTab },
	{ "Inventory", RenderInventoryTab },
	{ "Config",    RenderConfigTab },
	{ "Settings",  RenderSettingsTab },
};
static constexpr int TAB_COUNT = sizeof(g_tabs) / sizeof(g_tabs[0]);

// =================================================================
// tab rendering
// =================================================================
static void RenderAimbotTab()
{
	const float halfW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);

	// left column — aimbot
	ImGui::BeginChild("##aim_left", ImVec2(halfW, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("Aimbot");
		MENU::Checkbox("Enable##aim", &C::Get<bool>(aimbot_enabled));
		MENU::Checkbox("Always On##aim", &C::Get<bool>(aimbot_always_on));
		MENU::Tooltip("Aim without holding a key");

		ImGui::Spacing();

		ImGui::BeginDisabled(!C::Get<bool>(aimbot_enabled));

		MENU::SliderFloat("FOV", &C::Get<float>(aimbot_fov), 0.1f, 180.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
		MENU::Tooltip("Aimbot field of view cone (logarithmic scale)");

		MENU::SliderFloat("Smooth", &C::Get<float>(aimbot_smooth), 0.0f, 20.0f, "%.1f");
		MENU::Tooltip("Higher = slower aim correction");

		static const char* boneNames[] = { "Head", "Neck", "Chest", "Stomach", "Pelvis" };
		static const int boneLUT[] = { 6, 5, 4, 2, 0 };

		int& bone = C::Get<int>(aimbot_bone);
		int currentSel = 0;
		for (int i = 0; i < 5; i++)
			if (boneLUT[i] == bone) { currentSel = i; break; }
		if (MENU::Combo("Target Bone", &currentSel, boneNames, 5))
			bone = boneLUT[currentSel];

		MENU::Checkbox("Visible Only", &C::Get<bool>(aimbot_visible_only));
		MENU::Checkbox("Recoil Compensation", &C::Get<bool>(aimbot_rcs));
		MENU::Tooltip("Compensate for weapon recoil when aiming");
		MENU::Checkbox("Team Check##aim", &C::Get<bool>(aimbot_team_check));
		MENU::Tooltip("Only target enemies, not teammates");

		static const char* filterNames[] = { "Closest Angle", "Lowest Health", "Closest Distance" };
		MENU::Combo("Target Filter", &C::Get<int>(aimbot_target_filter), filterNames, 3);
		MENU::Tooltip("How to select which enemy to aim at");

		ImGui::Spacing();
		ImGui::BeginDisabled(C::Get<bool>(aimbot_always_on));
		ImGui::Text("Aim Key:");
		MENU::KeyBind("aimkey", &C::Get<int>(aimbot_key));
		ImGui::EndDisabled();

		ImGui::EndDisabled();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// right column — triggerbot
	ImGui::BeginChild("##aim_right", ImVec2(0, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("Triggerbot");
		MENU::Checkbox("Enable##trig", &C::Get<bool>(triggerbot_enabled));
		MENU::Checkbox("Always On##trig", &C::Get<bool>(triggerbot_always_on));
		MENU::Tooltip("Fire without holding a key");

		ImGui::Spacing();

		ImGui::BeginDisabled(!C::Get<bool>(triggerbot_enabled));

		MENU::Checkbox("Team Check##trig", &C::Get<bool>(triggerbot_team_check));
		MENU::Tooltip("Only shoot enemies, not teammates");

		MENU::Checkbox("Visible Only##trig", &C::Get<bool>(triggerbot_visible_only));
		MENU::Tooltip("Only fire when target is visible (not through walls)");

		MENU::SliderFloat("Delay (ms)", &C::Get<float>(triggerbot_delay), 0.0f, 500.0f, "%.0f");
		MENU::Tooltip("Delay before firing in milliseconds");

		MENU::SliderFloat("Randomization (ms)", &C::Get<float>(triggerbot_delay_rand), 0.0f, 200.0f, "%.0f");
		MENU::Tooltip("Random additional delay to appear more human");

		static const char* hitgroupNames[] = { "Any", "Head Only", "Chest Only", "Stomach Only" };
		MENU::Combo("Hitgroup Filter", &C::Get<int>(triggerbot_hitgroup), hitgroupNames, 4);
		MENU::Tooltip("Only fire when crosshair is on specific body part");

		ImGui::Spacing();
		ImGui::Text("Hold Randomization");
		MENU::SliderInt("Hold Min (ticks)", &C::Get<int>(triggerbot_hold_min), 1, 10);
		MENU::Tooltip("Minimum ticks to hold fire button down");
		MENU::SliderInt("Hold Max (ticks)", &C::Get<int>(triggerbot_hold_max), 1, 15);
		MENU::Tooltip("Maximum ticks to hold fire button — avoids 1-tick shots");

		ImGui::Spacing();
		ImGui::BeginDisabled(C::Get<bool>(triggerbot_always_on));
		ImGui::Text("Trigger Key:");
		MENU::KeyBind("trigkey", &C::Get<int>(triggerbot_key));
		ImGui::EndDisabled();

		ImGui::EndDisabled();
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
}

// ---------------------------------------------------------------
// Ragebot tab
// ---------------------------------------------------------------
static void RenderRagebotTab()
{
	const float halfW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);

	// left column — general settings
	ImGui::BeginChild("##rage_left", ImVec2(halfW, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("Ragebot");
		MENU::Checkbox("Enable##rage", &C::Get<bool>(rage_enabled));
		MENU::Checkbox("Always On##rage", &C::Get<bool>(rage_always_on));
		MENU::Tooltip("Run ragebot without holding a key");

		ImGui::Spacing();
		ImGui::BeginDisabled(!C::Get<bool>(rage_enabled));

		MENU::Checkbox("Silent Aim", &C::Get<bool>(rage_silent));
		MENU::Tooltip("Aim without moving your camera");
		MENU::Checkbox("Auto Shoot", &C::Get<bool>(rage_auto_shoot));
		MENU::Tooltip("Automatically fire when target is valid");
		MENU::Checkbox("Auto Stop", &C::Get<bool>(rage_auto_stop));
		MENU::Tooltip("Counter-strafe before shooting for accuracy");
		MENU::Checkbox("Auto Scope", &C::Get<bool>(rage_auto_scope));
		MENU::Tooltip("Automatically scope sniper rifles");
		MENU::Checkbox("Team Check##rage", &C::Get<bool>(rage_team_check));

		ImGui::Spacing();
		MENU::Separator("Autowall");
		MENU::SliderFloat("Min Damage", &C::Get<float>(rage_min_damage), 1.0f, 100.0f, "%.0f");
		MENU::Tooltip("Minimum damage required to shoot (autowall checks through walls)");

		ImGui::Spacing();
		ImGui::BeginDisabled(C::Get<bool>(rage_always_on));
		ImGui::Text("Rage Key:");
		MENU::KeyBind("ragekey", &C::Get<int>(rage_key));
		ImGui::EndDisabled();

		ImGui::EndDisabled();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// right column — hitbox + multipoint
	ImGui::BeginChild("##rage_right", ImVec2(0, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("Hitboxes");
		ImGui::BeginDisabled(!C::Get<bool>(rage_enabled));

		MENU::Checkbox("Head",    &C::Get<bool>(rage_hitbox_head));
		MENU::Checkbox("Chest",   &C::Get<bool>(rage_hitbox_chest));
		MENU::Checkbox("Stomach", &C::Get<bool>(rage_hitbox_stomach));
		MENU::Checkbox("Pelvis",  &C::Get<bool>(rage_hitbox_pelvis));

		ImGui::Spacing();
		MENU::Separator("Multipoint");
		MENU::Checkbox("Enable##mp", &C::Get<bool>(rage_multipoint));
		MENU::Tooltip("Scan multiple points per hitbox to find a shootable spot");

		ImGui::BeginDisabled(!C::Get<bool>(rage_multipoint));
		MENU::SliderFloat("Point Scale", &C::Get<float>(rage_multipoint_scale), 10.0f, 100.0f, "%.0f%%");
		MENU::Tooltip("Higher = more spread out points, lower = closer to center");
		ImGui::EndDisabled();

		ImGui::EndDisabled();
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
}

static void RenderVisualsTab()
{
	const float halfW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);

	// left column
	ImGui::BeginChild("##vis_left", ImVec2(halfW, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("ESP");
		MENU::Checkbox("Enable##esp", &C::Get<bool>(esp_enabled));
		MENU::Checkbox("Team ESP", &C::Get<bool>(esp_team));

		ImGui::Spacing();

		ImGui::BeginDisabled(!C::Get<bool>(esp_enabled));

		MENU::Checkbox("Box", &C::Get<bool>(esp_box));
		if (C::Get<bool>(esp_box))
		{
			static const char* boxTypes[] = { "Normal", "Corner" };
			MENU::Combo("Box Type", &C::Get<int>(esp_box_type), boxTypes, 2);
			MENU::ColorEdit("T Color##box", &C::Get<Color>(esp_box_color_t));
			MENU::ColorEdit("CT Color##box", &C::Get<Color>(esp_box_color_ct));
		}

		ImGui::Spacing();
		MENU::Checkbox("Name", &C::Get<bool>(esp_name));
		if (C::Get<bool>(esp_name))
			MENU::ColorEdit("Name Color", &C::Get<Color>(esp_name_color));
		MENU::Checkbox("Health Bar", &C::Get<bool>(esp_health));
		MENU::Checkbox("Armor Bar", &C::Get<bool>(esp_armor));
		if (C::Get<bool>(esp_armor))
			MENU::ColorEdit("Armor Color", &C::Get<Color>(esp_armor_color));
		MENU::Checkbox("Weapon", &C::Get<bool>(esp_weapon));
		if (C::Get<bool>(esp_weapon))
			MENU::ColorEdit("Weapon Color", &C::Get<Color>(esp_weapon_color));
		MENU::Checkbox("Dropped Items", &C::Get<bool>(esp_dropped_items));
		if (C::Get<bool>(esp_dropped_items))
			MENU::ColorEdit("Dropped Item Color", &C::Get<Color>(esp_dropped_item_color));
		MENU::Checkbox("Snaplines", &C::Get<bool>(esp_snaplines));
		if (C::Get<bool>(esp_snaplines))
		{
			MENU::ColorEdit("T Color##snapline", &C::Get<Color>(esp_snapline_color_t));
			MENU::ColorEdit("CT Color##snapline", &C::Get<Color>(esp_snapline_color_ct));
		}
		MENU::Checkbox("Skeleton", &C::Get<bool>(esp_skeleton));
		if (C::Get<bool>(esp_skeleton))
		{
			MENU::ColorEdit("T Color##skeleton", &C::Get<Color>(esp_skeleton_color_t));
			MENU::ColorEdit("CT Color##skeleton", &C::Get<Color>(esp_skeleton_color_ct));
			MENU::SliderFloat("Skeleton Thickness", &C::Get<float>(esp_skeleton_thickness), 1.0f, 5.0f);
		}

		ImGui::EndDisabled();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// right column — glow + live ESP preview
	ImGui::BeginChild("##vis_right", ImVec2(0, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("Glow");
		MENU::Checkbox("Enable##glow", &C::Get<bool>(glow_enabled));
		ImGui::BeginDisabled(!C::Get<bool>(glow_enabled));
		if (C::Get<bool>(glow_enabled))
		{
			MENU::ColorEdit("T Color##glow", &C::Get<Color>(glow_color_t));
			MENU::ColorEdit("CT Color##glow", &C::Get<Color>(glow_color_ct));
		}
		ImGui::EndDisabled();

		ImGui::Spacing();
		MENU::Separator("Preview");

		const float previewW = ImGui::GetContentRegionAvail().x;
		constexpr float previewH = 150.0f;
		const ImVec2 previewOrig = ImGui::GetCursorScreenPos();
		ImGui::Dummy(ImVec2(previewW, previewH));

		ImDrawList* pDraw = ImGui::GetWindowDrawList();
		pDraw->AddRectFilled(previewOrig, ImVec2(previewOrig.x + previewW, previewOrig.y + previewH),
			IM_COL32(10, 10, 15, 200), 4.0f);
		pDraw->AddRect(previewOrig, ImVec2(previewOrig.x + previewW, previewOrig.y + previewH),
			IM_COL32(40, 40, 55, 180), 4.0f);

		const float flHealthFactor = std::sinf(ImGui::GetTime() * 5.0f) * 0.25f + 0.75f;
		const float flArmorFactor = std::sinf(ImGui::GetTime() * 3.0f) * 0.2f + 0.55f;

		auto drawPreviewBox = [&](float x1, float y1, float x2, float y2, const Color& colBox)
		{
			const ImU32 col = colBox.ToImU32();
			if (C::Get<int>(esp_box_type) == 1)
			{
				const float boxW = x2 - x1;
				const float boxH = y2 - y1;
				const float cL = std::min(boxW, boxH) * 0.25f;
				pDraw->AddLine({ x1, y1 }, { x1 + cL, y1 }, col);
				pDraw->AddLine({ x1, y1 }, { x1, y1 + cL }, col);
				pDraw->AddLine({ x2, y1 }, { x2 - cL, y1 }, col);
				pDraw->AddLine({ x2, y1 }, { x2, y1 + cL }, col);
				pDraw->AddLine({ x1, y2 }, { x1 + cL, y2 }, col);
				pDraw->AddLine({ x1, y2 }, { x1, y2 - cL }, col);
				pDraw->AddLine({ x2, y2 }, { x2 - cL, y2 }, col);
				pDraw->AddLine({ x2, y2 }, { x2, y2 - cL }, col);
			}
			else
			{
				pDraw->AddRect({ x1, y1 }, { x2, y2 }, col);
			}
		};

		auto drawPreviewEntity = [&](float cx, const char* szName, const char* szWeapon,
			const Color& colBox, const Color& colSnapline)
		{
			const float boxH = C::Get<bool>(esp_team) ? 62.0f : 76.0f;
			const float boxW = boxH * 0.48f;
			const float y2 = previewOrig.y + previewH - 28.0f;
			const float y1 = y2 - boxH;
			const float x1 = cx - boxW * 0.5f;
			const float x2 = cx + boxW * 0.5f;

			if (C::Get<bool>(esp_snaplines))
				pDraw->AddLine({ previewOrig.x + previewW * 0.5f, previewOrig.y + previewH - 2.0f },
					{ cx, y2 }, colSnapline.ToImU32(), 1.0f);

			if (C::Get<bool>(esp_box))
				drawPreviewBox(x1, y1, x2, y2, colBox);

			if (C::Get<bool>(esp_health))
			{
				const float hbX = x1 - 7.0f;
				const float hbY = y1 + boxH * (1.0f - flHealthFactor);
				pDraw->AddRectFilled({ hbX, y1 }, { hbX + 4.0f, y2 }, IM_COL32(0, 0, 0, 160), 2.0f);
				const ImU32 hpColor = IM_COL32(
					static_cast<int>(255 * (1.0f - flHealthFactor)),
					static_cast<int>(205 * flHealthFactor), 50, 210);
				pDraw->AddRectFilled({ hbX, hbY }, { hbX + 4.0f, y2 }, hpColor, 2.0f);
			}

			if (C::Get<bool>(esp_armor))
			{
				const float abX = x2 + 3.0f;
				const float abY = y1 + boxH * (1.0f - flArmorFactor);
				pDraw->AddRectFilled({ abX, y1 }, { abX + 4.0f, y2 }, IM_COL32(0, 0, 0, 160), 2.0f);
				pDraw->AddRectFilled({ abX, abY }, { abX + 4.0f, y2 }, C::Get<Color>(esp_armor_color).ToImU32(), 2.0f);
			}

			if (C::Get<bool>(esp_name))
			{
				const ImVec2 tSz = ImGui::CalcTextSize(szName);
				pDraw->AddText({ cx - tSz.x * 0.5f, y1 - tSz.y - 2.0f }, C::Get<Color>(esp_name_color).ToImU32(), szName);
			}

			if (C::Get<bool>(esp_weapon))
			{
				const ImVec2 tSz = ImGui::CalcTextSize(szWeapon);
				pDraw->AddText({ cx - tSz.x * 0.5f, y2 + 2.0f }, C::Get<Color>(esp_weapon_color).ToImU32(), szWeapon);
			}
		};

		if (C::Get<bool>(esp_team))
		{
			drawPreviewEntity(previewOrig.x + previewW * 0.32f, "T", "AK-47",
				C::Get<Color>(esp_box_color_t),
				C::Get<Color>(esp_snapline_color_t));
			drawPreviewEntity(previewOrig.x + previewW * 0.68f, "CT", "M4A1",
				C::Get<Color>(esp_box_color_ct),
				C::Get<Color>(esp_snapline_color_ct));
		}
		else
		{
			drawPreviewEntity(previewOrig.x + previewW * 0.5f, "Enemy", "Rifle",
				C::Get<Color>(esp_box_color_ct),
				C::Get<Color>(esp_snapline_color_ct));
		}
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
}

static void RenderMiscTab()
{
	const float halfW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);

	ImGui::BeginChild("##misc_left", ImVec2(halfW, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("Movement");
		MENU::Checkbox("Bunny Hop", &C::Get<bool>(misc_bhop));
		MENU::Checkbox("Auto Strafe", &C::Get<bool>(misc_autostrafe));
		ImGui::BeginDisabled(!C::Get<bool>(misc_autostrafe));
		MENU::SliderFloat("Strafe Smooth", &C::Get<float>(misc_strafe_smooth), 1.0f, 100.0f, "%.0f");
		MENU::Tooltip("Air strafe smoothness (1=sharp, 100=smooth)");
		ImGui::EndDisabled();
		MENU::Checkbox("Auto Stop", &C::Get<bool>(misc_auto_stop));
		MENU::Tooltip("Counter-strafe to stop movement quickly (also triggered by ragebot)");

		ImGui::Spacing();
		MENU::Separator("Visual");
		MENU::Checkbox("No Flash", &C::Get<bool>(misc_noflash));
		if (C::Get<bool>(misc_noflash))
			MENU::SliderFloat("Flash Alpha", &C::Get<float>(misc_noflash_alpha), 0.0f, 255.0f, "%.0f");

		MENU::Checkbox("Sniper Crosshair", &C::Get<bool>(misc_sniper_crosshair));
		MENU::Tooltip("Draw crosshair when holding unscoped sniper");
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("##misc_right", ImVec2(0, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("Camera");
		MENU::Checkbox("Third Person", &C::Get<bool>(misc_thirdperson));
		if (C::Get<bool>(misc_thirdperson))
			MENU::SliderFloat("Distance", &C::Get<float>(misc_thirdperson_distance), 50.0f, 300.0f, "%.0f");

		MENU::SliderFloat("FOV Changer", &C::Get<float>(misc_fov_changer), 60.0f, 140.0f, "%.0f");

		ImGui::Spacing();
		MENU::Separator("Radar");
		MENU::Checkbox("Custom Radar", &C::Get<bool>(misc_radar_enabled));
		MENU::Tooltip("Draw a custom radar overlay showing enemy positions");
		if (C::Get<bool>(misc_radar_enabled))
		{
			MENU::SliderFloat("Radar Size", &C::Get<float>(misc_radar_size), 60.0f, 300.0f, "%.0f");
			MENU::SliderFloat("Radar Range", &C::Get<float>(misc_radar_range), 500.0f, 5000.0f, "%.0f");
			MENU::SliderFloat("Radar X", &C::Get<float>(misc_radar_pos_x), 0.0f, 100.0f, "%.0f%%");
			MENU::SliderFloat("Radar Y", &C::Get<float>(misc_radar_pos_y), 0.0f, 100.0f, "%.0f%%");
		}
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
}

static void RenderInventoryTab()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);

	ImGui::BeginChild("##inv_main", ImVec2(0, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("Skin Changer");
		MENU::Checkbox("Enable##inv", &C::Get<bool>(inventory_enabled));

		ImGui::Spacing();
		ImGui::TextDisabled("Select a weapon to configure skins:");
		ImGui::Spacing();

		ImGui::BeginChild("##skinlist", ImVec2(0, -4), ImGuiChildFlags_Borders);
		{
			ImGui::TextDisabled("No weapons configured yet.");
			ImGui::TextDisabled("Weapon skin UI will be added in a future update.");
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
}

static void RenderConfigTab()
{
	static bool bNeedsRefresh = true;
	if (bNeedsRefresh)
	{
		vecConfigList = C::GetConfigList();
		bNeedsRefresh = false;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);

	ImGui::BeginChild("##cfg_main", ImVec2(0, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("Configuration");

		// config file list
		ImGui::BeginChild("##configlist", ImVec2(0, -90), ImGuiChildFlags_Borders);
		{
			for (size_t i = 0; i < vecConfigList.size(); i++)
			{
				const bool bSelected = (strcmp(szConfigName, vecConfigList[i].c_str()) == 0);
				if (ImGui::Selectable(vecConfigList[i].c_str(), bSelected))
					strncpy_s(szConfigName, vecConfigList[i].c_str(), sizeof(szConfigName) - 1);
			}
			if (vecConfigList.empty())
				ImGui::TextDisabled("No configs found");
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
				NOTIFY::Push("Config saved!", NOTIFY_SUCCESS);
			}
		}
		ImGui::SameLine();
		if (MENU::Button("Load", ImVec2(flBtnW, 0)))
		{
			if (szConfigName[0] != '\0')
			{
				C::LoadFile(szConfigName);
				NOTIFY::Push("Config loaded!", NOTIFY_INFO);
			}
		}
		ImGui::SameLine();
		if (MENU::Button("Delete", ImVec2(flBtnW, 0)))
		{
			if (szConfigName[0] != '\0')
			{
				C::DeleteFile(szConfigName);
				bNeedsRefresh = true;
				NOTIFY::Push("Config deleted", NOTIFY_WARNING);
			}
		}
		ImGui::SameLine();
		if (MENU::Button("Refresh", ImVec2(flBtnW, 0)))
			bNeedsRefresh = true;
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
}

static void RenderSettingsTab()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);

	const float halfW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

	ImGui::BeginChild("##settings_left", ImVec2(halfW, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("Menu Effects");
		MENU::Checkbox("Particles", &C::Get<bool>(menu_particles));
		MENU::Tooltip("Floating particle background");

		MENU::Checkbox("Dim Background", &C::Get<bool>(menu_dim_background));
		MENU::Tooltip("Dim the game when menu is open");

		MENU::Checkbox("Glow Effect", &C::Get<bool>(menu_glow_effect));
		MENU::Tooltip("Shadow glow around menu window");

		MENU::Checkbox("Watermark", &C::Get<bool>(menu_watermark));

		ImGui::Spacing();
		MENU::SliderFloat("Anim Speed", &C::Get<float>(menu_animation_speed), 0.5f, 3.0f, "%.1fx");
		MENU::Tooltip("Menu animation speed multiplier");

		static const char* dpiOptions[] = { "100%", "125%", "150%", "175%", "200%" };
		MENU::Combo("DPI Scale", &C::Get<int>(menu_dpi_scale), dpiOptions, 5);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("##settings_right", ImVec2(0, 0), ImGuiChildFlags_Borders);
	{
		MENU::Separator("Theme Colors");
		MENU::ColorEdit("Accent", &C::Get<Color>(menu_accent_color));
		MENU::ColorEdit("Accent Hover", &C::Get<Color>(menu_accent_hover));
		MENU::ColorEdit("Accent Active", &C::Get<Color>(menu_accent_active));

		ImGui::Spacing();
		if (MENU::Button("Reset Colors", ImVec2(-1, 0)))
		{
			C::Get<Color>(menu_accent_color) = Color(85, 90, 160);
			C::Get<Color>(menu_accent_hover) = Color(100, 105, 175);
			C::Get<Color>(menu_accent_active) = Color(115, 120, 190);
			NOTIFY::Push("Colors reset to defaults", NOTIFY_INFO);
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::TextDisabled("GamerHack v1.0.0");
		ImGui::TextDisabled("Build: " __DATE__);
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
}

// =================================================================
// watermark
// =================================================================
void MENU::RenderWatermark()
{
	if (!C::Get<bool>(menu_watermark))
		return;

	ImDrawList* pDrawList = ImGui::GetForegroundDrawList();
	const ImVec2 screenSize = ImGui::GetIO().DisplaySize;

	char szBuf[128];
	snprintf(szBuf, sizeof(szBuf), "GamerHack v1.0.0 | %.0f fps", ImGui::GetIO().Framerate);

	const ImVec2 textSize = ImGui::CalcTextSize(szBuf);
	const float flPadX = 10.0f;
	const float flPadY = 4.0f;
	const float flX = screenSize.x - textSize.x - flPadX * 2.0f - 8.0f;
	const float flY = 8.0f;

	// background
	pDrawList->AddRectFilled(
		ImVec2(flX, flY),
		ImVec2(flX + textSize.x + flPadX * 2.0f, flY + textSize.y + flPadY * 2.0f),
		IM_COL32(15, 15, 20, 200), 6.0f);

	// accent bar at top
	const Color& acc = C::Get<Color>(menu_accent_color);
	pDrawList->AddRectFilled(
		ImVec2(flX, flY),
		ImVec2(flX + textSize.x + flPadX * 2.0f, flY + 2.0f),
		IM_COL32(acc.r, acc.g, acc.b, 200), 6.0f, ImDrawFlags_RoundCornersTop);

	// border
	pDrawList->AddRect(
		ImVec2(flX, flY),
		ImVec2(flX + textSize.x + flPadX * 2.0f, flY + textSize.y + flPadY * 2.0f),
		IM_COL32(40, 40, 50, 120), 6.0f);

	// text
	pDrawList->AddText(ImVec2(flX + flPadX, flY + flPadY), IM_COL32(220, 220, 230, 255), szBuf);
}

// =================================================================
// main render entry point — called from Present hook
// =================================================================
void MENU::Render()
{
	if (!bStyleInitialized)
		SetupStyle();

	// update animation
	const float flAnimSpeed = C::Get<float>(menu_animation_speed);
	menuAnimation.SetSwitch(bIsOpen);
	menuAnimation.Update(ImGui::GetIO().DeltaTime * flAnimSpeed, 0.3f);

	// update accent colors every frame
	UpdateStyleColors();

	// always render watermark and notifications
	RenderWatermark();
	NOTIFY::Render();

	const float flMenuAlpha = menuAnimation.GetValue();
	if (flMenuAlpha <= 0.01f)
		return;

	ImDrawList* pBgDrawList = ImGui::GetBackgroundDrawList();
	const ImVec2 screenSize = ImGui::GetIO().DisplaySize;

	// dim background
	if (C::Get<bool>(menu_dim_background))
	{
		const int dimAlpha = static_cast<int>(120.0f * flMenuAlpha);
		pBgDrawList->AddRectFilled(
			ImVec2(0, 0), screenSize,
			IM_COL32(0, 0, 0, dimAlpha));
	}

	// particle background
	if (C::Get<bool>(menu_particles))
		Particles::Render(pBgDrawList, screenSize, flMenuAlpha);

	// DPI scaling
	static const float dpiLUT[] = { 1.0f, 1.25f, 1.5f, 1.75f, 2.0f };
	const int nDpiIdx = std::clamp(C::Get<int>(menu_dpi_scale), 0, 4);
	const float flDpi = dpiLUT[nDpiIdx];

	const float flW = MENU_WIDTH * flDpi;
	const float flH = MENU_HEIGHT * flDpi;

	ImGui::SetNextWindowSize(ImVec2(flW, flH), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(500.0f, 400.0f), ImVec2(1600.0f, 1200.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, flMenuAlpha);

	constexpr ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoTitleBar;

	if (!ImGui::Begin("##GamerHackMain", nullptr, flags))
	{
		ImGui::End();
		ImGui::PopStyleVar();
		return;
	}

	// glow effect around window
	if (C::Get<bool>(menu_glow_effect))
	{
		const ImVec2 winPos = ImGui::GetWindowPos();
		const ImVec2 winSize = ImGui::GetWindowSize();
		const Color& acc = C::Get<Color>(menu_accent_color);
		const int glowAlpha = static_cast<int>(30.0f * flMenuAlpha);
		const float glowSize = 12.0f;

		ImDrawList* pFgDrawList = ImGui::GetForegroundDrawList();
		for (float i = glowSize; i > 0.0f; i -= 2.0f)
		{
			const int a = static_cast<int>(static_cast<float>(glowAlpha) * (i / glowSize));
			pFgDrawList->AddRect(
				ImVec2(winPos.x - i, winPos.y - i),
				ImVec2(winPos.x + winSize.x + i, winPos.y + winSize.y + i),
				IM_COL32(acc.r, acc.g, acc.b, a), 8.0f + i, 0, 1.0f);
		}
	}

	// title bar area
	{
		const Color& acc = C::Get<Color>(menu_accent_color);
		ImDrawList* pWinDrawList = ImGui::GetWindowDrawList();
		const ImVec2 winPos = ImGui::GetWindowPos();

		// gradient title bar
		pWinDrawList->AddRectFilledMultiColor(
			winPos,
			ImVec2(winPos.x + ImGui::GetWindowSize().x, winPos.y + 32.0f),
			IM_COL32(acc.r / 3, acc.g / 3, acc.b / 3, 200),
			IM_COL32(acc.r / 4, acc.g / 4, acc.b / 4, 200),
			IM_COL32(0, 0, 0, 0),
			IM_COL32(0, 0, 0, 0));

		// title text
		pWinDrawList->AddText(
			ImVec2(winPos.x + 14.0f, winPos.y + 8.0f),
			IM_COL32(220, 220, 230, 255), "GamerHack");

		// version right-aligned
		const char* szVer = "v1.0.0";
		const ImVec2 verSize = ImGui::CalcTextSize(szVer);
		pWinDrawList->AddText(
			ImVec2(winPos.x + ImGui::GetWindowSize().x - verSize.x - 14.0f, winPos.y + 8.0f),
			IM_COL32(140, 140, 150, 255), szVer);

		// separator line
		pWinDrawList->AddLine(
			ImVec2(winPos.x + 1.0f, winPos.y + 32.0f),
			ImVec2(winPos.x + ImGui::GetWindowSize().x - 1.0f, winPos.y + 32.0f),
			IM_COL32(acc.r, acc.g, acc.b, 120), 1.0f);
	}

	ImGui::Dummy(ImVec2(0, 24.0f)); // space for custom title bar

	// sidebar + content layout
	const float sidebarW = 110.0f * flDpi;
	const float contentH = ImGui::GetContentRegionAvail().y;

	// sidebar
	ImGui::BeginChild("##sidebar", ImVec2(sidebarW, contentH), false);
	{
		const Color& acc = C::Get<Color>(menu_accent_color);

		for (int i = 0; i < TAB_COUNT; i++)
		{
			const bool bSelected = (nCurrentTab == i);

			if (bSelected)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(acc.BaseFloat(0), acc.BaseFloat(1), acc.BaseFloat(2), 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(acc.BaseFloat(0), acc.BaseFloat(1), acc.BaseFloat(2), 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(acc.BaseFloat(0), acc.BaseFloat(1), acc.BaseFloat(2), 0.5f));
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.15f, 0.20f, 0.5f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.20f, 0.25f, 0.6f));
			}

			char szLabel[64];
			snprintf(szLabel, sizeof(szLabel), "  %s", g_tabs[i].szLabel);

			if (ImGui::Button(szLabel, ImVec2(sidebarW - 8.0f, 28.0f * flDpi)))
				nCurrentTab = i;

			ImGui::PopStyleColor(3);

			// accent indicator for selected tab
			if (bSelected)
			{
				ImDrawList* pWinDL = ImGui::GetWindowDrawList();
				const ImVec2 btnMin = ImGui::GetItemRectMin();
				const ImVec2 btnMax = ImGui::GetItemRectMax();
				pWinDL->AddRectFilled(
					ImVec2(btnMin.x, btnMin.y + 2.0f),
					ImVec2(btnMin.x + 3.0f, btnMax.y - 2.0f),
					IM_COL32(acc.r, acc.g, acc.b, 255), 2.0f);
			}
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// vertical separator
	{
		ImDrawList* pWinDL = ImGui::GetWindowDrawList();
		const ImVec2 curPos = ImGui::GetCursorScreenPos();
		pWinDL->AddLine(
			ImVec2(curPos.x - 4.0f, curPos.y),
			ImVec2(curPos.x - 4.0f, curPos.y + contentH),
			IM_COL32(40, 40, 50, 150), 1.0f);
	}

	// content area
	ImGui::BeginChild("##content", ImVec2(0, contentH), false);
	{
		if (nCurrentTab >= 0 && nCurrentTab < TAB_COUNT && g_tabs[nCurrentTab].pRender)
			g_tabs[nCurrentTab].pRender();
	}
	ImGui::EndChild();

	ImGui::End();
	ImGui::PopStyleVar();
}
