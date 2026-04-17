#include "schema.h"
#include "interfaces.h"
#include "patterns.h"
#include "../utilities/xorstr.h"
#include <fstream>
#include <algorithm>
#include <string>

using namespace SchemaInternal;

// ============================================================================
// internal storage — three parallel maps for different access patterns
// ============================================================================

// flat combined-hash -> offset: Hash("ClassName->fieldName") → offset
// used by entity.h SCHEMA_FIELD macros
static std::unordered_map<FNV1A_t, std::uint32_t> s_mapFlatOffsets;

// nested hash map: Hash(className) → { Hash(fieldName) → offset }
static std::unordered_map<FNV1A_t, std::unordered_map<FNV1A_t, std::uint32_t>> s_mapHashOffsets;

// string-based map: className → { fieldName → offset } (Andromeda-style, for debug)
static std::unordered_map<std::string, std::unordered_map<std::string, std::uint32_t>> s_mapStringOffsets;

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
// Used when runtime schema dump fails or as gap-filler.
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
	{"CGameSceneNode->m_nodeToWorld", 0x10},
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
	{"C_CSPlayerPawn->m_nSurvivalTeam", 0x26E0},

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
// helpers
// ============================================================================

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

// Store one class binding's fields into all three maps
static void StoreBinding(ClassBinding* pBinding)
{
	if (!pBinding)
		return;

	const char* szClassName = pBinding->GetName();
	if (!szClassName || !IsValidReadPtr(szClassName))
		return;

	const unsigned short nFieldCount = pBinding->GetDataArraySize();
	SchemaFieldData_t* pFields = pBinding->GetDataArray();
	if (nFieldCount == 0 || !pFields || !IsValidReadPtr(pFields))
		return;

	const FNV1A_t nClassHash = FNV1A::Hash(szClassName);
	auto& hashFieldMap = s_mapHashOffsets[nClassHash];
	auto& strFieldMap = s_mapStringOffsets[std::string(szClassName)];

	for (unsigned short f = 0; f < nFieldCount; ++f)
	{
		if (!IsValidReadPtr(&pFields[f]))
			break;

		const auto& field = pFields[f];
		if (!field.FieldName || !IsValidReadPtr(field.FieldName))
			continue;

		const auto nOffset = static_cast<std::uint32_t>(field.FieldOffset);
		const FNV1A_t nFieldHash = FNV1A::Hash(field.FieldName);

		// nested hash map
		hashFieldMap[nFieldHash] = nOffset;

		// flat combined-hash map (for entity.h macros)
		std::string combined = std::string(szClassName) + "->" + field.FieldName;
		s_mapFlatOffsets[FNV1A::Hash(combined.c_str())] = nOffset;

		// string map
		strFieldMap[std::string(field.FieldName)] = nOffset;

		++s_nTotalFields;

		DebugFieldInfo_t dbg;
		dbg.szClassName = szClassName;
		dbg.szFieldName = field.FieldName;
		dbg.nOffset = nOffset;
		s_vecDebugFields.push_back(dbg);
	}

	++s_nTotalClasses;
}

// SEH-safe wrapper to read NumSchema
static int SafeGetNumSchema(SchemaList<ClassBinding>* pContainer)
{
	__try
	{
		return pContainer->GetNumSchema();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return -1;
	}
}

// SEH-safe wrapper to iterate hash table buckets
static int SafeIterateBuckets(SchemaList<ClassBinding>* pContainer, int nNumSchema)
{
	int nBlockIndex = 0;
	int nFound = 0;

	__try
	{
		const auto& containers = pContainer->GetBlockContainers();

		for (const auto& bucket : containers)
		{
			for (auto* pBlock = bucket.GetFirstBlock();
				pBlock && nBlockIndex < nNumSchema;
				pBlock = pBlock->Next(), ++nBlockIndex)
			{
				auto* pBinding = pBlock->GetBinding();
				if (!pBinding || !IsValidReadPtr(pBinding))
					continue;

				StoreBinding(pBinding);
				++nFound;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// iteration crashed
	}

	return nFound;
}

// ============================================================================
// Andromeda-style: iterate 256-bucket hash table for a type scope
// ============================================================================
static int IterateHashTable(TypeScope* pScope)
{
	if (!pScope)
		return 0;

	auto* pClassContainer = pScope->GetClassContainer();
	if (!pClassContainer || !IsValidReadPtr(pClassContainer))
		return 0;

	int nNumSchema = SafeGetNumSchema(pClassContainer);
	if (nNumSchema < 0)
		return 0;

	if (nNumSchema == 0 || nNumSchema > 100000)
	{
		L_PRINT(LOG_WARNING) << _XS("  hash table NumSchema=") << nNumSchema << _XS(" — skipping");
		return 0;
	}

	L_PRINT(LOG_INFO) << _XS("  hash table reports ") << nNumSchema << _XS(" classes");

	int nFound = SafeIterateBuckets(pClassContainer, nNumSchema);
	if (nFound == 0)
		L_PRINT(LOG_WARNING) << _XS("  hash table iteration found 0 bindings");

	return nFound;
}

// ============================================================================
// Get GlobalTypeScope via VMT index 11
// ============================================================================
static TypeScope* GetGlobalTypeScope()
{
	if (!I::SchemaSystem)
		return nullptr;

	using Fn = TypeScope*(__thiscall*)(void*);
	const auto pVTable = *reinterpret_cast<std::uintptr_t**>(I::SchemaSystem);
	return reinterpret_cast<Fn>(pVTable[PATTERNS::VTABLE::SCHEMA::GLOBAL_TYPE_SCOPE])(I::SchemaSystem);
}

// ============================================================================
// Get all type scopes via pattern (Andromeda approach)
// ============================================================================
static TypeScope** GetAllTypeScopes(std::uint16_t& outCount)
{
	outCount = 0;

	// scan for the GetAllTypeScope pattern in schemasystem.dll
	const std::uintptr_t uPatternAddr = MEM::FindPattern(
		PATTERNS::MODULES::SCHEMASYSTEM,
		PATTERNS::FUNCTIONS::GET_ALL_TYPE_SCOPE,
		MEM::ESearchType::PTR);

	if (!uPatternAddr)
	{
		L_PRINT(LOG_WARNING) << _XS("GetAllTypeScope pattern not found");
		return nullptr;
	}

	// the scope count is stored 8 bytes before the resolved pointer
	auto** ppScopes = *reinterpret_cast<TypeScope***>(uPatternAddr);
	outCount = *reinterpret_cast<std::uint16_t*>(uPatternAddr - 0x8);

	L_PRINT(LOG_INFO) << _XS("GetAllTypeScope: ") << outCount << _XS(" scopes at ") << ppScopes;
	return ppScopes;
}

// ============================================================================
// FindTypeScopeForModule via VMT (fallback if hash table fails)
// ============================================================================
static TypeScope* FindTypeScopeForModule(const char* szModuleName)
{
	if (!I::SchemaSystem)
		return nullptr;

	using Fn = TypeScope*(__thiscall*)(void*, const char*, void*);
	const auto pVTable = *reinterpret_cast<std::uintptr_t**>(I::SchemaSystem);
	return reinterpret_cast<Fn>(pVTable[PATTERNS::VTABLE::SCHEMA::FIND_TYPE_SCOPE])(I::SchemaSystem, szModuleName, nullptr);
}

// ============================================================================
// DirectLookupClasses — fallback: call FindDeclaredClass per known class
// ============================================================================
static const char* s_arrRequiredClasses[] =
{
	"CEntityInstance", "CEntityIdentity",
	"CGameSceneNode", "CSkeletonInstance", "CModelState",
	"CCollisionProperty", "CGlowProperty",
	"EntitySpottedState_t",
	"C_BaseEntity", "C_BaseModelEntity", "CBaseAnimGraph",
	"C_BaseFlex", "C_PostProcessingVolume",
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
	"C_C4", "C_PlantedC4", "C_EnvSky",
};

// Use the old SchemaClassInfoData_t layout for FindDeclaredClass results
struct LegacySchemaClassInfo_t
{
	void*            pVtable;       // 0x00
	const char*      szName;        // 0x08
	const char*      szDescription; // 0x10
	int              m_nSize;       // 0x18
	std::int16_t     nFieldCount;   // 0x1C
	std::int16_t     _pad1e;        // 0x1E
	std::int16_t     _pad20;        // 0x20
	std::uint8_t     _pad22;        // 0x22
	std::uint8_t     _pad23;        // 0x23
	std::uint8_t     _pad24[0x04];  // 0x24
	SchemaFieldData_t* pFields;     // 0x28 — same layout as Andromeda's DataArray
};

// SEH-safe wrapper: calls FindDeclaredClass without C++ objects in scope
static void* SafeFindDeclaredClass(TypeScope* pScope, const char* szClassName)
{
	__try
	{
		return pScope->FindDeclaredClass(szClassName);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return nullptr;
	}
}

static int DirectLookupClasses(TypeScope* pScope)
{
	if (!pScope) return 0;
	int nFound = 0;

	for (const auto* szClassName : s_arrRequiredClasses)
	{
		auto* pInfo = reinterpret_cast<LegacySchemaClassInfo_t*>(
			SafeFindDeclaredClass(pScope, szClassName));
		if (!pInfo || !IsValidReadPtr(pInfo) || !pInfo->szName || pInfo->nFieldCount <= 0 || !pInfo->pFields)
			continue;

		const FNV1A_t nClassHash = FNV1A::Hash(szClassName);
		auto& hashFieldMap = s_mapHashOffsets[nClassHash];
		auto& strFieldMap = s_mapStringOffsets[std::string(szClassName)];

		for (std::int16_t f = 0; f < pInfo->nFieldCount; ++f)
		{
			if (!IsValidReadPtr(&pInfo->pFields[f]))
				break;

			const auto& field = pInfo->pFields[f];
			if (!field.FieldName || !IsValidReadPtr(field.FieldName))
				continue;

			const auto nOffset = static_cast<std::uint32_t>(field.FieldOffset);
			const FNV1A_t nFieldHash = FNV1A::Hash(field.FieldName);

			hashFieldMap[nFieldHash] = nOffset;
			strFieldMap[std::string(field.FieldName)] = nOffset;

			std::string combined = std::string(szClassName) + "->" + field.FieldName;
			s_mapFlatOffsets[FNV1A::Hash(combined.c_str())] = nOffset;

			++s_nTotalFields;

			DebugFieldInfo_t dbg;
			dbg.szClassName = szClassName;
			dbg.szFieldName = field.FieldName;
			dbg.nOffset = nOffset;
			s_vecDebugFields.push_back(dbg);
		}

		++s_nTotalClasses;
		++nFound;
	}

	return nFound;
}

// load hardcoded fallback offsets
static void LoadFallbackOffsets()
{
	L_PRINT(LOG_WARNING) << _XS("loading hardcoded fallback offsets");

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

// merge fallback offsets for any gaps
static std::size_t MergeFallbackOffsets()
{
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
	return nMerged;
}

// ============================================================================
// public API
// ============================================================================

bool SCHEMA::Setup()
{
	L_PRINT(LOG_INFO) << _XS("--- dumping schema system (Andromeda-style) ---");

	if (!I::SchemaSystem)
	{
		L_PRINT(LOG_ERROR) << _XS("ISchemaSystem is null");
		LoadFallbackOffsets();
		return true;
	}

	s_mapFlatOffsets.clear();
	s_mapHashOffsets.clear();
	s_mapStringOffsets.clear();
	s_vecDebugFields.clear();
	s_nTotalClasses = 0;
	s_nTotalFields  = 0;

	// ---- Phase 1: Andromeda approach — collect all type scopes and iterate hash tables ----
	std::vector<TypeScope*> vecScopes;

	// 1a. GlobalTypeScope via VMT[11]
	TypeScope* pGlobalScope = GetGlobalTypeScope();
	if (pGlobalScope)
	{
		L_PRINT(LOG_INFO) << _XS("GlobalTypeScope = ") << static_cast<void*>(pGlobalScope);
		vecScopes.push_back(pGlobalScope);
	}
	else
	{
		L_PRINT(LOG_WARNING) << _XS("GlobalTypeScope returned null");
	}

	// 1b. GetAllTypeScope via pattern
	std::uint16_t nScopeCount = 0;
	TypeScope** ppAllScopes = GetAllTypeScopes(nScopeCount);
	if (ppAllScopes && nScopeCount > 0)
	{
		for (std::uint16_t i = 0; i < nScopeCount; ++i)
		{
			if (ppAllScopes[i])
			{
				// avoid duplicates
				bool bDupe = false;
				for (const auto* pExisting : vecScopes)
				{
					if (pExisting == ppAllScopes[i]) { bDupe = true; break; }
				}
				if (!bDupe)
					vecScopes.push_back(ppAllScopes[i]);
			}
		}
	}

	L_PRINT(LOG_INFO) << _XS("total type scopes to iterate: ") << vecScopes.size();

	// 1c. Iterate hash tables for all scopes
	int nHashTableTotal = 0;
	for (auto* pScope : vecScopes)
	{
		const char* szModName = nullptr;
		if (IsValidReadPtr(pScope))
			szModName = pScope->GetModuleName();

		if (szModName && IsValidReadPtr(szModName))
			L_PRINT(LOG_INFO) << _XS("iterating scope: ") << szModName;
		else
			L_PRINT(LOG_INFO) << _XS("iterating scope: ") << static_cast<void*>(pScope);

		int nFound = IterateHashTable(pScope);
		nHashTableTotal += nFound;
	}

	L_PRINT(LOG_INFO) << _XS("hash table iteration: ") << nHashTableTotal << _XS(" classes, ")
		<< s_nTotalFields << _XS(" fields");

	// ---- Phase 2: Fallback — direct FindDeclaredClass if hash table got nothing ----
	if (s_nTotalFields == 0)
	{
		L_PRINT(LOG_WARNING) << _XS("hash table iteration yielded 0 fields, trying direct lookup...");

		const char* arrModules[] = {
			PATTERNS::MODULES::CLIENT,
			PATTERNS::MODULES::ENGINE2,
			PATTERNS::MODULES::SCHEMASYSTEM
		};

		int nDirectTotal = 0;
		for (const auto* szModule : arrModules)
		{
			TypeScope* pScope = FindTypeScopeForModule(szModule);
			if (pScope)
			{
				int nFound = DirectLookupClasses(pScope);
				L_PRINT(LOG_INFO) << _XS("  direct lookup in ") << szModule << _XS(": ") << nFound << _XS(" classes");
				nDirectTotal += nFound;
			}
		}

		L_PRINT(LOG_INFO) << _XS("direct lookup total: ") << nDirectTotal << _XS(" classes, ")
			<< s_nTotalFields << _XS(" fields");
	}

	// ---- Phase 3: Hardcoded fallbacks ----
	if (s_nTotalFields == 0)
	{
		LoadFallbackOffsets();
	}
	else
	{
		std::size_t nMerged = MergeFallbackOffsets();
		if (nMerged > 0)
			L_PRINT(LOG_INFO) << _XS("merged ") << nMerged << _XS(" fallback offsets for missing fields");
	}

	L_PRINT(LOG_INFO) << _XS("schema dump complete: ") << s_nTotalClasses << _XS(" classes, ")
		<< s_nTotalFields << _XS(" fields total");

	return true;
}

void SCHEMA::Destroy()
{
	s_mapFlatOffsets.clear();
	s_mapHashOffsets.clear();
	s_mapStringOffsets.clear();
	s_vecDebugFields.clear();
	s_nTotalClasses = 0;
	s_nTotalFields  = 0;

	L_PRINT(LOG_INFO) << _XS("schema data cleared");
}

std::uint32_t SCHEMA::GetOffset(FNV1A_t nCombinedHash)
{
	const auto it = s_mapFlatOffsets.find(nCombinedHash);
	if (it == s_mapFlatOffsets.end())
		return 0;
	return it->second;
}

std::uint32_t SCHEMA::GetOffset(FNV1A_t nClassHash, FNV1A_t nFieldHash)
{
	const auto itClass = s_mapHashOffsets.find(nClassHash);
	if (itClass == s_mapHashOffsets.end())
		return 0;
	const auto itField = itClass->second.find(nFieldHash);
	if (itField == itClass->second.end())
		return 0;
	return itField->second;
}

std::uint32_t SCHEMA::GetOffset(const char* szClassName, const char* szFieldName)
{
	const auto itClass = s_mapStringOffsets.find(szClassName);
	if (itClass == s_mapStringOffsets.end())
		return 0;
	const auto itField = itClass->second.find(szFieldName);
	if (itField == itClass->second.end())
		return 0;
	return itField->second;
}

std::size_t SCHEMA::GetTotalClasses() { return s_nTotalClasses; }
std::size_t SCHEMA::GetTotalFields()  { return s_nTotalFields; }

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
