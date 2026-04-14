#pragma once

#include <cstdint>
#include <vector>

namespace F::INVENTORY::STATE
{
	struct SkinConfig
	{
		std::uint16_t nWeaponDefIndex = 0;
		int nPaintKit = 0;
		int nSeed = 0;
		float flWear = 0.0001f;
		int nStatTrak = -1;
	};

	bool Setup();
	void Destroy();
	std::vector<SkinConfig>& GetSkinConfigs();
}
