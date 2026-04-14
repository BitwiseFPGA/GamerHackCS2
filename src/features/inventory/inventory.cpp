#include "inventory.h"

#include "../../core/config.h"
#include "../../core/variables.h"
#include "../../core/interfaces.h"
#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"
#include "../../sdk/entity.h"
#include "../../sdk/econ/econitem.h"
#include "../../sdk/econ/econitemschema.h"
#include "../../sdk/econ/inventory.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../sdk/const.h"

#include <vector>
#include <cstdint>

// ---------------------------------------------------------------
// per-weapon skin configuration (expand as needed)
// ---------------------------------------------------------------
struct SkinConfig
{
	std::uint16_t nWeaponDefIndex = 0;
	int   nPaintKit   = 0;
	int   nSeed       = 0;
	float flWear      = 0.0001f;
	int   nStatTrak   = -1; // -1 = disabled
};

// global skin config list — populated from menu / config file
static std::vector<SkinConfig> vecSkinConfigs;

// ---------------------------------------------------------------
// setup / destroy
// ---------------------------------------------------------------
bool F::INVENTORY::Setup()
{
	vecSkinConfigs.clear();

	// TODO: load paint kit and music kit lists from CEconItemSchema
	//   auto* pEconSystem = ... ;
	//   auto* pSchema = pEconSystem->GetEconItemSchema();
	//   iterate paint kits, music kits, etc.

	L_PRINT(LOG_INFO) << _XS("[INVENTORY] initialized");
	return true;
}

void F::INVENTORY::Destroy()
{
	vecSkinConfigs.clear();
	L_PRINT(LOG_INFO) << _XS("[INVENTORY] destroyed");
}

// ---------------------------------------------------------------
// OnFrameStageNotify — apply skins during net update
// ---------------------------------------------------------------
void F::INVENTORY::OnFrameStageNotify(int nStage)
{
	if (nStage != FRAME_NET_UPDATE_POSTDATAUPDATE_END)
		return;

	if (!C::Get<bool>(inventory_enabled))
		return;

	if (!I::Engine || !I::Engine->IsInGame())
		return;

	if (!I::GameEntitySystem)
		return;

	__try
	{
		auto* pLocalController = SDK_FUNC::GetLocalPlayerController ?
			SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
		if (!pLocalController || !pLocalController->IsPawnAlive())
			return;

		auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPawnHandle());
		if (!pLocalPawn)
			return;

		auto* pWeaponServices = pLocalPawn->GetWeaponServices();
		if (!pWeaponServices)
			return;

		// iterate weapon handles and apply skins
		// In CS2, weapon entities are typically iterated via the pawn's weapon services.
		// For each weapon, check if we have a SkinConfig matching its definition index.
		// If so, set fallback paint kit values on the weapon entity.
		//
		// Example flow:
		//   for each weapon handle in pWeaponServices->GetWeaponList():
		//     auto* pWeapon = I::GameEntitySystem->Get<C_CSWeaponBase>(hWeapon);
		//     auto* pAttrContainer = pWeapon->GetAttributeManager();
		//     auto* pItemView = pAttrContainer->GetItem();
		//     uint16_t nDefIdx = pItemView->GetItemDefinitionIndex();
		//
		//     auto it = find config for nDefIdx;
		//     if found:
		//       pItemView->SetFallbackPaintKit(config.nPaintKit);
		//       pItemView->SetFallbackSeed(config.nSeed);
		//       pItemView->SetFallbackWear(config.flWear);
		//       if config.nStatTrak >= 0:
		//         pItemView->SetFallbackStatTrak(config.nStatTrak);
		//       pItemView->SetItemIDHigh(-1); // force fallback

		// TODO: implement once schema field offsets for current build are confirmed
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLogged = false;
		if (!bLogged)
		{
			bLogged = true;
			L_PRINT(LOG_ERROR) << _XS("[INVENTORY] EXCEPTION in OnFrameStageNotify");
		}
	}
}

// ---------------------------------------------------------------
// ApplySkin — apply a specific skin to an econ item
// ---------------------------------------------------------------
void F::INVENTORY::ApplySkin(/* per-weapon config */)
{
	// CEconItem-based skin application via inventory manipulation
	//
	// Flow:
	//   1. Get local player inventory: CCSPlayerInventory::Get()
	//   2. Create a new CEconItem: CEconItem::Create()
	//   3. Set item fields:
	//      - m_unDefIndex = weapon definition index
	//      - m_unAccountID = local steam account ID
	//      - m_ulID = unique item ID
	//      - m_nRarity = desired rarity
	//      - SetPaintKit(paintKitId)
	//      - SetPaintSeed(seed)
	//      - SetPaintWear(wear)
	//      - SetStatTrak(count) if desired
	//   4. Add to inventory: inventory->AddEconItem(pItem)
	//   5. Equip in loadout: CCSInventoryManager::Get()->EquipItemInLoadout(team, slot, itemID)
	//   6. Force update: ForceFullUpdate()

	L_PRINT(LOG_INFO) << _XS("[INVENTORY] ApplySkin called");
}

// ---------------------------------------------------------------
// ForceFullUpdate — re-request full entity state from server
// ---------------------------------------------------------------
void F::INVENTORY::ForceFullUpdate()
{
	// Force the game client to re-request full entity updates
	// This is necessary after modifying inventory items so the
	// server re-sends the item data.
	//
	// Common approaches:
	//   1. Set INetworkClientService sequence delta to force full update
	//   2. Call the game's internal ForceFullUpdate function via pattern

	L_PRINT(LOG_INFO) << _XS("[INVENTORY] force full update requested");
}
