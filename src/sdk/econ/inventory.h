#pragma once
#include <cstdint>

#include "../datatypes/utlvector.h"

class CEconItem;
class C_EconItemView;
class CCSPlayerInventory;

// ---------------------------------------------------------------
// CCSPlayerInventory — player's local inventory
// ---------------------------------------------------------------
class CCSPlayerInventory
{
public:
	/// get the static local player inventory instance
	static CCSPlayerInventory* Get();

	/// get the item view for a specific item ID
	/// @param ulItemID — the item's unique ID
	/// @returns: pointer to the networked item view, or nullptr
	C_EconItemView* GetItemViewForItem(std::uint64_t ulItemID);

	/// get the underlying SOC data (CEconItem) for an item
	/// @param ulItemID — the item's unique ID
	/// @returns: pointer to the CEconItem, or nullptr
	CEconItem* GetSOCDataForItem(std::uint64_t ulItemID);

	/// get item view in a specific loadout slot
	/// @param nClass — team/class index
	/// @param nSlot  — loadout slot index
	/// @returns: pointer to the item view, or nullptr
	C_EconItemView* GetItemInLoadout(int nClass, int nSlot);

	/// add a new econ item to the local inventory
	bool AddEconItem(CEconItem* pItem);

	/// remove an econ item from the local inventory
	void RemoveEconItem(CEconItem* pItem);

	/// get the highest item/original IDs in inventory
	/// @param[out] pHighestID — highest item ID
	/// @param[out] pHighestOriginalID — highest original item ID (base items)
	void GetHighestIDs(std::uint64_t* pHighestID, std::uint64_t* pHighestOriginalID);

	/// get the inventory owner's account ID
	[[nodiscard]] std::uint32_t GetOwnerAccountID();

	/// get internal item vector
	[[nodiscard]] CUtlVector<C_EconItemView*>& GetItemVector();
};

// ---------------------------------------------------------------
// CCSInventoryManager — global inventory manager
// ---------------------------------------------------------------
class CCSInventoryManager
{
public:
	/// get the singleton instance
	static CCSInventoryManager* Get();

	/// get the local player's inventory (VFunc index 59)
	CCSPlayerInventory* GetLocalInventory();

	/// equip an item in the loadout
	/// @param nTeam   — team index (2 = T, 3 = CT)
	/// @param nSlot   — loadout slot
	/// @param ulItemID — item unique ID
	bool EquipItemInLoadout(int nTeam, int nSlot, std::uint64_t ulItemID);
};
