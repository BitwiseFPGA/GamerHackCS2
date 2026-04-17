#pragma once
#include "../../utilities/memory.h"

// CPVS — Potentially Visible Set (model occlusion system)
// Disabling PVS prevents the game from culling entity models that are
// behind walls or otherwise occluded. Both Andromeda and Asphyxia
// disable this in LevelInit to keep enemy models rendered at all times.
class CPVS
{
public:
	void Set(bool bState)
	{
		MEM::CallVFunc<void*, 7U>(this, bState);
	}
};
