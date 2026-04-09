#pragma once
#include <cstdint>

#include "../datatypes/utlmap.h"

// ---------------------------------------------------------------
// CPaintKit — paint kit / skin definition
// ---------------------------------------------------------------
struct CPaintKit
{
	int nID;
	const char* szName;
	const char* szDescriptionString;
	const char* szDescriptionTag;

private:
	std::uint8_t _pad0[0x20]; // internal fields
public:
	int nRarity;
};

// ---------------------------------------------------------------
// CMusicKit — music kit definition
// ---------------------------------------------------------------
struct CMusicKit
{
	std::int32_t nID;
	std::int32_t unk0;
	const char* szName;
	const char* szNameLocToken;
	const char* szLocDescription;
	const char* szPedestalDisplayModel;
	const char* szInventoryImage;

private:
	std::uint8_t _pad0[0x10];
};

// ---------------------------------------------------------------
// CEconItemDefinition — item definition info (forward declared)
// ---------------------------------------------------------------
class CEconItemDefinition;

// ---------------------------------------------------------------
// CEconItemSchema — master item schema, provides access to all
//                   item definitions, paint kits, music kits, etc.
//
// @note: the map accessors require runtime offsets that must be
//        resolved via pattern scanning during initialization.
//        Store them in globals (e.g. g_CEconItemSchema_GetSortedItemDefinitionMap)
//        and pass them here.
// ---------------------------------------------------------------

/// runtime offsets — must be set during init via pattern scanning
inline std::uint32_t g_CEconItemSchema_GetSortedItemDefinitionMap = 0;
inline std::uint32_t g_CEconItemSchema_GetPaintKits               = 0;
inline std::uint32_t g_CEconItemSchema_GetMusicKitDefinitions     = 0;

class CEconItemSchema
{
public:
	/// get attribute definition interface by attribute index
	/// @param nAttribIndex — attribute definition index (e.g. 6 for paint kit)
	/// @returns: opaque pointer to the attribute definition, or nullptr
	void* GetAttributeDefinitionInterface(int nAttribIndex);

	/// get the sorted map of all item definitions (def index -> CEconItemDefinition*)
	[[nodiscard]] CUtlMap<int, CEconItemDefinition*>& GetSortedItemDefinitionMap()
	{
		return *reinterpret_cast<CUtlMap<int, CEconItemDefinition*>*>(
			reinterpret_cast<std::uintptr_t>(this) + g_CEconItemSchema_GetSortedItemDefinitionMap);
	}

	/// get all paint kit definitions (paint kit ID -> CPaintKit*)
	[[nodiscard]] CUtlMap<int, CPaintKit*>& GetPaintKits()
	{
		return *reinterpret_cast<CUtlMap<int, CPaintKit*>*>(
			reinterpret_cast<std::uintptr_t>(this) + g_CEconItemSchema_GetPaintKits);
	}

	/// get all music kit definitions (music kit ID -> CMusicKit*)
	[[nodiscard]] CUtlMap<int, CMusicKit*>& GetMusicKitDefinitions()
	{
		return *reinterpret_cast<CUtlMap<int, CMusicKit*>*>(
			reinterpret_cast<std::uintptr_t>(this) + g_CEconItemSchema_GetMusicKitDefinitions);
	}
};

// ---------------------------------------------------------------
// CEconItemSystem — top-level item system
// ---------------------------------------------------------------
class CEconItemSystem
{
public:
	/// get the global econ item schema
	CEconItemSchema* GetEconItemSchema();
};
