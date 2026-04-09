#pragma once
#include <cstdint>
#include <functional>

#include "../../utilities/memory.h"

class IEngineClient
{
public:
	// VFunc index 34
	int GetMaxClients()
	{
		return MEM::CallVFunc<int, 34U>(this);
	}

	// VFunc index 35
	bool IsInGame()
	{
		return MEM::CallVFunc<bool, 35U>(this);
	}

	// VFunc index 36
	bool IsConnected()
	{
		return MEM::CallVFunc<bool, 36U>(this);
	}

	// VFunc index 49 — returns local player CBaseHandle index
	int GetLocalPlayer()
	{
		int nIndex = -1;
		MEM::CallVFunc<void, 49U>(this, std::ref(nIndex), 0);
		return nIndex + 1;
	}

	// VFunc index 56
	[[nodiscard]] const char* GetLevelName()
	{
		return MEM::CallVFunc<const char*, 56U>(this);
	}

	// VFunc index 57
	[[nodiscard]] const char* GetLevelNameShort()
	{
		return MEM::CallVFunc<const char*, 57U>(this);
	}

	// VFunc index 84
	[[nodiscard]] const char* GetProductVersionString()
	{
		return MEM::CallVFunc<const char*, 84U>(this);
	}
};
