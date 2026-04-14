#include "trace.h"

#include "../core/interfaces.h"
#include "bones.h"
#include "log.h"
#include "memory.h"
#include "xorstr.h"
#include "../sdk/entity.h"
#include "../sdk/interfaces/cgameentitysystem.h"

// ---------------------------------------------------------------
// setup / destroy
// ---------------------------------------------------------------
bool TRACE::Setup()
{
	if (!I::GameTraceManager)
	{
		L_PRINT(LOG_WARNING) << _XS("[TRACE] GameTraceManager is null — traces will fail");
		return true; // non-fatal, system can be used later if pointer becomes available
	}

	L_PRINT(LOG_INFO) << _XS("[TRACE] initialized");
	return true;
}

void TRACE::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[TRACE] destroyed");
}

// ---------------------------------------------------------------
// IsVisible — point-to-point visibility check
// ---------------------------------------------------------------
bool TRACE::IsVisible(const Vector3& vecStart, const Vector3& vecEnd, C_CSPlayerPawn* pSkipEntity)
{
	if (!I::GameTraceManager)
		return false;

	GameTrace_t trace{};
	if (!TraceShape(vecStart, vecEnd, pSkipEntity, &trace))
		return false;

	// fraction >= 0.97 means the ray reached (nearly) the end point
	return trace.m_flFraction >= 0.97f;
}

// ---------------------------------------------------------------
// IsBoneVisible — check visibility to a specific bone
// ---------------------------------------------------------------
bool TRACE::IsBoneVisible(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTargetPawn,
                          int nBoneIndex, Vector3* pOutBonePos)
{
	if (!pLocalPawn || !pTargetPawn)
		return false;

	// get eye position of local player
	Vector3 vecEyePos = pLocalPawn->GetEyePosition();
	if (vecEyePos.IsZero())
		return false;

	// get bone position of target
	auto* pSceneNode = pTargetPawn->GetGameSceneNode();
	if (!pSceneNode)
		return false;

	auto* pSkeleton = pSceneNode->GetSkeletonInstance();
	if (!pSkeleton)
		return false;

	BONES::CalcWorldSpaceBones(pSkeleton, 0xFFFFF);

	Vector3 vecBonePos;
	if (!pSceneNode->GetBonePosition(nBoneIndex, vecBonePos) || vecBonePos.IsZero())
		return false;

	if (pOutBonePos)
		*pOutBonePos = vecBonePos;

	return IsVisible(vecEyePos, vecBonePos, pLocalPawn);
}

// ---------------------------------------------------------------
// IsPlayerVisible — check if any bone on target player is visible
// ---------------------------------------------------------------
bool TRACE::IsPlayerVisible(C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pTargetController)
{
	if (!pLocalPawn || !pTargetController || !I::GameEntitySystem)
		return false;

	auto* pTargetPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pTargetController->GetPlayerPawnHandle());
	if (!pTargetPawn)
		return false;

	// check each visibility bone — return true if any is visible
	for (int i = 0; i < NUM_VISCHECK_BONES; i++)
	{
		if (IsBoneVisible(pLocalPawn, pTargetPawn, arrVisCheckBones[i]))
			return true;
	}

	return false;
}

// ---------------------------------------------------------------
// LineGoesThroughSmoke — check if line crosses smoke volume
// ---------------------------------------------------------------
bool TRACE::LineGoesThroughSmoke(const Vector3& vecStart, const Vector3& vecEnd)
{
	using fnLineGoesThroughSmoke = bool(__cdecl*)(const Vector3&, const Vector3&);
	static auto oLineGoesThroughSmoke = reinterpret_cast<fnLineGoesThroughSmoke>(
		MEM::FindPattern(_XS("client.dll"),
			_XS("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 44 0F B6")));

	if (!oLineGoesThroughSmoke)
		return false;

	return oLineGoesThroughSmoke(vecStart, vecEnd);
}

// ---------------------------------------------------------------
// TraceShape — generic trace using game's trace system
// ---------------------------------------------------------------
bool TRACE::TraceShape(const Vector3& vecStart, const Vector3& vecEnd,
                       C_CSPlayerPawn* pSkipEntity, GameTrace_t* pOutTrace)
{
	if (!I::GameTraceManager || !pOutTrace)
		return false;

	// build a zero-extent ray (point trace)
	Ray_t ray{};
	ray.m_vecStart = vecStart;
	ray.m_vecEnd = vecEnd;
	ray.m_vecMins = {};
	ray.m_vecMaxs = {};
	ray.m_nUnkType = 0;

	// build trace filter
	TraceFilter_t filter{};
	filter.m_uTraceMask = 0x1C1003; // MASK_SHOT (standard CS2 shot mask)
	filter.m_v3 = 0;
	filter.m_v4 = 0;
	filter.m_v5 = 0;

	// set skip handles from skip entity
	if (pSkipEntity)
	{
		CBaseHandle hSkip = pSkipEntity->GetRefEHandle();
		if (hSkip.IsValid())
		{
			filter.m_arrSkipHandles[0] = static_cast<std::int32_t>(hSkip.GetRawIndex());
			filter.m_arrSkipHandles[1] = static_cast<std::int32_t>(hSkip.GetRawIndex());
		}

		// get collision mask from entity
		std::uint16_t nCollisionMask = pSkipEntity->GetCollisionMask();
		if (nCollisionMask != 0)
			filter.m_uTraceMask = nCollisionMask;
	}

	return I::GameTraceManager->TraceShape(&ray, vecStart, vecEnd, &filter, pOutTrace);
}
