#pragma once
#include <cstdint>
#include <functional>

#include "../../utilities/memory.h"
#include "../../sdk/functionlist.h"

class IEngineClient
{
public:
	int GetMaxClients()
	{
		// vtable[34] — used rarely; not crash-critical
		return MEM::CallVFunc<int, 34U>(this);
	}

	bool IsInGame()
	{
		// NOT a vtable call — vtable indices are unstable across CS2 updates.
		// Andromeda uses a pattern-scanned function for this.
		if (SDK_FUNC::IsInGame)
			return SDK_FUNC::IsInGame(this);
		return false;
	}

	bool IsConnected()
	{
		// vtable[36] — used rarely; if it crashes update to pattern-scan like IsInGame
		return MEM::CallVFunc<bool, 36U>(this);
	}

	// vtable[49] is suspect. Use SDK_FUNC::GetLocalPlayerController(-1) instead
	// whenever you need the local player — it's safer and already pattern-resolved.
	int GetLocalPlayer()
	{
		int nIndex = -1;
		MEM::CallVFunc<void, 49U>(this, std::ref(nIndex), 0);
		return nIndex + 1;
	}

	[[nodiscard]] const char* GetLevelName()
	{
		if (SDK_FUNC::GetLevelName)
			return SDK_FUNC::GetLevelName(this);
		return nullptr;
	}

	[[nodiscard]] const char* GetLevelNameShort()
	{
		if (SDK_FUNC::GetLevelNameShort)
			return SDK_FUNC::GetLevelNameShort(this);
		return nullptr;
	}

	[[nodiscard]] const char* GetProductVersionString()
	{
		return MEM::CallVFunc<const char*, 84U>(this);
	}
};
