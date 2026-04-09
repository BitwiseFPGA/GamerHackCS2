#pragma once
#include <cstdint>

// ---------------------------------------------------------------
// forward declarations
// ---------------------------------------------------------------
class CSOEconItem;
class CEconItemSchema;
class CEconItemDefinition;

// ---------------------------------------------------------------
// econ item attribute definition indexes
// ---------------------------------------------------------------
enum EEconItemAttribute : std::uint32_t
{
	ATTRIBUTE_PAINT_KIT         = 6,
	ATTRIBUTE_PAINT_SEED        = 7,
	ATTRIBUTE_PAINT_WEAR        = 8,
	ATTRIBUTE_STAT_TRACK        = 80,
	ATTRIBUTE_STAT_TRACK_TYPE   = 81,
	ATTRIBUTE_STICKER_ID        = 113,
	ATTRIBUTE_STICKER_WEAR      = 114,
	ATTRIBUTE_STICKER_SCALE     = 115,
	ATTRIBUTE_STICKER_ROTATION  = 116,
	ATTRIBUTE_STICKER_SLOT_1_ID = 117,
	ATTRIBUTE_STICKER_SLOT_1_WEAR = 118,
	ATTRIBUTE_STICKER_SLOT_1_SCALE = 119,
	ATTRIBUTE_STICKER_SLOT_1_ROT = 120,
	ATTRIBUTE_STICKER_SLOT_2_ID = 121,
	ATTRIBUTE_STICKER_SLOT_2_WEAR = 122,
	ATTRIBUTE_STICKER_SLOT_2_SCALE = 123,
	ATTRIBUTE_STICKER_SLOT_2_ROT = 124,
	ATTRIBUTE_STICKER_SLOT_3_ID = 125,
	ATTRIBUTE_STICKER_SLOT_3_WEAR = 126,
	ATTRIBUTE_STICKER_SLOT_3_SCALE = 127,
	ATTRIBUTE_STICKER_SLOT_3_ROT = 128,
	ATTRIBUTE_STICKER_SLOT_4_ID = 129,
	ATTRIBUTE_STICKER_SLOT_4_WEAR = 130,
	ATTRIBUTE_STICKER_SLOT_4_SCALE = 131,
	ATTRIBUTE_STICKER_SLOT_4_ROT = 132,
	ATTRIBUTE_STICKER_ROTATION_X = 278,
	ATTRIBUTE_STICKER_ROTATION_Y = 279,
	ATTRIBUTE_MUSIC_ID          = 166,
	ATTRIBUTE_KEYCHAIN_SLOT_ID_0 = 399,
};

// ---------------------------------------------------------------
// CEconItemDefinition — static item type data from schema
// ---------------------------------------------------------------
class CEconItemDefinition
{
public:
	[[nodiscard]] bool IsWeapon();
	[[nodiscard]] bool IsKnife(bool bExcludeDefault = false);
	[[nodiscard]] bool IsGlove(bool bExcludeDefault = false);
	[[nodiscard]] bool IsAgent(bool bExcludeDefault = false);

	// offsets into internal data — use with care
	std::uint16_t GetDefIndex() const;
	std::uint8_t  GetItemRarity() const;
	const char*   GetItemBaseName() const;
	const char*   GetItemTypeName() const;
	const char*   GetModelName() const;
	const char*   GetWeaponName() const;
};

// ---------------------------------------------------------------
// CEconItem — in-memory economy item
// ---------------------------------------------------------------
class CEconItem
{
public:
	/// create a new CEconItem via the game's factory
	static CEconItem* Create();

	/// destroy this item (VFunc index 1)
	void Destruct();

	/// serialize item data to protobuf for network
	void* SerializeToProtoBufItem(CSOEconItem* pCSOEconItem);

	// ---------------------------------------------------------------
	// attribute setters — these call SetDynamicAttributeValue internally
	// ---------------------------------------------------------------

	void SetPaintKit(float kit)          { SetDynamicAttributeValue(ATTRIBUTE_PAINT_KIT, &kit); }
	void SetPaintSeed(float seed)        { SetDynamicAttributeValue(ATTRIBUTE_PAINT_SEED, &seed); }
	void SetPaintWear(float wear)        { SetDynamicAttributeValue(ATTRIBUTE_PAINT_WEAR, &wear); }

	void SetStatTrak(int count)
	{
		SetDynamicAttributeValue(ATTRIBUTE_STAT_TRACK, &count);
		int nType = 0;
		SetDynamicAttributeValue(ATTRIBUTE_STAT_TRACK_TYPE, &nType);
	}

	void SetStatTrakType(int type)
	{
		SetDynamicAttributeValue(ATTRIBUTE_STAT_TRACK_TYPE, &type);
	}

	void SetSticker(int slot, int id, float wear = 0.f, float scale = 1.f, float rotation = 0.f)
	{
		if (slot < 0 || slot > 5)
			return;

		SetDynamicAttributeValue(ATTRIBUTE_STICKER_ID + (slot * 4), &id);
		SetDynamicAttributeValue(ATTRIBUTE_STICKER_WEAR + (slot * 4), &wear);
		SetDynamicAttributeValue(ATTRIBUTE_STICKER_SCALE + (slot * 4), &scale);
		SetDynamicAttributeValue(ATTRIBUTE_STICKER_ROTATION + (slot * 4), &rotation);
	}

	void SetSticker(int slot, int id, float wear, float scale, float rotation,
	                float offsetX, float offsetY)
	{
		SetSticker(slot, id, wear, scale, rotation);
		SetDynamicAttributeValue(ATTRIBUTE_STICKER_ROTATION_X + (slot * 2), &offsetX);
		SetDynamicAttributeValue(ATTRIBUTE_STICKER_ROTATION_Y + (slot * 2), &offsetY);
	}

	void SetMusicId(int id)
	{
		SetDynamicAttributeValue(ATTRIBUTE_MUSIC_ID, &id);
	}

	void SetKeychain(int slot, int id)
	{
		SetDynamicAttributeValue(ATTRIBUTE_KEYCHAIN_SLOT_ID_0 + slot, &id);
	}

private:
	/// set a dynamic attribute via the game's attribute system
	/// @param nDefIndex — attribute definition index
	/// @param pValue    — pointer to the value to set
	void SetDynamicAttributeValue(int nDefIndex, void* pValue);

	// ---------------------------------------------------------------
	// memory layout (matches Source 2 in-memory format)
	// ---------------------------------------------------------------
	std::uint8_t _pad0[0x10];                   // 0x0000

public:
	std::uint64_t m_ulID;                       // 0x0010
	std::uint64_t m_ulOriginalID;               // 0x0018
	void* m_pCustomDataOptimizedObject;         // 0x0020
	std::uint32_t m_unAccountID;                // 0x0028
	std::uint32_t m_unInventory;                // 0x002C
	std::uint16_t m_unDefIndex;                 // 0x0030

	// packed bitfield
	std::uint16_t m_unOrigin : 5;               // 0x0032
	std::uint16_t m_nQuality : 4;
	std::uint16_t m_unLevel : 2;
	std::uint16_t m_nRarity : 4;
	std::uint16_t m_dirtybitInUse : 1;

	std::int16_t m_iItemSet;                    // 0x0034
	int m_bSOUpdateFrame;                       // 0x0036
	std::uint8_t m_unFlags;                     // 0x003A
};
