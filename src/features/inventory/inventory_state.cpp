#include "inventory_state.h"

#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"

namespace
{
	std::vector<F::INVENTORY::STATE::SkinConfig> g_vecSkinConfigs;
}

bool F::INVENTORY::STATE::Setup()
{
	g_vecSkinConfigs.clear();

	L_PRINT(LOG_INFO) << _XS("[INVENTORY] initialized");
	return true;
}

void F::INVENTORY::STATE::Destroy()
{
	g_vecSkinConfigs.clear();
	L_PRINT(LOG_INFO) << _XS("[INVENTORY] destroyed");
}

std::vector<F::INVENTORY::STATE::SkinConfig>& F::INVENTORY::STATE::GetSkinConfigs()
{
	return g_vecSkinConfigs;
}
