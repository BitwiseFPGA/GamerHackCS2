#include "schema.h"
#include "interfaces.h"
#include "patterns.h"
#include "../utilities/xorstr.h"
#include <fstream>

// ============================================================================
// schema system structures (mirroring game's CSchemaSystem layout)
// ============================================================================

class CSchemaSystemTypeScope;

struct SchemaClassFieldData_t
{
	const char* szName;       // 0x00
	void*       pSchemaType;  // 0x08
	std::int32_t nSingleInheritanceOffset; // 0x10
	std::int32_t nMetadataSize;            // 0x14
	void*        pMetadata;   // 0x18
};

struct SchemaClassInfoData_t
{
	void*                   pVtable;          // 0x00
	const char*             szName;           // 0x08
	const char*             szDescription;    // 0x10
	int                     m_nSize;          // 0x18
	std::int16_t            nFieldCount;      // 0x1C
	std::int16_t            nStaticSize;      // 0x1E
	std::int16_t            nMetadataSize;    // 0x20
	std::uint8_t            nAlignOf;         // 0x22
	std::uint8_t            nBaseClassesCount;// 0x23
	std::uint8_t            _pad2[0x04];      // 0x24
	SchemaClassFieldData_t* pFields;          // 0x28
};

class CSchemaSystemTypeScope
{
public:
	const char* GetModuleName()
	{
		return reinterpret_cast<const char*>(reinterpret_cast<std::uintptr_t>(this) + 0x08);
	}

	SchemaClassInfoData_t* FindDeclaredClass(const char* szClassName)
	{
		SchemaClassInfoData_t* pResult = nullptr;
		using Fn = void(__thiscall*)(void*, SchemaClassInfoData_t**, const char*);
		const auto pVTable = *reinterpret_cast<std::uintptr_t**>(this);
		reinterpret_cast<Fn>(pVTable[2])(this, &pResult, szClassName);
		return pResult;
	}
};

namespace SchemaVFuncs
{
	inline constexpr std::size_t FIND_TYPE_SCOPE_FOR_MODULE = PATTERNS::VTABLE::SCHEMA::FIND_TYPE_SCOPE;
}

// ============================================================================
// internal storage
// ============================================================================

// flat combined-hash -> offset (for "ClassName->fieldName" single-hash lookups)
static std::unordered_map<FNV1A_t, std::uint32_t> s_mapFlatOffsets;

// classHash -> (fieldHash -> offset)
static std::unordered_map<FNV1A_t, std::unordered_map<FNV1A_t, std::uint32_t>> s_mapSchemaOffsets;

struct DebugFieldInfo_t
{
	std::string szClassName;
	std::string szFieldName;
	std::uint32_t nOffset;
};
static std::vector<DebugFieldInfo_t> s_vecDebugFields;

static std::size_t s_nTotalClasses = 0;
static std::size_t s_nTotalFields  = 0;

// ============================================================================
// HARDCODED FALLBACK OFFSETS — from a2x/cs2-dumper
// Used when runtime schema dump fails. Must be updated per game build.
// ============================================================================
struct FallbackOffset_t
{
	const char* szCombined;  // "ClassName->fieldName"
	std::uint32_t nOffset;
};

static constexpr FallbackOffset_t s_arrFallbackOffsets[] =
{
	// CEntityInstance
	{"CEntityInstance->m_pEntity", 0x10},

	// CEntityIdentity
	{"CEntityIdentity->m_name", 0x18},
	{"CEntityIdentity->m_designerName", 0x20},
	{"CEntityIdentity->m_flags", 0x30},

	// CGameSceneNode
	{"CGameSceneNode->m_pOwner", 0x30},
	{"CGameSceneNode->m_pChild", 0x40},
	{"CGameSceneNode->m_pNextSibling", 0x48},
	{"CGameSceneNode->m_angRotation", 0xC0},
	{"CGameSceneNode->m_vecAbsOrigin", 0xD0},
	{"CGameSceneNode->m_angAbsRotation", 0xDC},
	{"CGameSceneNode->m_bDormant", 0x10B},
	{"CGameSceneNode->m_vRenderOrigin", 0x134},

	// CCollisionProperty
	{"CCollisionProperty->m_vecMins", 0x40},
	{"CCollisionProperty->m_vecMaxs", 0x4C},
	{"CCollisionProperty->m_usSolidFlags", 0x5A},
	{"CCollisionProperty->m_CollisionGroup", 0x5E},

	// CGlowProperty
	{"CGlowProperty->m_glowColorOverride", 0x40},
	{"CGlowProperty->m_bGlowing", 0x51},

	// CModelState
	{"CModelState->m_hModel", 0xA0},
	{"CModelState->m_ModelName", 0xA8},

	// CSkeletonInstance
	{"CSkeletonInstance->m_modelState", 0x160},
	{"CSkeletonInstance->m_nHitboxSet", 0x438},

	// EntitySpottedState_t
	{"EntitySpottedState_t->m_bSpotted", 0x8},

	// C_BaseEntity
	{"C_BaseEntity->m_pGameSceneNode", 0x338},
	{"C_BaseEntity->m_pCollision", 0x348},
	{"C_BaseEntity->m_iMaxHealth", 0x350},
	{"C_BaseEntity->m_iHealth", 0x354},
	{"C_BaseEntity->m_lifeState", 0x35C},
	{"C_BaseEntity->m_iEFlags", 0x37C},
	{"C_BaseEntity->m_nSubclassID", 0x388},
	{"C_BaseEntity->m_iTeamNum", 0x3F3},
	{"C_BaseEntity->m_fFlags", 0x400},
	{"C_BaseEntity->m_vecAbsVelocity", 0x404},
	{"C_BaseEntity->m_vecBaseVelocity", 0x518},
	{"C_BaseEntity->m_hOwnerEntity", 0x528},
	{"C_BaseEntity->m_nActualMoveType", 0x52E},
	{"C_BaseEntity->m_flWaterLevel", 0x530},

	// C_BaseModelEntity
	{"C_BaseModelEntity->m_Collision", 0xC10},
	{"C_BaseModelEntity->m_Glow", 0xCC0},
	{"C_BaseModelEntity->m_vecViewOffset", 0xD58},

	// CBasePlayerController
	{"CBasePlayerController->m_nTickBase", 0x6C0},
	{"CBasePlayerController->m_hPawn", 0x6C4},
	{"CBasePlayerController->m_steamID", 0x780},
	{"CBasePlayerController->m_bIsLocalPlayerController", 0x788},

	// CCSPlayerController
	{"CCSPlayerController->m_pInGameMoneyServices", 0x808},
	{"CCSPlayerController->m_pInventoryServices", 0x810},
	{"CCSPlayerController->m_iPing", 0x828},
	{"CCSPlayerController->m_sSanitizedPlayerName", 0x860},
	{"CCSPlayerController->m_hPlayerPawn", 0x90C},
	{"CCSPlayerController->m_bPawnIsAlive", 0x914},
	{"CCSPlayerController->m_iPawnHealth", 0x918},
	{"CCSPlayerController->m_iPawnArmor", 0x91C},
	{"CCSPlayerController->m_bPawnHasDefuser", 0x920},
	{"CCSPlayerController->m_bPawnHasHelmet", 0x921},

	// CCSPlayerController services
	{"CCSPlayerController_InGameMoneyServices->m_iAccount", 0x40},
	{"CCSPlayerController_InventoryServices->m_unMusicID", 0x58},

	// CPlayer_WeaponServices
	{"CPlayer_WeaponServices->m_hMyWeapons", 0x48},
	{"CPlayer_WeaponServices->m_hActiveWeapon", 0x60},

	// CCSPlayer_WeaponServices
	{"CCSPlayer_WeaponServices->m_flNextAttack", 0xD0},

	// CCSPlayer_ItemServices
	{"CCSPlayer_ItemServices->m_bHasDefuser", 0x48},
	{"CCSPlayer_ItemServices->m_bHasHelmet", 0x49},

	// CPlayer_ObserverServices
	{"CPlayer_ObserverServices->m_iObserverMode", 0x48},
	{"CPlayer_ObserverServices->m_hObserverTarget", 0x4C},

	// CPlayer_CameraServices
	{"CPlayer_CameraServices->m_vecCsViewPunchAngle", 0x48},
	{"CPlayer_CameraServices->m_hActivePostProcessingVolume", 0x1FC},

	// CCSPlayerBase_CameraServices
	{"CCSPlayerBase_CameraServices->m_iFOV", 0x290},

	// C_BasePlayerPawn
	{"C_BasePlayerPawn->m_pWeaponServices", 0x13D8},
	{"C_BasePlayerPawn->m_pItemServices", 0x13E0},
	{"C_BasePlayerPawn->m_pObserverServices", 0x13F0},
	{"C_BasePlayerPawn->m_pCameraServices", 0x1410},
	{"C_BasePlayerPawn->m_vOldOrigin", 0x1588},
	{"C_BasePlayerPawn->m_hController", 0x15A0},

	// C_CSPlayerPawnBase
	{"C_CSPlayerPawnBase->m_flLastSpawnTimeIndex", 0x15D4},
	{"C_CSPlayerPawnBase->m_flFlashBangTime", 0x15E4},
	{"C_CSPlayerPawnBase->m_flFlashMaxAlpha", 0x15F4},
	{"C_CSPlayerPawnBase->m_flFlashDuration", 0x15F8},
	{"C_CSPlayerPawnBase->m_vLastSmokeOverlayColor", 0x1620},

	// C_CSPlayerPawn
	{"C_CSPlayerPawn->m_aimPunchAngle", 0x16CC},
	{"C_CSPlayerPawn->m_bNeedToReApplyGloves", 0x188D},
	{"C_CSPlayerPawn->m_EconGloves", 0x1890},
	{"C_CSPlayerPawn->m_hHudModelArms", 0x2400},
	{"C_CSPlayerPawn->m_entitySpottedState", 0x26E0},
	{"C_CSPlayerPawn->m_bIsScoped", 0x26F8},
	{"C_CSPlayerPawn->m_bIsDefusing", 0x26FA},
	{"C_CSPlayerPawn->m_bIsGrabbingHostage", 0x26FB},
	{"C_CSPlayerPawn->m_iShotsFired", 0x270C},
	{"C_CSPlayerPawn->m_bWaitForNoAttack", 0x2720},
	{"C_CSPlayerPawn->m_ArmorValue", 0x272C},
	{"C_CSPlayerPawn->m_bGunGameImmunity", 0x3D74},
	{"C_CSPlayerPawn->m_angEyeAngles", 0x3DD0},
	{"C_CSPlayerPawn->m_nSurvivalTeam", 0x26E0}, // approximate

	// C_EconItemView
	{"C_EconItemView->m_iItemDefinitionIndex", 0x1BA},
	{"C_EconItemView->m_iItemID", 0x1C8},
	{"C_EconItemView->m_iItemIDHigh", 0x1D0},
	{"C_EconItemView->m_iItemIDLow", 0x1D4},
	{"C_EconItemView->m_iAccountID", 0x1D8},
	{"C_EconItemView->m_bInitialized", 0x1E8},
	{"C_EconItemView->m_bDisallowSOC", 0x1E9},
	{"C_EconItemView->m_szCustomName", 0x2F8},
	{"C_EconItemView->m_szCustomNameOverride", 0x399},

	// C_AttributeContainer
	{"C_AttributeContainer->m_Item", 0x50},

	// C_EconEntity
	{"C_EconEntity->m_AttributeManager", 0x1378},
	{"C_EconEntity->m_OriginalOwnerXuidLow", 0x1848},
	{"C_EconEntity->m_OriginalOwnerXuidHigh", 0x184C},
	{"C_EconEntity->m_nFallbackPaintKit", 0x1850},
	{"C_EconEntity->m_nFallbackSeed", 0x1854},
	{"C_EconEntity->m_flFallbackWear", 0x1858},
	{"C_EconEntity->m_nFallbackStatTrak", 0x185C},

	// CCSWeaponBaseVData
	{"CCSWeaponBaseVData->m_WeaponType", 0x440},
	{"CCSWeaponBaseVData->m_szName", 0x640},
	{"CCSWeaponBaseVData->m_nDamage", 0x740},
	{"CCSWeaponBaseVData->m_flHeadshotMultiplier", 0x744},
	{"CCSWeaponBaseVData->m_flArmorRatio", 0x748},
	{"CCSWeaponBaseVData->m_flPenetration", 0x74C},
	{"CCSWeaponBaseVData->m_flRange", 0x750},
	{"CCSWeaponBaseVData->m_flRangeModifier", 0x754},

	// CBasePlayerWeaponVData
	{"CBasePlayerWeaponVData->m_iMaxClip1", 0x3F0},

	// C_PostProcessingVolume
	{"C_PostProcessingVolume->m_flMinExposure", 0xF7C},
	{"C_PostProcessingVolume->m_flMaxExposure", 0xF80},
	{"C_PostProcessingVolume->m_bExposureControl", 0xF95},
};

// ============================================================================
// internal helpers
// ============================================================================

static CSchemaSystemTypeScope* FindTypeScopeForModule(const char* szModuleName)
{
	if (!I::SchemaSystem)
		return nullptr;

	using FindTypeScopeFn = CSchemaSystemTypeScope * (__thiscall*)(void*, const char*, void*);
	const auto pVTable = *reinterpret_cast<std::uintptr_t**>(I::SchemaSystem);
	const auto fn = reinterpret_cast<FindTypeScopeFn>(pVTable[SchemaVFuncs::FIND_TYPE_SCOPE_FOR_MODULE]);

	return fn(I::SchemaSystem, szModuleName, nullptr);
}

static bool IsValidReadPtr(const void* ptr)
{
	__try
	{
		volatile auto test = *static_cast<const volatile char*>(ptr);
		(void)test;
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}

// store a class's fields into the offset maps
static void StoreClassFields(SchemaClassInfoData_t* pClassInfo)
{
	if (!pClassInfo || !pClassInfo->szName)
		return;

	const FNV1A_t nClassHash = FNV1A::Hash(pClassInfo->szName);
	auto& fieldMap = s_mapSchemaOffsets[nClassHash];

	if (pClassInfo->nFieldCount > 0 && pClassInfo->pFields)
	{
		for (std::int16_t f = 0; f < pClassInfo->nFieldCount; ++f)
		{
			const auto& field = pClassInfo->pFields[f];
			if (!field.szName)
				continue;

			const FNV1A_t nFieldHash = FNV1A::Hash(field.szName);
			const auto nOffset = static_cast<std::uint32_t>(field.nSingleInheritanceOffset);

			fieldMap[nFieldHash] = nOffset;

			std::string combined = std::string(pClassInfo->szName) + "->" + field.szName;
			s_mapFlatOffsets[FNV1A::Hash(combined.c_str())] = nOffset;

			++s_nTotalFields;

			s_vecDebugFields.push_back({
				pClassInfo->szName,
				field.szName,
				nOffset
			});
		}
	}

	++s_nTotalClasses;
}

// All class names we need offsets for — used by direct FindDeclaredClass approach
static const char* s_arrRequiredClasses[] =
{
	"CEntityInstance", "CEntityIdentity",
	"CGameSceneNode", "CSkeletonInstance", "CModelState",
	"CCollisionProperty", "CGlowProperty",
	"EntitySpottedState_t",
	"C_BaseEntity", "C_BaseModelEntity", "CBaseAnimGraph",
	"C_BaseFlex",
	"C_PostProcessingVolume",
	"C_BaseToggle", "C_BaseTrigger",
	"C_BaseViewModel", "C_PredictedViewModel", "C_CSGOViewModel",
	"CBasePlayerController", "CCSPlayerController",
	"CCSPlayerController_InGameMoneyServices",
	"CCSPlayerController_ActionTrackingServices",
	"CCSPlayerController_InventoryServices",
	"CPlayerControllerComponent", "CPlayerPawnComponent",
	"CPlayer_WeaponServices", "CCSPlayer_WeaponServices",
	"CPlayer_ItemServices", "CCSPlayer_ItemServices",
	"CPlayer_ObserverServices",
	"CPlayer_ViewModelServices", "CCSPlayer_ViewModelServices",
	"CPlayer_CameraServices", "CCSPlayerBase_CameraServices",
	"C_BasePlayerPawn", "C_CSPlayerPawnBase", "C_CSPlayerPawn",
	"C_CSObserverPawn",
	"CBasePlayerWeaponVData", "CCSWeaponBaseVData",
	"C_EconItemView", "C_AttributeContainer", "CAttributeManager",
	"C_EconEntity",
	"C_BasePlayerWeapon", "C_CSWeaponBase", "C_CSWeaponBaseGun",
	"C_BaseCSGrenade", "C_BaseGrenade",
	"C_BaseCSGrenadeProjectile", "C_SmokeGrenadeProjectile",
	"C_C4", "C_PlantedC4",
	"C_EnvSky",
};

// Direct lookup approach: call FindDeclaredClass for each known class name
// This bypasses the CUtlTSHash iteration entirely and is more reliable
static int DirectLookupClasses(CSchemaSystemTypeScope* pScope)
{
	if (!pScope)
		return 0;

	int nFound = 0;

	for (const auto* szClassName : s_arrRequiredClasses)
	{
		__try
		{
			SchemaClassInfoData_t* pClassInfo = pScope->FindDeclaredClass(szClassName);
			if (pClassInfo && pClassInfo->szName && pClassInfo->nFieldCount > 0)
			{
				StoreClassFields(pClassInfo);
				++nFound;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			// skip this class if SEH fires
		}
	}

	return nFound;
}

// load hardcoded fallback offsets into the flat map
static void LoadFallbackOffsets()
{
	L_PRINT(LOG_WARNING) << _XS("loading hardcoded fallback offsets (schema dump failed)");

	for (const auto& entry : s_arrFallbackOffsets)
	{
		const FNV1A_t nHash = FNV1A::Hash(entry.szCombined);
		if (s_mapFlatOffsets.find(nHash) == s_mapFlatOffsets.end())
		{
			s_mapFlatOffsets[nHash] = entry.nOffset;
			++s_nTotalFields;
		}
	}

	L_PRINT(LOG_INFO) << _XS("loaded ") << s_nTotalFields << _XS(" fallback offsets");
}

// ============================================================================
// public API
// ============================================================================

bool SCHEMA::Setup()
{
	L_PRINT(LOG_INFO) << _XS("--- dumping schema system ---");

	if (!I::SchemaSystem)
	{
		L_PRINT(LOG_ERROR) << _XS("ISchemaSystem is null, cannot dump schemas");
		LoadFallbackOffsets();
		return true;
	}

	s_mapSchemaOffsets.clear();
	s_mapFlatOffsets.clear();
	s_vecDebugFields.clear();
	s_nTotalClasses = 0;
	s_nTotalFields  = 0;

	// dump schemas from primary game modules using direct FindDeclaredClass
	const char* arrModules[] = {
		PATTERNS::MODULES::CLIENT,
		PATTERNS::MODULES::ENGINE2,
		PATTERNS::MODULES::SCHEMASYSTEM
	};

	for (const auto* szModule : arrModules)
	{
		DumpModule(szModule);
	}

	L_PRINT(LOG_INFO) << _XS("schema dump complete: ") << s_nTotalClasses << _XS(" classes, ")
		<< s_nTotalFields << _XS(" fields");

	if (s_nTotalFields == 0)
	{
		L_PRINT(LOG_WARNING) << _XS("runtime schema dump found 0 fields, loading fallbacks");
		LoadFallbackOffsets();
	}
	else
	{
		// merge fallback offsets for any fields that the runtime dump missed
		std::size_t nMerged = 0;
		for (const auto& entry : s_arrFallbackOffsets)
		{
			const FNV1A_t nHash = FNV1A::Hash(entry.szCombined);
			if (s_mapFlatOffsets.find(nHash) == s_mapFlatOffsets.end())
			{
				s_mapFlatOffsets[nHash] = entry.nOffset;
				++nMerged;
			}
		}
		if (nMerged > 0)
			L_PRINT(LOG_INFO) << _XS("merged ") << nMerged << _XS(" fallback offsets for missing fields");
	}

	return true;
}

void SCHEMA::Destroy()
{
	s_mapSchemaOffsets.clear();
	s_mapFlatOffsets.clear();
	s_vecDebugFields.clear();
	s_nTotalClasses = 0;
	s_nTotalFields  = 0;

	L_PRINT(LOG_INFO) << _XS("schema data cleared");
}

std::uint32_t SCHEMA::GetOffset(FNV1A_t nClassHash, FNV1A_t nFieldHash)
{
	const auto itClass = s_mapSchemaOffsets.find(nClassHash);
	if (itClass == s_mapSchemaOffsets.end())
		return 0;

	const auto itField = itClass->second.find(nFieldHash);
	if (itField == itClass->second.end())
		return 0;

	return itField->second;
}

std::uint32_t SCHEMA::GetOffset(FNV1A_t nCombinedHash)
{
	const auto it = s_mapFlatOffsets.find(nCombinedHash);
	if (it == s_mapFlatOffsets.end())
		return 0;
	return it->second;
}

bool SCHEMA::DumpModule(const char* szModuleName)
{
	L_PRINT(LOG_INFO) << _XS("dumping schemas from: ") << szModuleName;

	CSchemaSystemTypeScope* pScope = FindTypeScopeForModule(szModuleName);
	if (!pScope)
	{
		L_PRINT(LOG_WARNING) << _XS("  failed to find type scope for ") << szModuleName;
		return false;
	}

	L_PRINT(LOG_INFO) << _XS("  type scope = ") << static_cast<const void*>(pScope);

	// use direct FindDeclaredClass lookup (bypasses CUtlTSHash iteration issues)
	int nFound = DirectLookupClasses(pScope);
	L_PRINT(LOG_INFO) << _XS("  found ") << nFound << _XS(" classes via direct lookup in ") << szModuleName;

	return nFound > 0;
}

void SCHEMA::DumpToFile(const char* szFilePath)
{
	std::ofstream file(szFilePath);
	if (!file.is_open())
	{
		L_PRINT(LOG_ERROR) << _XS("failed to open file for schema dump: ") << szFilePath;
		return;
	}

	file << "// GamerHack schema dump\n";
	file << "// Classes: " << s_nTotalClasses << "  Fields: " << s_nTotalFields << "\n\n";

	std::sort(s_vecDebugFields.begin(), s_vecDebugFields.end(),
		[](const DebugFieldInfo_t& a, const DebugFieldInfo_t& b)
		{
			if (a.szClassName != b.szClassName)
				return a.szClassName < b.szClassName;
			return a.nOffset < b.nOffset;
		});

	std::string szLastClass;
	for (const auto& field : s_vecDebugFields)
	{
		if (field.szClassName != szLastClass)
		{
			if (!szLastClass.empty())
				file << "}\n\n";

			file << "class " << field.szClassName << " {\n";
			szLastClass = field.szClassName;
		}

		char szOffsetHex[16];
		snprintf(szOffsetHex, sizeof(szOffsetHex), "0x%04X", field.nOffset);
		file << "    " << szOffsetHex << " " << field.szFieldName << "\n";
	}

	if (!szLastClass.empty())
		file << "}\n";

	file.close();
	L_PRINT(LOG_INFO) << _XS("schema dump written to: ") << szFilePath;
}
