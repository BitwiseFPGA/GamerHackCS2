#pragma once
#include <cstdint>

// ---------------------------------------------------------------
// game constants
// ---------------------------------------------------------------
inline constexpr int MAX_PLAYERS            = 64;
inline constexpr int MAX_NETWORKED_ENTITIES = 16384;
inline constexpr int MAX_ENTITY_ENTRIES     = 0x7FFE; // 32766

// ---------------------------------------------------------------
// frame stages
// ---------------------------------------------------------------
enum EClientFrameStage : int
{
	FRAME_UNDEFINED = -1,
	FRAME_START = 0,
	FRAME_NET_UPDATE_START,
	FRAME_NET_UPDATE_END,
	FRAME_NET_UPDATE_POSTDATAUPDATE_START,
	FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	FRAME_RENDER_START,
	FRAME_RENDER_END,
	FRAME_NET_FULL_FRAME_UPDATE_ON_REMOVE
};

// ---------------------------------------------------------------
// life states
// ---------------------------------------------------------------
enum ELifeState : int
{
	LIFE_ALIVE = 0,
	LIFE_DYING,
	LIFE_DEAD,
	LIFE_RESPAWNABLE,
	LIFE_DISCARDBODY
};

// ---------------------------------------------------------------
// team IDs
// ---------------------------------------------------------------
enum ETeamID : int
{
	TEAM_NONE = 0,
	TEAM_SPECTATOR,
	TEAM_TT,
	TEAM_CT
};

// ---------------------------------------------------------------
// entity flags
// ---------------------------------------------------------------
enum EFlags : int
{
	FL_ONGROUND                = (1 << 0),
	FL_DUCKING                 = (1 << 1),
	FL_ANIMDUCKING             = (1 << 2),
	FL_WATERJUMP               = (1 << 3),
	FL_ONTRAIN                 = (1 << 4),
	FL_INRAIN                  = (1 << 5),
	FL_FROZEN                  = (1 << 6),
	FL_ATCONTROLS              = (1 << 7),
	FL_CLIENT                  = (1 << 8),
	FL_FAKECLIENT              = (1 << 9),
	FL_INWATER                 = (1 << 10),
	FL_FLY                     = (1 << 11),
	FL_SWIM                    = (1 << 12),
	FL_CONVEYOR                = (1 << 13),
	FL_NPC                     = (1 << 14),
	FL_GODMODE                 = (1 << 15),
	FL_NOTARGET                = (1 << 16),
	FL_AIMTARGET               = (1 << 17),
	FL_PARTIALGROUND           = (1 << 18),
	FL_STATICPROP              = (1 << 19),
	FL_GRAPHED                 = (1 << 20),
	FL_GRENADE                 = (1 << 21),
	FL_STEPMOVEMENT            = (1 << 22),
	FL_DONTTOUCH               = (1 << 23),
	FL_BASEVELOCITY            = (1 << 24),
	FL_WORLDBRUSH              = (1 << 25),
	FL_OBJECT                  = (1 << 26),
	FL_KILLME                  = (1 << 27),
	FL_ONFIRE                  = (1 << 28),
	FL_DISSOLVING              = (1 << 29),
	FL_TRANSRAGDOLL            = (1 << 30),
	FL_UNBLOCKABLE_BY_PLAYER   = (1 << 31)
};

// ---------------------------------------------------------------
// extended entity flags
// ---------------------------------------------------------------
enum EEFlags : int
{
	EFL_KILLME                         = (1 << 0),
	EFL_DORMANT                        = (1 << 1),
	EFL_NOCLIP_ACTIVE                  = (1 << 2),
	EFL_SETTING_UP_BONES               = (1 << 3),
	EFL_KEEP_ON_RECREATE_ENTITIES      = (1 << 4),
	EFL_DIRTY_SHADOWUPDATE             = (1 << 5),
	EFL_NOTIFY                         = (1 << 6),
	EFL_FORCE_CHECK_TRANSMIT           = (1 << 7),
	EFL_BOT_FROZEN                     = (1 << 8),
	EFL_SERVER_ONLY                    = (1 << 9),
	EFL_NO_AUTO_EDICT_ATTACH           = (1 << 10),
	EFL_DIRTY_ABSTRANSFORM             = (1 << 11),
	EFL_DIRTY_ABSVELOCITY              = (1 << 12),
	EFL_DIRTY_ABSANGVELOCITY           = (1 << 13),
	EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS = (1 << 14),
	EFL_DIRTY_SPATIAL_PARTITION        = (1 << 15),
	EFL_HAS_PLAYER_CHILD               = (1 << 16),
	EFL_IN_SKYBOX                      = (1 << 17),
	EFL_USE_PARTITION_WHEN_NOT_SOLID   = (1 << 18),
	EFL_TOUCHING_FLUID                 = (1 << 19),
	EFL_IS_BEING_LIFTED_BY_BARNACLE   = (1 << 20),
	EFL_NO_ROTORWASH_PUSH             = (1 << 21),
	EFL_NO_THINK_FUNCTION             = (1 << 22),
	EFL_NO_GAME_PHYSICS_SIMULATION    = (1 << 23),
	EFL_CHECK_UNTOUCH                 = (1 << 24),
	EFL_DONTBLOCKLOS                  = (1 << 25),
	EFL_DONTWALKON                    = (1 << 26),
	EFL_NO_DISSOLVE                   = (1 << 27),
	EFL_NO_MEGAPHYSCANNON_RAGDOLL     = (1 << 28),
	EFL_NO_WATER_VELOCITY_CHANGE      = (1 << 29),
	EFL_NO_PHYSCANNON_INTERACTION     = (1 << 30),
	EFL_NO_DAMAGE_FORCES              = (1 << 31)
};

// ---------------------------------------------------------------
// move types
// ---------------------------------------------------------------
enum EMoveType : std::uint8_t
{
	MOVETYPE_NONE = 0,
	MOVETYPE_OBSOLETE,
	MOVETYPE_WALK,
	MOVETYPE_FLY,
	MOVETYPE_FLYGRAVITY,
	MOVETYPE_VPHYSICS,
	MOVETYPE_PUSH,
	MOVETYPE_NOCLIP,
	MOVETYPE_OBSERVER,
	MOVETYPE_LADDER,
	MOVETYPE_CUSTOM,
	MOVETYPE_LAST,
	MOVETYPE_INVALID,
	MOVETYPE_MAX_BITS = 5
};

// ---------------------------------------------------------------
// water level
// ---------------------------------------------------------------
enum EWaterLevel : int
{
	WL_NOTINWATER = 0,
	WL_FEET,
	WL_WAIST,
	WL_EYES
};

// ---------------------------------------------------------------
// game types / modes
// ---------------------------------------------------------------
enum EGameType : int
{
	GAMETYPE_UNKNOWN = -1,
	GAMETYPE_CLASSIC = 0,
	GAMETYPE_GUNGAME,
	GAMETYPE_TRAINING,
	GAMETYPE_CUSTOM,
	GAMETYPE_COOPERATIVE,
	GAMETYPE_SKIRMISH,
	GAMETYPE_FREEFORALL
};

enum EGameMode : int
{
	GAMEMODE_UNKNOWN = -1,

	// GAMETYPE_CLASSIC
	GAMEMODE_CLASSIC_CASUAL = 0,
	GAMEMODE_CLASSIC_COMPETITIVE,
	GAMEMODE_CLASSIC_SCRIM_COMPETITIVE2V2,
	GAMEMODE_CLASSIC_SCRIM_COMPETITIVE5V5,

	// GAMETYPE_GUNGAME
	GAMEMODE_GUNGAME_PROGRESSIVE = 0,
	GAMEMODE_GUNGAME_BOMB,
	GAMEMODE_GUNGAME_DEATHMATCH,

	// GAMETYPE_TRAINING
	GAMEMODE_TRAINING_DEFAULT = 0,

	// GAMETYPE_CUSTOM
	GAMEMODE_CUSTOM_DEFAULT = 0,

	// GAMETYPE_COOPERATIVE
	GAMEMODE_COOPERATIVE_DEFAULT = 0,
	GAMEMODE_COOPERATIVE_MISSION,

	// GAMETYPE_SKIRMISH
	GAMEMODE_SKIRMISH_DEFAULT = 0,

	// GAMETYPE_FREEFORALL
	GAMEMODE_FREEFORALL_SURVIVAL = 0
};

// ---------------------------------------------------------------
// weapon definition indexes
// ---------------------------------------------------------------
using ItemDefinitionIndex_t = std::uint16_t;

enum EItemDefinitionIndex : ItemDefinitionIndex_t
{
	WEAPON_NONE = 0,
	WEAPON_DEAGLE = 1,
	WEAPON_ELITE = 2,
	WEAPON_FIVESEVEN = 3,
	WEAPON_GLOCK = 4,
	WEAPON_AK47 = 7,
	WEAPON_AUG = 8,
	WEAPON_AWP = 9,
	WEAPON_FAMAS = 10,
	WEAPON_G3SG1 = 11,
	WEAPON_GALILAR = 13,
	WEAPON_M249 = 14,
	WEAPON_M4A4 = 16,
	WEAPON_MAC10 = 17,
	WEAPON_P90 = 19,
	WEAPON_REPULSOR = 20,
	WEAPON_MP5SD = 23,
	WEAPON_UMP45 = 24,
	WEAPON_XM1014 = 25,
	WEAPON_BIZON = 26,
	WEAPON_MAG7 = 27,
	WEAPON_NEGEV = 28,
	WEAPON_SAWEDOFF = 29,
	WEAPON_TEC9 = 30,
	WEAPON_TASER = 31,
	WEAPON_HKP2000 = 32,
	WEAPON_MP7 = 33,
	WEAPON_MP9 = 34,
	WEAPON_NOVA = 35,
	WEAPON_P250 = 36,
	WEAPON_SHIELD = 37,
	WEAPON_SCAR20 = 38,
	WEAPON_SG556 = 39,
	WEAPON_SSG08 = 40,
	WEAPON_KNIFE_CT = 41,
	WEAPON_KNIFE_T = 42,
	WEAPON_FLASHBANG = 43,
	WEAPON_HEGRENADE = 44,
	WEAPON_SMOKEGRENADE = 45,
	WEAPON_MOLOTOV = 46,
	WEAPON_DECOY = 47,
	WEAPON_INCGRENADE = 48,
	WEAPON_C4 = 49,
	WEAPON_KEVLAR = 50,
	WEAPON_ASSAULTSUIT = 51,
	WEAPON_HEAVYASSAULTSUIT = 52,
	WEAPON_DEFUSER = 55,
	WEAPON_RESCUEKIT = 56,
	WEAPON_MEDISHOT = 57,
	WEAPON_MUSICKIT = 58,
	WEAPON_KNIFE_GG = 59,
	WEAPON_M4A1S = 60,
	WEAPON_USPS = 61,
	WEAPON_TRADEUP_CONTRACT = 62,
	WEAPON_CZ75A = 63,
	WEAPON_REVOLVER = 64,
	WEAPON_TAGRENADE = 68,
	WEAPON_FISTS = 69,
	WEAPON_BREACHCHARGE = 70,
	WEAPON_TABLET = 72,
	WEAPON_MELEE = 74,
	WEAPON_AXE = 75,
	WEAPON_HAMMER = 76,
	WEAPON_WRENCH = 78,
	WEAPON_SPECTRAL_SHIV = 80,
	WEAPON_FIREBOMB = 81,
	WEAPON_DIVERSION = 82,
	WEAPON_FRAG_GRENADE = 83,
	WEAPON_SNOWBALL = 84,
	WEAPON_BUMPMINE = 85,

	// knives (skins)
	WEAPON_KNIFE_BAYONET = 500,
	WEAPON_KNIFE_CLASSIC = 503,
	WEAPON_KNIFE_FLIP = 505,
	WEAPON_KNIFE_GUT = 506,
	WEAPON_KNIFE_KARAMBIT = 507,
	WEAPON_KNIFE_M9_BAYONET = 508,
	WEAPON_KNIFE_HUNTSMAN = 509,
	WEAPON_KNIFE_FALCHION = 512,
	WEAPON_KNIFE_BOWIE = 514,
	WEAPON_KNIFE_BUTTERFLY = 515,
	WEAPON_KNIFE_SHADOW_DAGGERS = 516,
	WEAPON_KNIFE_PARACORD = 517,
	WEAPON_KNIFE_SURVIVAL = 518,
	WEAPON_KNIFE_URSUS = 519,
	WEAPON_KNIFE_NAVAJA = 520,
	WEAPON_KNIFE_NOMAD = 521,
	WEAPON_KNIFE_STILETTO = 522,
	WEAPON_KNIFE_TALON = 523,
	WEAPON_KNIFE_SKELETON = 525,

	// gloves
	GLOVE_STUDDED_BLOODHOUND = 5027,
	GLOVE_T = 5028,
	GLOVE_CT = 5029,
	GLOVE_SPORTY = 5030,
	GLOVE_SLICK = 5031,
	GLOVE_LEATHER_HANDWRAPS = 5032,
	GLOVE_MOTORCYCLE = 5033,
	GLOVE_SPECIALIST = 5034,
	GLOVE_STUDDED_HYDRA = 5035
};

// ---------------------------------------------------------------
// weapon types
// ---------------------------------------------------------------
enum EWeaponType : std::uint32_t
{
	WEAPONTYPE_KNIFE = 0,
	WEAPONTYPE_PISTOL,
	WEAPONTYPE_SUBMACHINEGUN,
	WEAPONTYPE_RIFLE,
	WEAPONTYPE_SHOTGUN,
	WEAPONTYPE_SNIPER_RIFLE,
	WEAPONTYPE_MACHINEGUN,
	WEAPONTYPE_C4,
	WEAPONTYPE_TASER,
	WEAPONTYPE_GRENADE,
	WEAPONTYPE_EQUIPMENT,
	WEAPONTYPE_STACKABLEITEM,
	WEAPONTYPE_FISTS,
	WEAPONTYPE_BREACHCHARGE,
	WEAPONTYPE_BUMPMINE,
	WEAPONTYPE_TABLET,
	WEAPONTYPE_MELEE,
	WEAPONTYPE_SHIELD,
	WEAPONTYPE_ZONE_REPULSOR,
	WEAPONTYPE_UNKNOWN
};

// ---------------------------------------------------------------
// hitgroups
// ---------------------------------------------------------------
enum EHitgroup : int
{
	HITGROUP_GENERIC = 0,
	HITGROUP_HEAD,
	HITGROUP_CHEST,
	HITGROUP_STOMACH,
	HITGROUP_LEFTARM,
	HITGROUP_RIGHTARM,
	HITGROUP_LEFTLEG,
	HITGROUP_RIGHTLEG,
	HITGROUP_NECK,
	HITGROUP_GEAR = 10
};

// ---------------------------------------------------------------
// observer modes
// ---------------------------------------------------------------
enum ObserverMode_t : std::uint32_t
{
	OBS_MODE_NONE    = 0x0,
	OBS_MODE_FIXED   = 0x1,
	OBS_MODE_IN_EYE  = 0x2,
	OBS_MODE_CHASE   = 0x3,
	OBS_MODE_ROAMING = 0x4,
	OBS_MODE_DIRECTED = 0x5,
	NUM_OBSERVER_MODES = 0x6,
};

// ---------------------------------------------------------------
// CS weapon type (class enum matching schema)
// ---------------------------------------------------------------
enum class CSWeaponType_t : std::int32_t
{
	WEAPONTYPE_KNIFE = 0,
	WEAPONTYPE_PISTOL = 1,
	WEAPONTYPE_SUBMACHINEGUN = 2,
	WEAPONTYPE_RIFLE = 3,
	WEAPONTYPE_SHOTGUN = 4,
	WEAPONTYPE_SNIPER_RIFLE = 5,
	WEAPONTYPE_MACHINEGUN = 6,
	WEAPONTYPE_C4 = 7,
	WEAPONTYPE_TASER = 8,
	WEAPONTYPE_GRENADE = 9,
	WEAPONTYPE_EQUIPMENT = 10,
	WEAPONTYPE_STACKABLEITEM = 11,
	WEAPONTYPE_FISTS = 12,
	WEAPONTYPE_BREACHCHARGE = 13,
	WEAPONTYPE_BUMPMINE = 14,
	WEAPONTYPE_TABLET = 15,
	WEAPONTYPE_MELEE = 16,
	WEAPONTYPE_SHIELD = 17,
	WEAPONTYPE_ZONE_REPULSOR = 18,
	WEAPONTYPE_UNKNOWN = 19
};

// ---------------------------------------------------------------
// bone flags
// ---------------------------------------------------------------
enum EBoneFlags : std::uint32_t
{
	FLAG_NO_BONE_FLAGS             = 0x0,
	FLAG_BONEFLEXDRIVER            = 0x4,
	FLAG_CLOTH                     = 0x8,
	FLAG_PHYSICS                   = 0x10,
	FLAG_ATTACHMENT                = 0x20,
	FLAG_ANIMATION                 = 0x40,
	FLAG_MESH                      = 0x80,
	FLAG_HITBOX                    = 0x100,
	FLAG_BONE_USED_BY_VERTEX_LOD0  = 0x400,
	FLAG_BONE_USED_BY_VERTEX_LOD1  = 0x800,
	FLAG_BONE_USED_BY_VERTEX_LOD2  = 0x1000,
	FLAG_BONE_USED_BY_VERTEX_LOD3  = 0x2000,
	FLAG_BONE_USED_BY_VERTEX_LOD4  = 0x4000,
	FLAG_BONE_USED_BY_VERTEX_LOD5  = 0x8000,
	FLAG_BONE_USED_BY_VERTEX_LOD6  = 0x10000,
	FLAG_BONE_USED_BY_VERTEX_LOD7  = 0x20000,
	FLAG_BONE_MERGE_READ           = 0x40000,
	FLAG_BONE_MERGE_WRITE          = 0x80000,
	FLAG_ALL_BONE_FLAGS            = 0xFFFFF,
	BLEND_PREALIGNED               = 0x100000,
	FLAG_RIGIDLENGTH               = 0x200000,
	FLAG_PROCEDURAL                = 0x400000,
};

// ---------------------------------------------------------------
// network channel buffer types
// ---------------------------------------------------------------
enum NetChannelBufType_t : std::uint8_t
{
	BUF_DEFAULT    = 0xFF,
	BUF_UNRELIABLE = 0x0,
	BUF_RELIABLE   = 0x01,
	BUF_VOICE      = 0x02,
};

// ---------------------------------------------------------------
// input buttons (uint64_t bitmask matching Source 2 input)
// @note: comprehensive input button enums are in datatypes/usercmd.h
//        (ECommandButtons, CInButtonState). The constants below are
//        standalone convenience aliases that don't conflict.
// ---------------------------------------------------------------
