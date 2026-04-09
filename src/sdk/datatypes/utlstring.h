#pragma once
#include <cstdint>
#include <cstring>

// @source: master/public/tier1/utlstring.h
// Game-memory-compatible CUtlString for reading Source 2 string data.

class CUtlString
{
public:
	CUtlString() = default;

	[[nodiscard]] const char* Get() const { return m_pString ? m_pString : ""; }
	[[nodiscard]] bool IsEmpty() const { return !m_pString || m_pString[0] == '\0'; }
	[[nodiscard]] int Length() const { return m_pString ? static_cast<int>(std::strlen(m_pString)) : 0; }

	operator const char*() const { return Get(); }

private:
	char* m_pString = nullptr;
};
