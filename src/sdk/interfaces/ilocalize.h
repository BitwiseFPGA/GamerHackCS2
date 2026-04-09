#pragma once
#include <cstdint>

#include "../../utilities/memory.h"

// ---------------------------------------------------------------
// CLocalize — localization string lookup
// ---------------------------------------------------------------
class CLocalize
{
public:
	/// find a localized string by token name
	/// @param pszTokenName — localization token (e.g. "#SFUI_WPNHUD_AK47")
	/// @returns: localized UTF-8 string, or the token itself on failure
	const char* FindSafe(const char* pszTokenName)
	{
		return MEM::CallVFunc<const char*, 17U>(this, pszTokenName);
	}
};
