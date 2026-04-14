#pragma once
#include <cstdint>
#include <type_traits>
#include <vector>

#include "../utilities/fnv1a.h"
#include "../utilities/memory.h"

#include "datatypes/vector.h"
#include "datatypes/qangle.h"
#include "datatypes/matrix.h"
#include "datatypes/color.h"
#include "datatypes/utlvector.h"
#include "datatypes/utlsymbol.h"
#include "datatypes/stronghandle.h"
#include "entity_handle.h"
#include "const.h"

// ---------------------------------------------------------------
// forward declarations
// ---------------------------------------------------------------
class CGameEntitySystem;
struct SchemaClassInfoData_t;
class C_BaseEntity;
class CEntityInstance;
class CEconItem;
class CEconItemDefinition;
class CSkeletonInstance;
class C_CSPlayerPawn;
class C_CSWeaponBase;
class C_BasePlayerWeapon;
class C_CS2HudModelWeapon;
class CCSWeaponBaseVData;

// ---------------------------------------------------------------
// SCHEMA namespace — offset resolution (implemented in core/schema.cpp)
// ---------------------------------------------------------------
namespace SCHEMA
{
	/// resolve field offset from hashed "ClassName->fieldName" string
	[[nodiscard]] std::uint32_t GetOffset(const FNV1A_t uHashedFieldName);
}

// ---------------------------------------------------------------
// schema field macros
// ---------------------------------------------------------------

/// returns a reference to a schema field
/// @param TYPE       — C++ type of the field
/// @param NAME       — accessor function name
/// @param FIELD      — schema field string in format "ClassName->m_fieldName"
/// @note if the offset is 0 (schema dump failed), accessing the field will read the vtable.
///       callers should check schema validity or use null checks on pointer-type fields.
#define SCHEMA_FIELD(TYPE, NAME, FIELD) \
	[[nodiscard]] __forceinline std::add_lvalue_reference_t<TYPE> NAME() \
	{ \
		static const std::uint32_t uOffset = SCHEMA::GetOffset(FNV1A::HashConst(FIELD)); \
		return *reinterpret_cast<std::add_pointer_t<TYPE>>(reinterpret_cast<std::uint8_t*>(this) + uOffset); \
	}

/// returns a pointer to a schema field
#define SCHEMA_FIELD_POINTER(TYPE, NAME, FIELD) \
	[[nodiscard]] __forceinline std::add_pointer_t<TYPE> NAME() \
	{ \
		static const std::uint32_t uOffset = SCHEMA::GetOffset(FNV1A::HashConst(FIELD)); \
		return reinterpret_cast<std::add_pointer_t<TYPE>>(reinterpret_cast<std::uint8_t*>(this) + uOffset); \
	}

/// returns a reference to a schema field with additional byte offset
#define SCHEMA_FIELD_OFFSET(TYPE, NAME, FIELD, ADDITIONAL) \
	[[nodiscard]] __forceinline std::add_lvalue_reference_t<TYPE> NAME() \
	{ \
		static const std::uint32_t uOffset = SCHEMA::GetOffset(FNV1A::HashConst(FIELD)) + (ADDITIONAL); \
		return *reinterpret_cast<std::add_pointer_t<TYPE>>(reinterpret_cast<std::uint8_t*>(this) + uOffset); \
	}

/// raw offset accessor (no schema — hardcoded offset from `this`)
#define OFFSET_FIELD(TYPE, NAME, OFFSET) \
	[[nodiscard]] __forceinline std::add_lvalue_reference_t<TYPE> NAME() \
	{ \
		return *reinterpret_cast<std::add_pointer_t<TYPE>>(reinterpret_cast<std::uint8_t*>(this) + (OFFSET)); \
	}

// ---------------------------------------------------------------
// type aliases
// ---------------------------------------------------------------
using GameTime_t    = float;
using GameTick_t    = std::int32_t;
using CGlobalSymbol = const char*;

// map Source 2 networked container types to our implementations
#define C_NetworkUtlVectorBase        CUtlVector
#define C_UtlVectorEmbeddedNetworkVar CUtlVector
#define CNetworkViewOffsetVector      Vector3

// ---------------------------------------------------------------
// component base classes
// ---------------------------------------------------------------
class CPlayerControllerComponent {};
class CPlayerPawnComponent {};
class CEntitySubclassVDataBase {};

// ---------------------------------------------------------------
// bone / model data
// ---------------------------------------------------------------
struct alignas(16) CBoneData
{
	Vector3       position;
	float         scale;
	Vector3       rotation;
};

struct alignas(16) QuaternionAligned
{
	constexpr QuaternionAligned(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f) :
		x(x), y(y), z(z), w(w) { }

	[[nodiscard]] Matrix3x4 ToMatrix(const Vector3& vecOrigin = {}) const
	{
		Matrix3x4 matOut;

		matOut[0][0] = 1.0f - 2.0f * y * y - 2.0f * z * z;
		matOut[1][0] = 2.0f * x * y + 2.0f * w * z;
		matOut[2][0] = 2.0f * x * z - 2.0f * w * y;

		matOut[0][1] = 2.0f * x * y - 2.0f * w * z;
		matOut[1][1] = 1.0f - 2.0f * x * x - 2.0f * z * z;
		matOut[2][1] = 2.0f * y * z + 2.0f * w * x;

		matOut[0][2] = 2.0f * x * z + 2.0f * w * y;
		matOut[1][2] = 2.0f * y * z - 2.0f * w * x;
		matOut[2][2] = 1.0f - 2.0f * x * x - 2.0f * y * y;

		matOut[0][3] = vecOrigin.x;
		matOut[1][3] = vecOrigin.y;
		matOut[2][3] = vecOrigin.z;
		return matOut;
	}

	float x, y, z, w;
};

struct alignas(16) CTransform
{
	VectorAligned      vecPosition;
	QuaternionAligned  quatOrientation;
};

class CModel
{
private:
	std::uint8_t _pad0[0x168];
public:
	const char**  m_szBoneNames;
	std::uint32_t m_nBoneCount;
};

class CModelState
{
private:
	std::uint8_t _pad0[0x80];
public:
	CBoneData* m_pBones;

	SCHEMA_FIELD(CStrongHandle<CModel>, GetModel, "CModelState->m_hModel");
	SCHEMA_FIELD(CUtlSymbolLarge, GetModelName, "CModelState->m_ModelName");
};

// ---------------------------------------------------------------
// EntitySpottedState_t
// ---------------------------------------------------------------
class EntitySpottedState_t
{
public:
	SCHEMA_FIELD(bool, IsSpotted, "EntitySpottedState_t->m_bSpotted");
};

// ---------------------------------------------------------------
// CEntityIdentity
// ---------------------------------------------------------------
class CEntityIdentity
{
public:
	OFFSET_FIELD(std::uint32_t, GetIndex, 0x10);
	SCHEMA_FIELD(const char*, GetDesignerName, "CEntityIdentity->m_designerName");
	SCHEMA_FIELD(CUtlSymbolLarge, GetName, "CEntityIdentity->m_name");
	SCHEMA_FIELD(std::uint32_t, GetFlags, "CEntityIdentity->m_flags");

	[[nodiscard]] bool IsValid()
	{
		return GetIndex() != INVALID_EHANDLE_INDEX;
	}

	[[nodiscard]] int GetEntryIndex()
	{
		if (!IsValid())
			return ENT_ENTRY_MASK;

		return GetIndex() & ENT_ENTRY_MASK;
	}

	[[nodiscard]] int GetSerialNumber()
	{
		return GetIndex() >> NUM_SERIAL_NUM_SHIFT_BITS;
	}

	CEntityInstance* pInstance; // 0x00
};

// ---------------------------------------------------------------
// CEntityInstance — base of all entities
// ---------------------------------------------------------------
class CEntityInstance
{
public:
	void GetSchemaClassInfo(SchemaClassInfoData_t** ppReturn)
	{
		MEM::CallVFunc<void, 38U>(this, ppReturn);
	}

	[[nodiscard]] CBaseHandle GetRefEHandle()
	{
		CEntityIdentity* pIdentity = GetIdentity();
		if (pIdentity == nullptr)
			return CBaseHandle();

		return CBaseHandle(
			pIdentity->GetEntryIndex(),
			pIdentity->GetSerialNumber() - (pIdentity->GetFlags() & 1));
	}

	void PostDataUpdate(int nUpdateType = 1)
	{
		MEM::CallVFunc<void, 7U>(this, nUpdateType);
	}

	SCHEMA_FIELD(CEntityIdentity*, GetIdentity, "CEntityInstance->m_pEntity");
};

// ---------------------------------------------------------------
// CCollisionProperty
// ---------------------------------------------------------------
class CCollisionProperty
{
public:
	[[nodiscard]] std::uint16_t CollisionMask()
	{
		return *reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::uintptr_t>(this) + 0x38);
	}

	SCHEMA_FIELD(Vector3, GetMins, "CCollisionProperty->m_vecMins");
	SCHEMA_FIELD(Vector3, GetMaxs, "CCollisionProperty->m_vecMaxs");
	SCHEMA_FIELD(std::uint8_t, GetSolidFlags, "CCollisionProperty->m_usSolidFlags");
	SCHEMA_FIELD(std::uint8_t, GetCollisionGroup, "CCollisionProperty->m_CollisionGroup");
};

// ---------------------------------------------------------------
// CGlowProperty
// ---------------------------------------------------------------
class CGlowProperty
{
public:
	OFFSET_FIELD(CEntityInstance*, GetOwner, 0x18);

	SCHEMA_FIELD(Color, GetGlowColorOverride, "CGlowProperty->m_glowColorOverride");
	SCHEMA_FIELD(bool, IsGlowing, "CGlowProperty->m_bGlowing");
};

// ---------------------------------------------------------------
// CGameSceneNode — scene graph node, holds transforms
// ---------------------------------------------------------------
class CGameSceneNode
{
public:
	SCHEMA_FIELD(CTransform, GetNodeToWorld, "CGameSceneNode->m_nodeToWorld");
	SCHEMA_FIELD(Vector3, GetAbsOrigin, "CGameSceneNode->m_vecAbsOrigin");
	SCHEMA_FIELD(Vector3, GetRenderOrigin, "CGameSceneNode->m_vRenderOrigin");
	SCHEMA_FIELD(QAngle, GetAngleRotation, "CGameSceneNode->m_angRotation");
	SCHEMA_FIELD(QAngle, GetAbsAngleRotation, "CGameSceneNode->m_angAbsRotation");
	SCHEMA_FIELD(CEntityInstance*, GetOwner, "CGameSceneNode->m_pOwner");
	SCHEMA_FIELD(CGameSceneNode*, GetChild, "CGameSceneNode->m_pChild");
	SCHEMA_FIELD(CGameSceneNode*, GetNextSibling, "CGameSceneNode->m_pNextSibling");
	SCHEMA_FIELD(bool, IsDormant, "CGameSceneNode->m_bDormant");

	CSkeletonInstance* GetSkeletonInstance()
	{
		return reinterpret_cast<CSkeletonInstance*>(this);
	}

	void SetMeshGroupMask(std::uint64_t uMeshGroupMask);
	bool GetBonePosition(std::int32_t nBoneIndex, Vector3& vecBonePos);
};

// ---------------------------------------------------------------
// CSkeletonInstance — bone data access
// ---------------------------------------------------------------
class CSkeletonInstance : public CGameSceneNode
{
public:
	[[nodiscard]] int GetBoneCount()
	{
		CStrongHandle<CModel> hModel = GetModelState().GetModel();
		if (CModel* pModel = static_cast<CModel*>(hModel); pModel && pModel->m_nBoneCount > 0)
			return static_cast<int>(pModel->m_nBoneCount);

		return *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(this) + 0x1CC);
	}

	SCHEMA_FIELD(CModelState, GetModelState, "CSkeletonInstance->m_modelState");
	SCHEMA_FIELD(std::uint8_t, GetHitboxSet, "CSkeletonInstance->m_nHitboxSet");

	Matrix2x4* GetBoneCache()
	{
		return *reinterpret_cast<Matrix2x4**>(reinterpret_cast<std::uintptr_t>(this) + 0x1F0);
	}

	/// get world-space bone position by index
	[[nodiscard]] Vector3 GetBonePosition(int nBoneIndex)
	{
		CBoneData* pBones = GetModelState().m_pBones;
		const int nBoneCount = GetBoneCount();
		if (!pBones || nBoneIndex < 0 || nBoneIndex >= nBoneCount)
			return {};

		return pBones[nBoneIndex].position;
	}

	void CalcWorldSpaceBones(unsigned int nMask);
};

// ---------------------------------------------------------------
// weapon VData — static weapon definitions
// ---------------------------------------------------------------
class CBasePlayerWeaponVData : public CEntitySubclassVDataBase
{
public:
	SCHEMA_FIELD(std::int32_t, GetMaxClip1, "CBasePlayerWeaponVData->m_iMaxClip1");
};

class CCSWeaponBaseVData : public CBasePlayerWeaponVData
{
public:
	SCHEMA_FIELD(std::int32_t, GetWeaponType, "CCSWeaponBaseVData->m_WeaponType");
	SCHEMA_FIELD(CGlobalSymbol, GetWeaponName, "CCSWeaponBaseVData->m_szName");
	SCHEMA_FIELD(std::int32_t, GetDamage, "CCSWeaponBaseVData->m_nDamage");
	SCHEMA_FIELD(float, GetHeadshotMultiplier, "CCSWeaponBaseVData->m_flHeadshotMultiplier");
	SCHEMA_FIELD(float, GetArmorRatio, "CCSWeaponBaseVData->m_flArmorRatio");
	SCHEMA_FIELD(float, GetPenetration, "CCSWeaponBaseVData->m_flPenetration");
	SCHEMA_FIELD(float, GetRange, "CCSWeaponBaseVData->m_flRange");
	SCHEMA_FIELD(float, GetRangeModifier, "CCSWeaponBaseVData->m_flRangeModifier");
};

// ---------------------------------------------------------------
// C_EconItemView — networked item data
// ---------------------------------------------------------------
class C_EconItemView
{
public:
	CEconItem* GetSOCData();
	CEconItemDefinition* GetStaticData();
	CCSWeaponBaseVData* GetBasePlayerWeaponVData();

	SCHEMA_FIELD(std::uint16_t, GetItemDefinitionIndex, "C_EconItemView->m_iItemDefinitionIndex");
	SCHEMA_FIELD(std::uint64_t, GetItemID, "C_EconItemView->m_iItemID");
	SCHEMA_FIELD(std::uint32_t, GetItemIDHigh, "C_EconItemView->m_iItemIDHigh");
	SCHEMA_FIELD(std::uint32_t, GetItemIDLow, "C_EconItemView->m_iItemIDLow");
	SCHEMA_FIELD(std::uint32_t, GetAccountID, "C_EconItemView->m_iAccountID");
	SCHEMA_FIELD(bool, IsInitialized, "C_EconItemView->m_bInitialized");
	SCHEMA_FIELD(bool, IsDisallowSOC, "C_EconItemView->m_bDisallowSOC");
	SCHEMA_FIELD(char[161], GetCustomName, "C_EconItemView->m_szCustomName");
	SCHEMA_FIELD(char[161], GetCustomNameOverride, "C_EconItemView->m_szCustomNameOverride");
};

// ---------------------------------------------------------------
// attribute container
// ---------------------------------------------------------------
class CAttributeManager
{
public:
	virtual ~CAttributeManager() = 0;
};

class C_AttributeContainer : public CAttributeManager
{
public:
	SCHEMA_FIELD_POINTER(C_EconItemView, GetItem, "C_AttributeContainer->m_Item");
};

// ---------------------------------------------------------------
// C_BaseEntity
// ---------------------------------------------------------------
class C_BaseEntity : public CEntityInstance
{
public:
	// type checks (schema class name based)
	[[nodiscard]] bool IsBasePlayerController();
	[[nodiscard]] bool IsBasePlayerWeapon();
	[[nodiscard]] bool IsObserverPawn();
	[[nodiscard]] bool IsPlayerPawn();
	[[nodiscard]] bool IsPlantedC4();
	[[nodiscard]] bool IsC4();
	[[nodiscard]] bool IsSmokeGrenadeProjectile();
	[[nodiscard]] bool IsGrenadeProjectile();
	[[nodiscard]] bool IsCS2HudModelWeapon();
	[[nodiscard]] bool IsChicken();
	[[nodiscard]] bool IsHostage();

	// life state check
	[[nodiscard]] bool IsAlive();

	/// get entity origin from the scene graph node
	[[nodiscard]] const Vector3& GetSceneOrigin();

	/// get entity world origin (alias for GetSceneOrigin)
	[[nodiscard]] const Vector3& GetOrigin();

	/// compute axis-aligned bounding box in world space
	/// @param[out] vecMins — world-space mins
	/// @param[out] vecMaxs — world-space maxs
	/// @returns: true if computed successfully
	[[nodiscard]] bool GetBoundingBox(Vector3& vecMins, Vector3& vecMaxs);

	/// get bone id by name (searches model bone name table)
	[[nodiscard]] int GetBoneIdByName(const char* szName);

	/// compute hitbox surrounding box
	[[nodiscard]] bool ComputeHitboxSurroundingBox(Vector3* pMins, Vector3* pMaxs);

	SCHEMA_FIELD(CGameSceneNode*, GetGameSceneNode, "C_BaseEntity->m_pGameSceneNode");
	SCHEMA_FIELD(CCollisionProperty*, GetCollision, "C_BaseEntity->m_pCollision");
	SCHEMA_FIELD(std::uint8_t, GetTeam, "C_BaseEntity->m_iTeamNum");
	SCHEMA_FIELD(CBaseHandle, GetOwnerHandle, "C_BaseEntity->m_hOwnerEntity");
	SCHEMA_FIELD(Vector3, GetBaseVelocity, "C_BaseEntity->m_vecBaseVelocity");
	SCHEMA_FIELD(Vector3, GetAbsVelocity, "C_BaseEntity->m_vecAbsVelocity");
	SCHEMA_FIELD(std::uint32_t, GetFlags, "C_BaseEntity->m_fFlags");
	SCHEMA_FIELD(std::int32_t, GetEflags, "C_BaseEntity->m_iEFlags");
	SCHEMA_FIELD(std::uint8_t, GetMoveType, "C_BaseEntity->m_nActualMoveType");
	SCHEMA_FIELD(std::uint8_t, GetLifeState, "C_BaseEntity->m_lifeState");
	SCHEMA_FIELD(std::int32_t, GetHealth, "C_BaseEntity->m_iHealth");
	SCHEMA_FIELD(std::int32_t, GetMaxHealth, "C_BaseEntity->m_iMaxHealth");
	SCHEMA_FIELD(float, GetWaterLevel, "C_BaseEntity->m_flWaterLevel");
	SCHEMA_FIELD(CUtlStringToken, GetSubclassID, "C_BaseEntity->m_nSubclassID");
	SCHEMA_FIELD_OFFSET(void*, GetVData, "C_BaseEntity->m_nSubclassID", 0x8);
};

// ---------------------------------------------------------------
// C_BaseModelEntity
// ---------------------------------------------------------------
class C_BaseModelEntity : public C_BaseEntity
{
public:
	void SetModel(const char* szModel);

	SCHEMA_FIELD(CCollisionProperty, GetCollisionInstance, "C_BaseModelEntity->m_Collision");
	SCHEMA_FIELD(CGlowProperty, GetGlowProperty, "C_BaseModelEntity->m_Glow");
	SCHEMA_FIELD(Vector3, GetViewOffset, "C_BaseModelEntity->m_vecViewOffset");
	SCHEMA_FIELD(GameTime_t, GetCreationTime, "C_BaseModelEntity->m_flCreateTime");
	SCHEMA_FIELD(GameTick_t, GetCreationTick, "C_BaseModelEntity->m_nCreationTick");
	SCHEMA_FIELD(float, GetAnimTime, "C_BaseModelEntity->m_flAnimTime");
	SCHEMA_FIELD(float, GetSimulationTime, "C_BaseModelEntity->m_flSimulationTime");
};

// ---------------------------------------------------------------
// animation graph entities
// ---------------------------------------------------------------
class CBaseAnimGraph : public C_BaseModelEntity
{
public:
	SCHEMA_FIELD(bool, IsClientRagdoll, "CBaseAnimGraph->m_bClientRagdoll");
};

class C_BaseFlex : public CBaseAnimGraph
{
public:
};

// ---------------------------------------------------------------
// toggle / trigger / post-processing hierarchy
// ---------------------------------------------------------------
class C_BaseToggle : public C_BaseModelEntity
{
public:
};

class C_BaseTrigger : public C_BaseToggle
{
public:
};

class C_PostProcessingVolume : public C_BaseTrigger
{
public:
	SCHEMA_FIELD(float, GetMinExposure, "C_PostProcessingVolume->m_flMinExposure");
	SCHEMA_FIELD(float, GetMaxExposure, "C_PostProcessingVolume->m_flMaxExposure");
	SCHEMA_FIELD(bool, IsExposureControl, "C_PostProcessingVolume->m_bExposureControl");
};

// ---------------------------------------------------------------
// view model hierarchy
// ---------------------------------------------------------------
class C_BaseViewModel : public CBaseAnimGraph
{
public:
	SCHEMA_FIELD(CBaseHandle, GetWeaponHandle, "C_BaseViewModel->m_hWeapon");
};

class C_PredictedViewModel : public C_BaseViewModel
{
public:
};

class C_CSGOViewModel : public C_PredictedViewModel
{
public:
};

// ---------------------------------------------------------------
// HUD model hierarchy
// ---------------------------------------------------------------
class C_LateUpdatedAnimating : public CBaseAnimGraph
{
public:
};

class C_CS2HudModelBase : public C_LateUpdatedAnimating
{
public:
};

class C_CS2HudModelWeapon : public C_CS2HudModelBase
{
public:
};

class C_CS2HudModelArms : public C_CS2HudModelBase
{
public:
};

// ---------------------------------------------------------------
// controller services
// ---------------------------------------------------------------
class CCSPlayerController_InGameMoneyServices : public CPlayerControllerComponent
{
public:
	SCHEMA_FIELD(std::int32_t, GetAccount, "CCSPlayerController_InGameMoneyServices->m_iAccount");
};

class CCSPlayerController_InventoryServices : public CPlayerControllerComponent
{
public:
	SCHEMA_FIELD(std::uint16_t, GetMusicID, "CCSPlayerController_InventoryServices->m_unMusicID");
};

// ---------------------------------------------------------------
// pawn services
// ---------------------------------------------------------------
class CPlayer_WeaponServices : public CPlayerPawnComponent
{
public:
	SCHEMA_FIELD(CBaseHandle, GetActiveWeapon, "CPlayer_WeaponServices->m_hActiveWeapon");
	SCHEMA_FIELD(CUtlVector<CBaseHandle>, GetMyWeapons, "CPlayer_WeaponServices->m_hMyWeapons");
};

class CCSPlayer_WeaponServices : public CPlayer_WeaponServices
{
public:
	SCHEMA_FIELD(GameTime_t, GetNextAttack, "CCSPlayer_WeaponServices->m_flNextAttack");
};

class CPlayer_ItemServices : public CPlayerPawnComponent
{
public:
};

class CCSPlayer_ItemServices : public CPlayer_ItemServices
{
public:
	SCHEMA_FIELD(bool, HasDefuser, "CCSPlayer_ItemServices->m_bHasDefuser");
	SCHEMA_FIELD(bool, HasHelmet, "CCSPlayer_ItemServices->m_bHasHelmet");
	SCHEMA_FIELD(bool, HasHeavyArmor, "CCSPlayer_ItemServices->m_bHasHeavyArmor");
};

class CPlayer_ObserverServices : public CPlayerPawnComponent
{
public:
	SCHEMA_FIELD(std::uint8_t, GetObserverMode, "CPlayer_ObserverServices->m_iObserverMode");
	SCHEMA_FIELD(CBaseHandle, GetObserverTarget, "CPlayer_ObserverServices->m_hObserverTarget");
};

class CPlayer_ViewModelServices : public CPlayerPawnComponent
{
public:
};

class CCSPlayer_ViewModelServices : public CPlayer_ViewModelServices
{
public:
	SCHEMA_FIELD_POINTER(CBaseHandle, GetViewModel, "CCSPlayer_ViewModelServices->m_hViewModel");
};

class CPlayer_CameraServices : public CPlayerPawnComponent
{
public:
	SCHEMA_FIELD(QAngle, GetViewPunchAngle, "CPlayer_CameraServices->m_vecCsViewPunchAngle");
	SCHEMA_FIELD(CBaseHandle, GetActivePostProcessingVolume, "CPlayer_CameraServices->m_hActivePostProcessingVolume");
};

class CCSPlayerBase_CameraServices : public CPlayer_CameraServices
{
public:
	SCHEMA_FIELD(std::uint32_t, GetFOV, "CCSPlayerBase_CameraServices->m_iFOV");
};

// ---------------------------------------------------------------
// CBasePlayerController
// ---------------------------------------------------------------
class CBasePlayerController : public C_BaseModelEntity
{
public:
	SCHEMA_FIELD(std::uint64_t, GetSteamId, "CBasePlayerController->m_steamID");
	SCHEMA_FIELD(std::uint32_t, GetTickBase, "CBasePlayerController->m_nTickBase");
	SCHEMA_FIELD(CBaseHandle, GetPawnHandle, "CBasePlayerController->m_hPawn");
	SCHEMA_FIELD(bool, IsLocalPlayerController, "CBasePlayerController->m_bIsLocalPlayerController");
};

// ---------------------------------------------------------------
// CCSPlayerController
// ---------------------------------------------------------------
class CCSPlayerController : public CBasePlayerController
{
public:
	[[nodiscard]] static CCSPlayerController* GetLocalPlayerController();
	[[nodiscard]] const Vector3& GetPawnOrigin();

	SCHEMA_FIELD(CCSPlayerController_InGameMoneyServices*, GetInGameMoneyServices, "CCSPlayerController->m_pInGameMoneyServices");
	SCHEMA_FIELD(CCSPlayerController_InventoryServices*, GetInventoryServices, "CCSPlayerController->m_pInventoryServices");
	SCHEMA_FIELD(std::uint32_t, GetPing, "CCSPlayerController->m_iPing");
	SCHEMA_FIELD(const char*, GetPlayerName, "CCSPlayerController->m_sSanitizedPlayerName");
	SCHEMA_FIELD(std::int32_t, GetPawnHealth, "CCSPlayerController->m_iPawnHealth");
	SCHEMA_FIELD(std::int32_t, GetPawnArmor, "CCSPlayerController->m_iPawnArmor");
	SCHEMA_FIELD(bool, IsPawnHasDefuser, "CCSPlayerController->m_bPawnHasDefuser");
	SCHEMA_FIELD(bool, IsPawnHasHelmet, "CCSPlayerController->m_bPawnHasHelmet");
	SCHEMA_FIELD(bool, IsPawnAlive, "CCSPlayerController->m_bPawnIsAlive");
	SCHEMA_FIELD(CBaseHandle, GetPlayerPawnHandle, "CCSPlayerController->m_hPlayerPawn");
};

// ---------------------------------------------------------------
// C_BasePlayerPawn
// ---------------------------------------------------------------
class C_BasePlayerPawn : public C_BaseModelEntity
{
public:
	SCHEMA_FIELD(CBaseHandle, GetControllerHandle, "C_BasePlayerPawn->m_hController");
	SCHEMA_FIELD(CCSPlayer_WeaponServices*, GetWeaponServices, "C_BasePlayerPawn->m_pWeaponServices");
	SCHEMA_FIELD(CCSPlayer_ItemServices*, GetItemServices, "C_BasePlayerPawn->m_pItemServices");
	SCHEMA_FIELD(CPlayer_ObserverServices*, GetObserverServices, "C_BasePlayerPawn->m_pObserverServices");
	SCHEMA_FIELD(CCSPlayerBase_CameraServices*, GetCameraServices, "C_BasePlayerPawn->m_pCameraServices");
	SCHEMA_FIELD(Vector3, GetOldOrigin, "C_BasePlayerPawn->m_vOldOrigin");

	/// get eye position via VFunc (index 169)
	[[nodiscard]] Vector3 GetEyePosition()
	{
		Vector3 vecEyePos{};
		MEM::CallVFunc<void, 169U>(this, &vecEyePos);
		return vecEyePos;
	}
};

// ---------------------------------------------------------------
// C_CSPlayerPawnBase
// ---------------------------------------------------------------
class C_CSPlayerPawnBase : public C_BasePlayerPawn
{
public:
	SCHEMA_FIELD(CCSPlayer_ViewModelServices*, GetViewModelServices, "C_CSPlayerPawnBase->m_pViewModelServices");
	SCHEMA_FIELD(float, GetFlashBangTime, "C_CSPlayerPawnBase->m_flFlashBangTime");
	SCHEMA_FIELD(float, GetFlashMaxAlpha, "C_CSPlayerPawnBase->m_flFlashMaxAlpha");
	SCHEMA_FIELD(float, GetFlashDuration, "C_CSPlayerPawnBase->m_flFlashDuration");
	SCHEMA_FIELD(GameTime_t, GetLastSpawnTimeIndex, "C_CSPlayerPawnBase->m_flLastSpawnTimeIndex");
	SCHEMA_FIELD(Vector3, GetLastSmokeOverlayColor, "C_CSPlayerPawnBase->m_vLastSmokeOverlayColor");
};

// ---------------------------------------------------------------
// C_CSObserverPawn
// ---------------------------------------------------------------
class C_CSObserverPawn : public C_CSPlayerPawnBase
{
public:
};

// ---------------------------------------------------------------
// C_CSPlayerPawn — full player pawn
// ---------------------------------------------------------------
class C_CSPlayerPawn : public C_CSPlayerPawnBase
{
public:
	/// is this pawn alive (checks controller's pawn alive flag)
	[[nodiscard]] bool IsAlive();

	[[nodiscard]] bool IsOtherEnemy(C_CSPlayerPawn* pOther);
	[[nodiscard]] int GetAssociatedTeam();
	[[nodiscard]] std::uint16_t GetCollisionMask();

	/// check if player has armor protecting the given hitgroup
	[[nodiscard]] bool HasArmor(int nHitGroup);

	/// get HUD weapon view models
	[[nodiscard]] std::vector<C_CS2HudModelWeapon*> GetViewModels();
	[[nodiscard]] C_CS2HudModelWeapon* GetViewModel();
	[[nodiscard]] C_CS2HudModelWeapon* GetKnifeModel();

	SCHEMA_FIELD(bool, IsScoped, "C_CSPlayerPawn->m_bIsScoped");
	SCHEMA_FIELD(bool, IsDefusing, "C_CSPlayerPawn->m_bIsDefusing");
	SCHEMA_FIELD(bool, IsGrabbingHostage, "C_CSPlayerPawn->m_bIsGrabbingHostage");
	SCHEMA_FIELD(bool, IsWaitForNoAttack, "C_CSPlayerPawn->m_bWaitForNoAttack");
	SCHEMA_FIELD(bool, NeedToReApplyGloves, "C_CSPlayerPawn->m_bNeedToReApplyGloves");
	SCHEMA_FIELD(int, GetShotsFired, "C_CSPlayerPawn->m_iShotsFired");
	SCHEMA_FIELD(std::int32_t, GetArmorValue, "C_CSPlayerPawn->m_ArmorValue");
	SCHEMA_FIELD(QAngle, GetAimPunchAngle, "C_CSPlayerPawn->m_aimPunchAngle");
	SCHEMA_FIELD(EntitySpottedState_t, GetSpottedState, "C_CSPlayerPawn->m_entitySpottedState");
	SCHEMA_FIELD(CBaseHandle, GetHudModelArms, "C_CSPlayerPawn->m_hHudModelArms");
	SCHEMA_FIELD(bool, IsGunGameImmunity, "C_CSPlayerPawn->m_bGunGameImmunity");
	SCHEMA_FIELD(QAngle, GetEyeAngles, "C_CSPlayerPawn->m_angEyeAngles");

	// pointer variant for embedded C_EconItemView
	SCHEMA_FIELD_POINTER(C_EconItemView, GetEconGloves, "C_CSPlayerPawn->m_EconGloves");
};

// ---------------------------------------------------------------
// econ entity
// ---------------------------------------------------------------
class C_EconEntity : public C_BaseFlex
{
public:
	SCHEMA_FIELD_POINTER(C_AttributeContainer, GetAttributeManager, "C_EconEntity->m_AttributeManager");
	SCHEMA_FIELD(std::uint32_t, GetOriginalOwnerXuidLow, "C_EconEntity->m_OriginalOwnerXuidLow");
	SCHEMA_FIELD(std::uint32_t, GetOriginalOwnerXuidHigh, "C_EconEntity->m_OriginalOwnerXuidHigh");
	SCHEMA_FIELD(std::int32_t, GetFallbackPaintKit, "C_EconEntity->m_nFallbackPaintKit");
	SCHEMA_FIELD(std::int32_t, GetFallbackSeed, "C_EconEntity->m_nFallbackSeed");
	SCHEMA_FIELD(std::int32_t, GetFallbackWear, "C_EconEntity->m_flFallbackWear");
	SCHEMA_FIELD(std::int32_t, GetFallbackStatTrak, "C_EconEntity->m_nFallbackStatTrak");

	[[nodiscard]] std::uint64_t GetOriginalOwnerXuid()
	{
		return (static_cast<std::uint64_t>(GetOriginalOwnerXuidHigh()) << 32) | GetOriginalOwnerXuidLow();
	}
};

// ---------------------------------------------------------------
// weapon classes
// ---------------------------------------------------------------
class C_BasePlayerWeapon : public C_EconEntity
{
public:
	SCHEMA_FIELD(GameTick_t, GetNextPrimaryAttackTick, "C_BasePlayerWeapon->m_nNextPrimaryAttackTick");
	SCHEMA_FIELD(float, GetNextPrimaryAttackTickRatio, "C_BasePlayerWeapon->m_flNextPrimaryAttackTickRatio");
	SCHEMA_FIELD(GameTick_t, GetNextSecondaryAttackTick, "C_BasePlayerWeapon->m_nNextSecondaryAttackTick");
	SCHEMA_FIELD(float, GetNextSecondaryAttackTickRatio, "C_BasePlayerWeapon->m_flNextSecondaryAttackTickRatio");
	SCHEMA_FIELD(std::int32_t, GetClip1, "C_BasePlayerWeapon->m_iClip1");
	SCHEMA_FIELD(std::int32_t, GetClip2, "C_BasePlayerWeapon->m_iClip2");
	SCHEMA_FIELD(std::int32_t[2], GetReserveAmmo, "C_BasePlayerWeapon->m_pReserveAmmo");
};

class C_CSWeaponBase : public C_BasePlayerWeapon
{
public:
	void UpdateCompositeMaterial();
	void UpdateSubclass();
	void UpdateSkin();

	SCHEMA_FIELD(bool, IsInReload, "C_CSWeaponBase->m_bInReload");
	SCHEMA_FIELD(bool, IsBurstMode, "C_CSWeaponBase->m_bBurstMode");
	SCHEMA_FIELD(std::int32_t, GetOriginalTeamNumber, "C_CSWeaponBase->m_iOriginalTeamNumber");
};

class C_CSWeaponBaseGun : public C_CSWeaponBase
{
public:
	SCHEMA_FIELD(std::int32_t, GetZoomLevel, "C_CSWeaponBaseGun->m_zoomLevel");
	SCHEMA_FIELD(std::int32_t, GetBurstShotsRemaining, "C_CSWeaponBaseGun->m_iBurstShotsRemaining");
};

// ---------------------------------------------------------------
// grenade classes
// ---------------------------------------------------------------
class C_BaseGrenade : public C_BaseFlex
{
public:
};

class C_BaseCSGrenade : public C_CSWeaponBase
{
public:
	SCHEMA_FIELD(bool, IsHeldByPlayer, "C_BaseCSGrenade->m_bIsHeldByPlayer");
	SCHEMA_FIELD(bool, IsPinPulled, "C_BaseCSGrenade->m_bPinPulled");
	SCHEMA_FIELD(GameTime_t, GetThrowTime, "C_BaseCSGrenade->m_fThrowTime");
	SCHEMA_FIELD(float, GetThrowStrength, "C_BaseCSGrenade->m_flThrowStrength");
};

class C_BaseCSGrenadeProjectile : public C_BaseGrenade
{
public:
};

class C_SmokeGrenadeProjectile : public C_BaseCSGrenadeProjectile
{
public:
	SCHEMA_FIELD(bool, DidSmokeEffect, "C_SmokeGrenadeProjectile->m_bDidSmokeEffect");
	SCHEMA_FIELD(Vector3, GetSmokeColor, "C_SmokeGrenadeProjectile->m_vSmokeColor");
};

// ---------------------------------------------------------------
// C4 classes
// ---------------------------------------------------------------
class C_C4 : public C_CSWeaponBase
{
public:
	SCHEMA_FIELD(bool, IsStartedArming, "C_C4->m_bStartedArming");
	SCHEMA_FIELD(bool, IsBombPlacedAnimation, "C_C4->m_bBombPlacedAnimation");
	SCHEMA_FIELD(bool, IsBombPlanted, "C_C4->m_bBombPlanted");
};

class C_PlantedC4 : public CBaseAnimGraph
{
public:
	SCHEMA_FIELD(bool, IsBombTicking, "C_PlantedC4->m_bBombTicking");
	SCHEMA_FIELD(std::int32_t, GetBombSite, "C_PlantedC4->m_nBombSite");
	SCHEMA_FIELD(float, GetC4Blow, "C_PlantedC4->m_flC4Blow");
	SCHEMA_FIELD(bool, HasExploded, "C_PlantedC4->m_bHasExploded");
	SCHEMA_FIELD(bool, IsBeingDefused, "C_PlantedC4->m_bBeingDefused");
	SCHEMA_FIELD(bool, IsBombDefused, "C_PlantedC4->m_bBombDefused");
	SCHEMA_FIELD(CBaseHandle, GetBombDefuserHandle, "C_PlantedC4->m_hBombDefuser");
	SCHEMA_FIELD(float, GetTimerLength, "C_PlantedC4->m_flTimerLength");
	SCHEMA_FIELD(float, GetDefuseLength, "C_PlantedC4->m_flDefuseLength");
	SCHEMA_FIELD(float, GetDefuseCountDown, "C_PlantedC4->m_flDefuseCountDown");
};

// ---------------------------------------------------------------
// environment entities
// ---------------------------------------------------------------
class C_EnvSky : public C_BaseModelEntity
{
public:
	SCHEMA_FIELD(Color, GetTintColor, "C_EnvSky->m_vTintColor");
	SCHEMA_FIELD(Color, GetTintColorLightingOnly, "C_EnvSky->m_vTintColorLightingOnly");
};

// ---------------------------------------------------------------
// CHandle<T>::Get() implementation — requires CGameEntitySystem
// ---------------------------------------------------------------
// @note: this must be included AFTER CGameEntitySystem and
//        IGameResourceService are accessible via globals.
//        Typically resolved in a separate translation unit (entity.cpp)
//        where the interface pointers are available.
