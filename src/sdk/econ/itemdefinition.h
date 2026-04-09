#pragma once
#include <cstdint>

// ---------------------------------------------------------------
// item definition index enum — all CS2 weapon/knife/glove/agent IDs
//
// @note: the primary EItemDefinitionIndex enum is defined in const.h
//        this header provides additional lookup utilities and
//        name/description tables for the skin changer
// ---------------------------------------------------------------

// ---------------------------------------------------------------
// WeaponName_t — weapon definition index to name mapping
// ---------------------------------------------------------------
struct WeaponName_t
{
	int         nDefinitionIndex;
	const char* szName;
	const char* szDesc;
};

// ---------------------------------------------------------------
// KnifeName_t — knife definition index to name mapping
// ---------------------------------------------------------------
struct KnifeName_t
{
	int         nDefinitionIndex;
	const char* szName;
	const char* szDesc;
	const char* szIconName;
};

// ---------------------------------------------------------------
// GloveName_t — glove definition index to name mapping
// ---------------------------------------------------------------
struct GloveName_t
{
	int         nDefinitionIndex;
	const char* szName;
	const char* szDesc;
};

// ---------------------------------------------------------------
// lookup tables (defined in itemdefinition.cpp)
// ---------------------------------------------------------------
extern const WeaponName_t g_WeaponNames[];
extern const int          g_nWeaponNameCount;

extern const KnifeName_t  g_KnifeNames[];
extern const int           g_nKnifeNameCount;

extern const GloveName_t  g_GloveNames[];
extern const int           g_nGloveNameCount;

// ---------------------------------------------------------------
// lookup functions
// ---------------------------------------------------------------

/// find weapon display name from definition index
const char* GetWeaponDescFromDefinitionIndex(int nDefIndex);

/// find glove display name from definition index
const char* GetGloveDescFromDefinitionIndex(int nDefIndex);

/// find knife display name from definition index
const char* GetKnifeDescFromDefinitionIndex(int nDefIndex);

/// find knife icon name from definition index
const char* GetKnifeIconNameFromDefinitionIndex(int nDefIndex);
