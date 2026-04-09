#pragma once
#include <cstdint>

#include "../../utilities/memory.h"
#include "../../utilities/xorstr.h"
#include "../entity_handle.h"

inline constexpr int MAX_ENTITIES_IN_LIST = 512;
inline constexpr int MAX_ENTITY_LISTS     = 64;
inline constexpr int MAX_TOTAL_ENTITIES   = MAX_ENTITIES_IN_LIST * MAX_ENTITY_LISTS;

class C_BaseEntity;

class CGameEntitySystem
{
public:
	/// get entity by index (typed)
	template <typename T = C_BaseEntity>
	T* Get(int nIndex)
	{
		return reinterpret_cast<T*>(GetEntityByIndex(nIndex));
	}

	/// get entity from handle (typed)
	template <typename T = C_BaseEntity>
	T* Get(const CBaseHandle hHandle)
	{
		if (!hHandle.IsValid())
			return nullptr;

		return reinterpret_cast<T*>(GetEntityByIndex(hHandle.GetEntryIndex()));
	}

	int GetHighestEntityIndex()
	{
		return *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(this) + 0x1510);
	}

private:
	void* GetEntityByIndex(int nIndex)
	{
		// pattern: "81 FA ? ? ? ? 77 ? 8B C2 C1 F8 ? 83 F8 ? 77 ..."
		// resolved via MEM::FindPattern at init time
		using fnGetBaseEntity = void*(__thiscall*)(void*, int);
		static auto oGetBaseEntity = reinterpret_cast<fnGetBaseEntity>(
			MEM::FindPattern(_XS("client.dll"),
				_XS("81 FA ? ? ? ? 77 ? 8B C2 C1 F8 ? 83 F8 ? 77 ? 48 98 48 8B 4C C1 ? 48 85 C9 74 ? 8B C2 25 ? ? ? ? 48 6B C0 ? 48 03 C8 74 ? 8B 41 ? 25 ? ? ? ? 3B C2 75 ? 48 8B 01")));

		return oGetBaseEntity(this, nIndex);
	}
};
