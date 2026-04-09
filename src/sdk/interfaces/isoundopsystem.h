#pragma once
#include <cstdint>

// ---------------------------------------------------------------
// CSoundEventManager — sound event hash lookup
// ---------------------------------------------------------------
class CSoundEventManager
{
public:
	virtual std::uint32_t GetSoundEventHash(const char* szSound) = 0;
	virtual bool          IsValidSoundEventHash(std::uint32_t nHash) = 0;
	virtual const char*   GetSoundEventName(std::uint32_t nHash) = 0;
};

// ---------------------------------------------------------------
// CSoundOpSystem — sound operation system
// ---------------------------------------------------------------
class CSoundOpSystem
{
public:
	/// get the sound event manager (at offset 0x8)
	[[nodiscard]] CSoundEventManager* GetCSoundEventManager()
	{
		return *reinterpret_cast<CSoundEventManager**>(
			reinterpret_cast<std::uintptr_t>(this) + 0x8);
	}
};
