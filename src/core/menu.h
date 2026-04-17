#pragma once
#include <imgui.h>

struct Color;

/*
 * PARTICLE SYSTEM
 * Floating particles with connection lines in menu background
 */
struct Particle_t
{
	ImVec2 vecPosition;
	ImVec2 vecVelocity;
};

/*
 * ANIMATION HANDLER
 * Smooth transitions with easing functions
 */
struct AnimationHandler_t
{
	bool bSwitch = false;
	float flValue = 0.0f;

	void Update(float flDeltaTime, float flDuration);
	float GetValue(float flScale = 1.0f) const { return flValue * flScale; }
	void SetSwitch(bool bState) { bSwitch = bState; }
};

namespace MENU
{
	inline bool bIsOpen = false;
	inline AnimationHandler_t menuAnimation;

	void Render();
	void SetupStyle();

	// custom widget helpers
	bool Checkbox(const char* label, bool* value);
	bool SliderFloat(const char* label, float* value, float min, float max, const char* fmt = "%.1f", int flags = 0);
	bool SliderInt(const char* label, int* value, int min, int max);
	bool ColorEdit(const char* label, Color* color);
	bool Combo(const char* label, int* value, const char** items, int itemCount);
	bool KeyBind(const char* label, int* key);
	bool Button(const char* label, const ImVec2& size = ImVec2(0, 0));
	void Separator(const char* label = nullptr);
	void Tooltip(const char* text);

	// watermark
	void RenderWatermark();

	// particle system
	namespace Particles
	{
		void Initialize(const ImVec2& screenSize);
		void Render(ImDrawList* pDrawList, const ImVec2& screenSize, float flAlpha);
	}
}
