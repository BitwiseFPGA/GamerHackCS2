#include "itemdefinition.h"
#include "../const.h"

// ---------------------------------------------------------------
// weapon name table
// ---------------------------------------------------------------
const WeaponName_t g_WeaponNames[] =
{
	{ WEAPON_DEAGLE,        "weapon_deagle",          "Desert Eagle" },
	{ WEAPON_ELITE,         "weapon_elite",           "Dual Berettas" },
	{ WEAPON_FIVESEVEN,     "weapon_fiveseven",       "Five-SeveN" },
	{ WEAPON_GLOCK,         "weapon_glock",           "Glock-18" },
	{ WEAPON_AK47,          "weapon_ak47",            "AK-47" },
	{ WEAPON_AUG,           "weapon_aug",             "AUG" },
	{ WEAPON_AWP,           "weapon_awp",             "AWP" },
	{ WEAPON_FAMAS,         "weapon_famas",           "FAMAS" },
	{ WEAPON_G3SG1,         "weapon_g3sg1",           "G3SG1" },
	{ WEAPON_GALILAR,       "weapon_galilar",         "Galil AR" },
	{ WEAPON_M249,          "weapon_m249",            "M249" },
	{ WEAPON_M4A4,          "weapon_m4a1",            "M4A4" },
	{ WEAPON_MAC10,         "weapon_mac10",           "MAC-10" },
	{ WEAPON_P90,           "weapon_p90",             "P90" },
	{ WEAPON_REPULSOR,      "weapon_zone_repulsor",   "Zone Repulsor" },
	{ WEAPON_MP5SD,         "weapon_mp5sd",           "MP5-SD" },
	{ WEAPON_UMP45,         "weapon_ump45",           "UMP-45" },
	{ WEAPON_XM1014,        "weapon_xm1014",          "XM1014" },
	{ WEAPON_BIZON,         "weapon_bizon",           "PP-Bizon" },
	{ WEAPON_MAG7,          "weapon_mag7",            "MAG-7" },
	{ WEAPON_NEGEV,         "weapon_negev",           "Negev" },
	{ WEAPON_SAWEDOFF,      "weapon_sawedoff",        "Sawed-Off" },
	{ WEAPON_TEC9,          "weapon_tec9",            "Tec-9" },
	{ WEAPON_TASER,         "weapon_taser",           "Zeus x27" },
	{ WEAPON_HKP2000,       "weapon_hkp2000",         "P2000" },
	{ WEAPON_MP7,           "weapon_mp7",             "MP7" },
	{ WEAPON_MP9,           "weapon_mp9",             "MP9" },
	{ WEAPON_NOVA,          "weapon_nova",            "Nova" },
	{ WEAPON_P250,          "weapon_p250",            "P250" },
	{ WEAPON_SCAR20,        "weapon_scar20",          "SCAR-20" },
	{ WEAPON_SG556,         "weapon_sg556",           "SG 553" },
	{ WEAPON_SSG08,         "weapon_ssg08",           "SSG 08" },
	{ WEAPON_M4A1S,         "weapon_m4a1_silencer",   "M4A1-S" },
	{ WEAPON_USPS,          "weapon_usp_silencer",    "USP-S" },
	{ WEAPON_CZ75A,         "weapon_cz75a",           "CZ75-Auto" },
	{ WEAPON_REVOLVER,      "weapon_revolver",        "R8 Revolver" },
	{ WEAPON_KNIFE_CT,      "weapon_knife",           "Default CT Knife" },
	{ WEAPON_KNIFE_T,       "weapon_knife_t",         "Default T Knife" },
	{ WEAPON_FLASHBANG,     "weapon_flashbang",       "Flashbang" },
	{ WEAPON_HEGRENADE,     "weapon_hegrenade",       "HE Grenade" },
	{ WEAPON_SMOKEGRENADE,  "weapon_smokegrenade",    "Smoke Grenade" },
	{ WEAPON_MOLOTOV,       "weapon_molotov",         "Molotov" },
	{ WEAPON_DECOY,         "weapon_decoy",           "Decoy" },
	{ WEAPON_INCGRENADE,    "weapon_incgrenade",      "Incendiary Grenade" },
	{ WEAPON_C4,            "weapon_c4",              "C4 Explosive" },
	{ WEAPON_MEDISHOT,      "weapon_healthshot",      "Healthshot" },
	{ WEAPON_KNIFE_GG,      "weapon_knife_gg",        "Golden Knife" },
};

const int g_nWeaponNameCount = sizeof(g_WeaponNames) / sizeof(g_WeaponNames[0]);

// ---------------------------------------------------------------
// knife name table
// ---------------------------------------------------------------
const KnifeName_t g_KnifeNames[] =
{
	{ WEAPON_KNIFE_BAYONET,        "weapon_bayonet",             "Bayonet",          "knife_bayonet" },
	{ WEAPON_KNIFE_CLASSIC,        "weapon_knife_css",           "Classic Knife",    "knife_css" },
	{ WEAPON_KNIFE_FLIP,           "weapon_knife_flip",          "Flip Knife",       "knife_flip" },
	{ WEAPON_KNIFE_GUT,            "weapon_knife_gut",           "Gut Knife",        "knife_gut" },
	{ WEAPON_KNIFE_KARAMBIT,       "weapon_knife_karambit",      "Karambit",         "knife_karambit" },
	{ WEAPON_KNIFE_M9_BAYONET,     "weapon_knife_m9_bayonet",    "M9 Bayonet",       "knife_m9_bayonet" },
	{ WEAPON_KNIFE_HUNTSMAN,       "weapon_knife_tactical",      "Huntsman Knife",   "knife_tactical" },
	{ WEAPON_KNIFE_FALCHION,       "weapon_knife_falchion",      "Falchion Knife",   "knife_falchion" },
	{ WEAPON_KNIFE_BOWIE,          "weapon_knife_survival_bowie","Bowie Knife",      "knife_survival_bowie" },
	{ WEAPON_KNIFE_BUTTERFLY,      "weapon_knife_butterfly",     "Butterfly Knife",  "knife_butterfly" },
	{ WEAPON_KNIFE_SHADOW_DAGGERS, "weapon_knife_push",          "Shadow Daggers",   "knife_push" },
	{ WEAPON_KNIFE_PARACORD,       "weapon_knife_cord",          "Paracord Knife",   "knife_cord" },
	{ WEAPON_KNIFE_SURVIVAL,       "weapon_knife_canis",         "Survival Knife",   "knife_canis" },
	{ WEAPON_KNIFE_URSUS,          "weapon_knife_ursus",         "Ursus Knife",      "knife_ursus" },
	{ WEAPON_KNIFE_NAVAJA,         "weapon_knife_gypsy_jackknife","Navaja Knife",    "knife_gypsy_jackknife" },
	{ WEAPON_KNIFE_NOMAD,          "weapon_knife_outdoor",       "Nomad Knife",      "knife_outdoor" },
	{ WEAPON_KNIFE_STILETTO,       "weapon_knife_stiletto",      "Stiletto Knife",   "knife_stiletto" },
	{ WEAPON_KNIFE_TALON,          "weapon_knife_widowmaker",    "Talon Knife",      "knife_widowmaker" },
	{ WEAPON_KNIFE_SKELETON,       "weapon_knife_skeleton",      "Skeleton Knife",   "knife_skeleton" },
};

const int g_nKnifeNameCount = sizeof(g_KnifeNames) / sizeof(g_KnifeNames[0]);

// ---------------------------------------------------------------
// glove name table
// ---------------------------------------------------------------
const GloveName_t g_GloveNames[] =
{
	{ GLOVE_STUDDED_BLOODHOUND,  "studded_bloodhound_gloves",  "Bloodhound Gloves" },
	{ GLOVE_T,                   "t_gloves",                   "Default T Gloves" },
	{ GLOVE_CT,                  "ct_gloves",                  "Default CT Gloves" },
	{ GLOVE_SPORTY,              "sporty_gloves",              "Sport Gloves" },
	{ GLOVE_SLICK,               "slick_gloves",               "Driver Gloves" },
	{ GLOVE_LEATHER_HANDWRAPS,   "leather_handwraps",          "Hand Wraps" },
	{ GLOVE_MOTORCYCLE,          "motorcycle_gloves",          "Moto Gloves" },
	{ GLOVE_SPECIALIST,          "specialist_gloves",          "Specialist Gloves" },
	{ GLOVE_STUDDED_HYDRA,       "studded_hydra_gloves",       "Hydra Gloves" },
};

const int g_nGloveNameCount = sizeof(g_GloveNames) / sizeof(g_GloveNames[0]);

// ---------------------------------------------------------------
// lookup implementations
// ---------------------------------------------------------------
const char* GetWeaponDescFromDefinitionIndex(int nDefIndex)
{
	for (int i = 0; i < g_nWeaponNameCount; ++i)
	{
		if (g_WeaponNames[i].nDefinitionIndex == nDefIndex)
			return g_WeaponNames[i].szDesc;
	}
	return "Unknown";
}

const char* GetGloveDescFromDefinitionIndex(int nDefIndex)
{
	for (int i = 0; i < g_nGloveNameCount; ++i)
	{
		if (g_GloveNames[i].nDefinitionIndex == nDefIndex)
			return g_GloveNames[i].szDesc;
	}
	return "Unknown";
}

const char* GetKnifeDescFromDefinitionIndex(int nDefIndex)
{
	for (int i = 0; i < g_nKnifeNameCount; ++i)
	{
		if (g_KnifeNames[i].nDefinitionIndex == nDefIndex)
			return g_KnifeNames[i].szDesc;
	}
	return "Unknown";
}

const char* GetKnifeIconNameFromDefinitionIndex(int nDefIndex)
{
	for (int i = 0; i < g_nKnifeNameCount; ++i)
	{
		if (g_KnifeNames[i].nDefinitionIndex == nDefIndex)
			return g_KnifeNames[i].szIconName;
	}
	return nullptr;
}
