#pragma once
#include <cstdint>
#include <windows.h>

#include "../../utilities/memory.h"
#include "../../utilities/xorstr.h"
#include "../entity_handle.h"
#include "../functionlist.h"

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

		const int nIndex = hHandle.GetEntryIndex();
		if (nIndex < 0 || nIndex >= MAX_TOTAL_ENTITIES)
			return nullptr;

		return reinterpret_cast<T*>(GetEntityByIndex(nIndex));
	}

	int GetHighestEntityIndex()
	{
		return *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(this) + 0x20A0);
	}

private:
	void* GetEntityByIndex(int nIndex)
	{
		if (!SDK_FUNC::GetBaseEntity)
			return nullptr;

		__try
		{
			return SDK_FUNC::GetBaseEntity(this, nIndex);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return nullptr;
		}
	}
};
