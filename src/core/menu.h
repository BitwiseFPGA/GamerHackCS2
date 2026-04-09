#pragma once
#include <imgui.h>

struct Color;

namespace MENU
{
	inline bool bIsOpen = false;

	void Render();
	void SetupStyle();

	// custom widget helpers
	bool Checkbox(const char* label, bool* value);
	bool SliderFloat(const char* label, float* value, float min, float max, const char* fmt = "%.1f");
	bool SliderInt(const char* label, int* value, int min, int max);
	bool ColorEdit(const char* label, Color* color);
	bool Combo(const char* label, int* value, const char** items, int itemCount);
	bool KeyBind(const char* label, int* key);
	bool Button(const char* label, const ImVec2& size = ImVec2(0, 0));
	void Separator(const char* label = nullptr);
	void Tooltip(const char* text);
}
