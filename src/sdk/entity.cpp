#include "entity.h"
#include "interfaces/cgameentitysystem.h"
#include "interfaces/igameresourceservice.h"
#include "interfaces/iengineclient.h"
#include "interfaces/ischemasystem.h"
#include "functionlist.h"

// ---------------------------------------------------------------
// placeholder empty vector for null-safe origin returns
// ---------------------------------------------------------------
static Vector3 s_vecEmpty{ 0.0f, 0.0f, 0.0f };

// ---------------------------------------------------------------
// C_BaseEntity — type checks
// ---------------------------------------------------------------
static bool CheckSchemaClassName(C_BaseEntity* pEntity, FNV1A_t uTargetHash)
{
	SchemaClassInfoData_t* pClassInfo = nullptr;
	pEntity->GetSchemaClassInfo(&pClassInfo);

	if (pClassInfo == nullptr)
		return false;

	return FNV1A::Hash(pClassInfo->szName) == uTargetHash;
}

bool C_BaseEntity::IsBasePlayerController()
{
	return CheckSchemaClassName(this, FNV1A::HashConst("C_CSPlayerController"));
}

bool C_BaseEntity::IsBasePlayerWeapon()
{
	return CheckSchemaClassName(this, FNV1A::HashConst("C_CSWeaponBase")) ||
	       CheckSchemaClassName(this, FNV1A::HashConst("C_CSWeaponBaseGun")) ||
	       CheckSchemaClassName(this, FNV1A::HashConst("C_BasePlayerWeapon"));
}

bool C_BaseEntity::IsObserverPawn()
{
	return CheckSchemaClassName(this, FNV1A::HashConst("C_CSObserverPawn"));
}

bool C_BaseEntity::IsPlayerPawn()
{
	return CheckSchemaClassName(this, FNV1A::HashConst("C_CSPlayerPawn"));
}

bool C_BaseEntity::IsPlantedC4()
{
	return CheckSchemaClassName(this, FNV1A::HashConst("C_PlantedC4"));
}

bool C_BaseEntity::IsC4()
{
	return CheckSchemaClassName(this, FNV1A::HashConst("C_C4"));
}

bool C_BaseEntity::IsSmokeGrenadeProjectile()
{
	return CheckSchemaClassName(this, FNV1A::HashConst("C_SmokeGrenadeProjectile"));
}

bool C_BaseEntity::IsGrenadeProjectile()
{
	return CheckSchemaClassName(this, FNV1A::HashConst("C_BaseCSGrenadeProjectile")) ||
	       CheckSchemaClassName(this, FNV1A::HashConst("C_SmokeGrenadeProjectile"));
}

bool C_BaseEntity::IsCS2HudModelWeapon()
{
	return CheckSchemaClassName(this, FNV1A::HashConst("C_CS2HudModelWeapon"));
}

bool C_BaseEntity::IsChicken()
{
	return CheckSchemaClassName(this, FNV1A::HashConst("C_Chicken"));
}

bool C_BaseEntity::IsHostage()
{
	return CheckSchemaClassName(this, FNV1A::HashConst("C_Hostage"));
}

bool C_BaseEntity::IsAlive()
{
	return GetLifeState() == LIFE_ALIVE;
}

const Vector3& C_BaseEntity::GetSceneOrigin()
{
	CGameSceneNode* pNode = GetGameSceneNode();
	if (pNode != nullptr)
		return pNode->GetAbsOrigin();

	return s_vecEmpty;
}

const Vector3& C_BaseEntity::GetOrigin()
{
	return GetSceneOrigin();
}

bool CGameSceneNode::GetBonePosition(std::int32_t nBoneIndex, Vector3& vecBonePos)
{
	CSkeletonInstance* pSkeleton = GetSkeletonInstance();
	if (!pSkeleton || nBoneIndex < 0)
		return false;

	const int nBoneCount = pSkeleton->GetBoneCount();
	if (nBoneIndex >= nBoneCount)
		return false;

	if (Matrix2x4* pBoneCache = pSkeleton->GetBoneCache(); pBoneCache != nullptr)
	{
		vecBonePos = pBoneCache->GetOrigin(nBoneIndex);
		if (!vecBonePos.IsZero())
			return true;
	}

	CModelState& modelState = pSkeleton->GetModelState();
	CBoneData* pBones = modelState.m_pBones;
	if (!pBones)
		return false;

	vecBonePos = pBones[nBoneIndex].position;
	return !vecBonePos.IsZero();
}

bool C_BaseEntity::GetBoundingBox(Vector3& vecMins, Vector3& vecMaxs)
{
	CCollisionProperty* pCollision = GetCollision();
	if (!pCollision)
		return false;

	const Vector3& vecOrigin = GetSceneOrigin();
	vecMins = vecOrigin + pCollision->GetMins();
	vecMaxs = vecOrigin + pCollision->GetMaxs();
	return true;
}

bool C_BaseEntity::ComputeHitboxSurroundingBox(Vector3* pMins, Vector3* pMaxs)
{
	if (!pMins || !pMaxs)
		return false;

	if (SDK_FUNC::ComputeHitboxSurroundingBox)
		return SDK_FUNC::ComputeHitboxSurroundingBox(this, pMins, pMaxs);

	return GetBoundingBox(*pMins, *pMaxs);
}

int C_BaseEntity::GetBoneIdByName(const char* szName)
{
	if (SDK_FUNC::GetBoneIdByName && szName && szName[0] != '\0')
		return SDK_FUNC::GetBoneIdByName(this, szName);

	CGameSceneNode* pGameSceneNode = GetGameSceneNode();
	if (!pGameSceneNode)
		return -1;

	CSkeletonInstance* pSkeleton = pGameSceneNode->GetSkeletonInstance();
	if (!pSkeleton)
		return -1;

	CModelState& modelState = pSkeleton->GetModelState();
	CStrongHandle<CModel> hModel = modelState.GetModel();
	CModel* pModel = static_cast<CModel*>(hModel);
	if (!pModel || !pModel->m_szBoneNames)
		return -1;

	for (std::uint32_t i = 0; i < pModel->m_nBoneCount; ++i)
	{
		if (pModel->m_szBoneNames[i] && std::strcmp(pModel->m_szBoneNames[i], szName) == 0)
			return static_cast<int>(i);
	}

	return -1;
}

// ---------------------------------------------------------------
// C_CSPlayerPawn
// ---------------------------------------------------------------
bool C_CSPlayerPawn::IsAlive()
{
	// player pawn life check — checks lifeState directly
	return GetLifeState() == LIFE_ALIVE && GetHealth() > 0;
}

bool C_CSPlayerPawn::IsOtherEnemy(C_CSPlayerPawn* pOther)
{
	if (pOther == nullptr || this == pOther)
		return false;

	return GetAssociatedTeam() != pOther->GetAssociatedTeam();
}

int C_CSPlayerPawn::GetAssociatedTeam()
{
	return static_cast<int>(GetTeam());
}

std::uint16_t C_CSPlayerPawn::GetCollisionMask()
{
	CCollisionProperty* pCollision = GetCollision();
	if (pCollision != nullptr)
		return pCollision->CollisionMask();

	return 0;
}

bool C_CSPlayerPawn::HasArmor(int nHitGroup)
{
	CCSPlayer_ItemServices* pItemServices = GetItemServices();
	if (!pItemServices)
		return false;

	// head hitgroup requires helmet
	if (nHitGroup == 1)
		return pItemServices->HasHelmet();

	return GetArmorValue() > 0;
}

std::vector<C_CS2HudModelWeapon*> C_CSPlayerPawn::GetViewModels()
{
	std::vector<C_CS2HudModelWeapon*> vecResult;

	CGameSceneNode* pGameSceneNode = GetGameSceneNode();
	if (!pGameSceneNode)
		return vecResult;

	CGameSceneNode* pChild = pGameSceneNode->GetChild();
	while (pChild)
	{
		CEntityInstance* pOwner = pChild->GetOwner();
		if (pOwner)
		{
			C_BaseEntity* pEntity = static_cast<C_BaseEntity*>(pOwner);
			if (pEntity->IsCS2HudModelWeapon())
				vecResult.push_back(static_cast<C_CS2HudModelWeapon*>(pEntity));
		}
		pChild = pChild->GetNextSibling();
	}

	return vecResult;
}

C_CS2HudModelWeapon* C_CSPlayerPawn::GetViewModel()
{
	auto vecViewModels = GetViewModels();
	return vecViewModels.empty() ? nullptr : vecViewModels.front();
}

C_CS2HudModelWeapon* C_CSPlayerPawn::GetKnifeModel()
{
	auto vecViewModels = GetViewModels();
	// knife model is typically the second view model
	return vecViewModels.size() > 1 ? vecViewModels[1] : nullptr;
}

// ---------------------------------------------------------------
// CCSPlayerController
// ---------------------------------------------------------------

/*
CCSPlayerController* CCSPlayerController::GetLocalPlayerController()
{
	const int nIndex = g_pEngineClient->GetLocalPlayer();
	return g_pGameResourceService->pGameEntitySystem->Get<CCSPlayerController>(nIndex);
}

const Vector3& CCSPlayerController::GetPawnOrigin()
{
	CBaseHandle hPawn = GetPawnHandle();
	if (!hPawn.IsValid())
		return s_vecEmpty;

	C_CSPlayerPawn* pPawn = g_pGameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(hPawn);
	if (pPawn == nullptr)
		return s_vecEmpty;

	return pPawn->GetSceneOrigin();
}
*/

// ---------------------------------------------------------------
// C_EconItemView — delegating to pattern-resolved functions
// ---------------------------------------------------------------
#include "functionlist.h"

CEconItem* C_EconItemView::GetSOCData()
{
	// SOC data retrieval is inventory-specific; placeholder
	return nullptr;
}

CEconItemDefinition* C_EconItemView::GetStaticData()
{
	if (!SDK_FUNC::C_EconItemView_GetStaticData)
		return nullptr;
	return SDK_FUNC::C_EconItemView_GetStaticData(this);
}

CCSWeaponBaseVData* C_EconItemView::GetBasePlayerWeaponVData()
{
	if (!SDK_FUNC::C_EconItemView_GetBasePlayerWeaponVData)
		return nullptr;
	return SDK_FUNC::C_EconItemView_GetBasePlayerWeaponVData(this);
}
